/**
 * @file vc_coin_detection.cpp
 * @brief Enhanced functions for coin detection and classification.
 *
 * This file contains specialized functions to detect and classify
 * different types of coins with better robustness to lighting and positioning.
 *
 * @author Original: Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types_c.h>

#include "vc.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Constants for coin diameters (in pixels)
// Bronze/copper coins
const float D_1CENT = 122.0f;
const float D_2CENT = 135.0f;
const float D_5CENT = 152.0f;
// Gold coins
const float D_10CENT = 143.0f;
const float D_20CENT = 160.0f;
const float D_50CENT = 174.0f;

// Base tolerance for diameter comparison
const float BASE_TOLERANCE = 0.08f;

// Maximum count of coins to track
#define MAX_COINS 50

// Structure to represent a reference coin for calibration
typedef struct {
    float knownDiameterPixels;  // Diameter in pixels from reference capture
    float realDiameterMm;       // Real diameter in mm
    float calibrationFactor;    // Pixels-to-mm conversion factor
} CalibrationCoin;

// Global calibration factor - initialized with default value
static float g_pixelsToMmFactor = 1.0f;

// Known real sizes of euro coins in mm
const float REAL_SIZE_1CENT = 16.25f;   // 1 cent actual diameter
const float REAL_SIZE_2CENT = 18.75f;   // 2 cent actual diameter
const float REAL_SIZE_5CENT = 21.25f;   // 5 cent actual diameter
const float REAL_SIZE_10CENT = 19.75f;  // 10 cent actual diameter
const float REAL_SIZE_20CENT = 22.25f;  // 20 cent actual diameter
const float REAL_SIZE_50CENT = 24.25f;  // 50 cent actual diameter

/**
 * @brief Calculate adaptive tolerance based on coin position and frame edge proximity
 * 
 * Increase tolerance for coins near edges or in corners where perspective distortion
 * causes more significant diameter variations.
 * 
 * @param xc X-coordinate of coin center
 * @param yc Y-coordinate of coin center
 * @param frameWidth Width of the frame
 * @param frameHeight Height of the frame
 * @return Adjusted tolerance value
 */
float CalculateAdaptiveTolerance(int xc, int yc, int frameWidth, int frameHeight) {
    // Base tolerance
    float tolerance = BASE_TOLERANCE;
    
    // Edge proximity check parameters
    const int edgeMargin = 50;
    
    // Calculate distances to all four edges
    float distToLeftEdge = (float)xc;
    float distToRightEdge = (float)(frameWidth - xc);
    float distToTopEdge = (float)yc;
    float distToBottomEdge = (float)(frameHeight - yc);
    
    // Find the closest edge distance
    float minDist = fminf(fminf(distToLeftEdge, distToRightEdge), 
                         fminf(distToTopEdge, distToBottomEdge));
    
    // Increase tolerance for coins near edges
    if (minDist < edgeMargin) {
        // Scale tolerance linearly based on proximity to edge
        float scaleFactor = 1.0f + 0.5f * (1.0f - (minDist / edgeMargin));
        tolerance *= scaleFactor;
    }
    
    return tolerance;
}

/**
 * @brief Calibrate the pixel-to-mm conversion factor using a reference coin
 * 
 * This helps adjust for varying camera distances by using a known coin to calibrate.
 * 
 * @param referenceBlob Blob of a reference coin (any coin with known diameter)
 * @param referenceCoinType Type of the reference coin (1-6 for euro cents)
 * @return True if calibration successful, false otherwise
 */
bool CalibrateWithReferenceCoin(OVC *referenceBlob, int referenceCoinType) {
    if (!referenceBlob) return false;
    
    // Calculate diameter in pixels
    float diameterPixels = CalculateCircularDiameter(referenceBlob);
    
    // Get the real diameter in mm based on coin type
    float realDiameterMm = 0.0f;
    
    switch (referenceCoinType) {
        case 1: // 1 cent
            realDiameterMm = REAL_SIZE_1CENT;
            break;
        case 2: // 2 cent
            realDiameterMm = REAL_SIZE_2CENT;
            break;
        case 3: // 5 cent
            realDiameterMm = REAL_SIZE_5CENT;
            break;
        case 4: // 10 cent
            realDiameterMm = REAL_SIZE_10CENT;
            break;
        case 5: // 20 cent
            realDiameterMm = REAL_SIZE_20CENT;
            break;
        case 6: // 50 cent
            realDiameterMm = REAL_SIZE_50CENT;
            break;
        default:
            return false; // Unknown coin type
    }
    
    // Calculate calibration factor (pixels per mm)
    g_pixelsToMmFactor = diameterPixels / realDiameterMm;
    
    printf("Calibration complete: %.2f pixels per mm\n", g_pixelsToMmFactor);
    
    return true;
}

/**
 * @brief Get expected pixel diameter based on coin type and current calibration
 * 
 * @param coinType Type of coin (1-6 for euro cents)
 * @return Expected diameter in pixels
 */
float GetExpectedDiameter(int coinType) {
    float realDiameterMm = 0.0f;
    
    switch (coinType) {
        case 1: // 1 cent
            realDiameterMm = REAL_SIZE_1CENT;
            break;
        case 2: // 2 cent
            realDiameterMm = REAL_SIZE_2CENT;
            break;
        case 3: // 5 cent
            realDiameterMm = REAL_SIZE_5CENT;
            break;
        case 4: // 10 cent
            realDiameterMm = REAL_SIZE_10CENT;
            break;
        case 5: // 20 cent
            realDiameterMm = REAL_SIZE_20CENT;
            break;
        case 6: // 50 cent
            realDiameterMm = REAL_SIZE_50CENT;
            break;
        default:
            return 0.0f; // Unknown coin type
    }
    
    // Convert mm to pixels using calibration factor
    return realDiameterMm * g_pixelsToMmFactor;
}

/**
 * @brief Improved copper coin detection with adaptive tolerance
 */
bool DetectBronzeCoinsImproved(OVC *blob, OVC *copperBlobs, int ncopperBlobs, 
                             int *excludeList, int *counters, int distThresholdSq) {
    if (!blob || !copperBlobs || ncopperBlobs <= 0)
        return false;
    
    // Frame dimensions for adaptive tolerance
    const int frameWidth = 640;   // Assumed frame width
    const int frameHeight = 480;  // Assumed frame height
    
    // Edge proximity check parameter - distance from blob center to frame edge
    const int EDGE_MARGIN = 80; // Half the diameter of the largest copper coin + margin
    
    for (int i = 0; i < ncopperBlobs; i++) {
        // Verify blob validity
        if (copperBlobs[i].label == 0 || copperBlobs[i].area < 7000)
            continue;
            
        // Check if centers are close
        int dx = copperBlobs[i].xc - blob->xc;
        int dy = copperBlobs[i].yc - blob->yc;
        int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            // Found nearby copper blob
            float diameter = CalculateCircularDiameter(&copperBlobs[i]);
            float circularity = CalculateCircularity(&copperBlobs[i]);
            
            // Skip non-circular objects
            if (circularity < 0.75f) {
                continue;
            }
            
            // Apply shadow correction
            int correctedYC = copperBlobs[i].yc + (int)(diameter * 0.05f);
            
            // Calculate adaptive tolerance based on position in frame
            float tolerance = CalculateAdaptiveTolerance(copperBlobs[i].xc, copperBlobs[i].yc, 
                                                        frameWidth, frameHeight);
            
            // Check if blob is near the edge of the frame
            bool isNearEdge = false;
            if (copperBlobs[i].xc < EDGE_MARGIN || copperBlobs[i].yc < EDGE_MARGIN || 
                copperBlobs[i].xc > frameWidth - EDGE_MARGIN || copperBlobs[i].yc > frameHeight - EDGE_MARGIN) {
                isNearEdge = true;
                
                // For edge coins, try to maintain previous frame's classification
                int coinType = GetLastCoinTypeAtLocation(copperBlobs[i].xc, copperBlobs[i].yc);
                
                // If we have a previous classification and it's a copper coin, use it
                if (coinType >= 1 && coinType <= 3) {
                    // Only count if not already counted
                    if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, coinType, 1)) {
                        counters[coinType - 1]++;
                        printf("COIN COUNTED: %d cent (COPPER contour) | Diam: %.1f | Area: %d | Circularity: %.2f\n",
                            coinType == 1 ? 1 : (coinType == 2 ? 2 : 5), 
                            diameter, copperBlobs[i].area, circularity);
                    }
                    ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                    return true;
                }
                
                // If no previous classification or poor match, make a best guess based on diameter ratio
                float size1CentRatio = diameter / D_1CENT;
                float size2CentRatio = diameter / D_2CENT;
                float size5CentRatio = diameter / D_5CENT;
                
                int bestGuessType;
                // Find closest match by comparing ratios to 1.0
                float diff1Cent = fabsf(size1CentRatio - 1.0f);
                float diff2Cent = fabsf(size2CentRatio - 1.0f);
                float diff5Cent = fabsf(size5CentRatio - 1.0f);
                
                if (diff1Cent < diff2Cent && diff1Cent < diff5Cent) {
                    bestGuessType = 0; // 1c
                } else if (diff2Cent < diff1Cent && diff2Cent < diff5Cent) {
                    bestGuessType = 1; // 2c
                } else {
                    bestGuessType = 2; // 5c
                }
                
                // Only count if not already counted
                if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, bestGuessType + 1, 1)) {
                    counters[bestGuessType]++;
                    printf("COIN COUNTED: %d cent (COPPER contour) | Diam: %.1f | Area: %d | Circularity: %.2f\n",
                        bestGuessType == 0 ? 1 : (bestGuessType == 1 ? 2 : 5), 
                        diameter, copperBlobs[i].area, circularity);
                }
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            
            // Regular classification with adaptive tolerance
            if (diameter >= D_1CENT * (1.0f - tolerance) && diameter <= D_1CENT * (1.0f + tolerance)) {
                // 1 cent (index 0)
                // Only count if not already counted - passing 1 to mark as counted
                if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, 1, 1)) {
                    counters[0]++;
                    printf("[DEBUG] COIN COUNTED: 1 cent (COPPER) | Diam: %.1f | Area: %d | Circ: %.2f | Total now: %d\n",
                           diameter, copperBlobs[i].area, circularity, counters[0]);
                } else {
                    printf("[DEBUG] Detected (already counted): 1 cent at (%d,%d)\n", 
                           copperBlobs[i].xc, copperBlobs[i].yc);
                }
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            else if (diameter >= D_2CENT * (1.0f - tolerance) && diameter <= D_2CENT * (1.0f + tolerance)) {
                // 2 cents (index 1)
                // Only count if not already counted - passing 1 to mark as counted
                if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, 2, 1)) {
                    counters[1]++;
                    printf("[DEBUG] COIN COUNTED: 2 cents (COPPER) | Diam: %.1f | Area: %d | Circ: %.2f | Total now: %d\n",
                           diameter, copperBlobs[i].area, circularity, counters[1]);
                } else {
                    printf("[DEBUG] Detected (already counted): 2 cents at (%d,%d)\n", 
                           copperBlobs[i].xc, copperBlobs[i].yc);
                }
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            else if (diameter >= D_5CENT * (1.0f - tolerance) && diameter <= D_5CENT * (1.0f + tolerance)) {
                // 5 cents (index 2)
                // Only count if not already counted - passing 1 to mark as counted
                if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, 3, 1)) {
                    counters[2]++;
                    printf("[DEBUG] COIN COUNTED: 5 cents (COPPER) | Diam: %.1f | Area: %d | Circ: %.2f | Total now: %d\n",
                           diameter, copperBlobs[i].area, circularity, counters[2]);
                } else {
                    printf("[DEBUG] Detected (already counted): 5 cents at (%d,%d)\n", 
                           copperBlobs[i].xc, copperBlobs[i].yc);
                }
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief Improved gold coin detection with adaptive tolerance and better edge handling
 */
bool DetectGoldCoinsImproved(OVC *blob, OVC *goldBlobs, int ngoldBlobs, 
                           int *excludeList, int *counters, int distThresholdSq) {
    if (!blob || !goldBlobs || ngoldBlobs <= 0)
        return false;
    
    // Frame dimensions for adaptive tolerance
    const int frameWidth = 640;   // Assumed frame width
    const int frameHeight = 480;  // Assumed frame height
    
    // Edge proximity check parameter
    const int EDGE_MARGIN = 90; // Half the diameter of the largest gold coin + margin
    
    for (int i = 0; i < ngoldBlobs; i++) {
        // Verify blob validity
        if (goldBlobs[i].label == 0 || goldBlobs[i].area < 7000)
            continue;
            
        // Check if centers are close
        int dx = goldBlobs[i].xc - blob->xc;
        int dy = goldBlobs[i].yc - blob->yc;
        int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            // Found nearby gold blob
            float diameter = CalculateCircularDiameter(&goldBlobs[i]);
            float circularity = CalculateCircularity(&goldBlobs[i]);
            
            // Additional circularity check for gold coins (more strict)
            if (circularity < 0.8f) {
                continue; // Skip non-circular objects
            }
            
            // Calculate adaptive tolerance based on position in frame
            float tolerance = CalculateAdaptiveTolerance(goldBlobs[i].xc, goldBlobs[i].yc, 
                                                        frameWidth, frameHeight);
            
            // Check if blob is near the edge of the frame
            bool isNearEdge = false;
            if (goldBlobs[i].xc < EDGE_MARGIN || goldBlobs[i].yc < EDGE_MARGIN || 
                goldBlobs[i].xc > frameWidth - EDGE_MARGIN || goldBlobs[i].yc > frameHeight - EDGE_MARGIN) {
                isNearEdge = true;
                
                // For edge coins, try to maintain previous frame's classification
                int coinType = GetLastCoinTypeAtLocation(goldBlobs[i].xc, goldBlobs[i].yc);
                
                // If we have a previous classification and it's a gold coin, use it
                if (coinType >= 4 && coinType <= 6) {
                    // Only count if not already counted
                    if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, coinType, 1)) {
                        counters[coinType - 1]++;
                        printf("COIN COUNTED: %d cents (GOLD contour) | Diam: %.1f | Area: %d | Circularity: %.2f\n",
                            coinType == 4 ? 10 : (coinType == 5 ? 20 : 50), 
                            diameter, goldBlobs[i].area, circularity);
                    }
                    ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                    return true;
                }
                
                // If no previous classification, make a best guess based on diameter ratio
                float size10CentRatio = diameter / D_10CENT;
                float size20CentRatio = diameter / D_20CENT;
                float size50CentRatio = diameter / D_50CENT;
                
                int bestGuessType;
                // Find closest match by comparing ratios to 1.0
                float diff10Cent = fabsf(size10CentRatio - 1.0f);
                float diff20Cent = fabsf(size20CentRatio - 1.0f);
                float diff50Cent = fabsf(size50CentRatio - 1.0f);
                
                if (diff10Cent < diff20Cent && diff10Cent < diff50Cent) {
                    bestGuessType = 3; // 10c
                } else if (diff20Cent < diff10Cent && diff20Cent < diff50Cent) {
                    bestGuessType = 4; // 20c
                } else {
                    bestGuessType = 5; // 50c
                }
                
                // Only count if not already counted
                if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, bestGuessType + 1, 1)) {
                    counters[bestGuessType]++;
                    printf("COIN COUNTED: %d cents (GOLD contour) | Diam: %.1f | Area: %d | Circularity: %.2f\n",
                        bestGuessType == 3 ? 10 : (bestGuessType == 4 ? 20 : 50), 
                        diameter, goldBlobs[i].area, circularity);
                }
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            
            // Regular classification with adaptive tolerance
            if (diameter >= D_10CENT * (1.0f - tolerance) && diameter <= D_10CENT * (1.0f + tolerance)) {
                // 10 cents (index 3)
                // Only count if not already counted - passing 1 to mark as counted
                if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, 4, 1)) {
                    counters[3]++;
                    printf("[DEBUG] COIN COUNTED: 10 cents (GOLD) | Diam: %.1f | Area: %d | Circ: %.2f | Total now: %d\n",
                           diameter, goldBlobs[i].area, circularity, counters[3]);
                } else {
                    printf("[DEBUG] Detected (already counted): 10 cents at (%d,%d)\n", 
                           goldBlobs[i].xc, goldBlobs[i].yc);
                }
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            else if (diameter >= D_20CENT * (1.0f - tolerance) && diameter <= D_20CENT * (1.0f + tolerance)) {
                // 20 cents (index 4)
                // Only count if not already counted - passing 1 to mark as counted
                if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, 5, 1)) {
                    counters[4]++;
                    printf("[DEBUG] COIN COUNTED: 20 cents (GOLD) | Diam: %.1f | Area: %d | Circ: %.2f | Total now: %d\n",
                           diameter, goldBlobs[i].area, circularity, counters[4]);
                } else {
                    printf("[DEBUG] Detected (already counted): 20 cents at (%d,%d)\n", 
                           goldBlobs[i].xc, goldBlobs[i].yc);
                }
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            else if (diameter >= D_50CENT * (1.0f - tolerance) && diameter <= D_50CENT * (1.0f + tolerance)) {
                // 50 cents (index 5)
                // Only count if not already counted - passing 1 to mark as counted
                if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, 6, 1)) {
                    counters[5]++;
                    printf("[DEBUG] COIN COUNTED: 50 cents (GOLD) | Diam: %.1f | Area: %d | Circ: %.2f | Total now: %d\n",
                           diameter, goldBlobs[i].area, circularity, counters[5]);
                } else {
                    printf("[DEBUG] Detected (already counted): 50 cents at (%d,%d)\n", 
                           goldBlobs[i].xc, goldBlobs[i].yc);
                }
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
        }
    }
    
    return false;
}


#ifdef __cplusplus
}
#endif
