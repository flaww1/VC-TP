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

// These are declared in vc_coin_utils.cpp and need to be declared as extern here
extern int MAX_TRACKED_COINS;
extern int detectedCoins[][5];

#ifdef __cplusplus
extern "C"
{
#endif

// Constants for coin diameters (in pixels)
const float D_1CENT = 122.0f;
const float D_2CENT = 135.0f;
const float D_5CENT = 152.0f;
const float D_10CENT = 143.0f;
const float D_20CENT = 160.0f;
const float D_50CENT = 174.0f;
const float D_1EURO = 185.0f;
const float D_2EURO = 195.0f;
const float BASE_TOLERANCE = 0.08f;

// Maximum count of coins to track
#define MAX_COINS 50

// Simplified and optimized function to calculate adaptive tolerance
float CalculateAdaptiveTolerance(int xc, int yc, int frameWidth, int frameHeight) {
    float tolerance = BASE_TOLERANCE;
    const int edgeMargin = 50;
    
    // Find minimum distance to any edge with fewer operations
    float minDist = fminf(fminf((float)xc, (float)(frameWidth - xc)), 
                         fminf((float)yc, (float)(frameHeight - yc)));
    
    // Only apply scaling if near edge (optimizes common case)
    if (minDist < edgeMargin)
        tolerance *= 1.0f + 0.5f * (1.0f - (minDist / edgeMargin));
    
    return tolerance;
}

// Optimized detect bronze coins function 
bool DetectBronzeCoinsImproved(OVC *blob, OVC *copperBlobs, int ncopperBlobs, 
                             int *excludeList, int *counters, int distThresholdSq) {
    if (!blob || !copperBlobs || ncopperBlobs <= 0)
        return false;
    
    const int frameWidth = 640;
    const int frameHeight = 480;
    const int EDGE_MARGIN = 80;
    
    // Precompute diameter ranges for efficient comparison
    const float MIN_VALID_AREA = 6000;
    const float MIN_VALID_CIRCULARITY = 0.70f;
    
    for (int i = 0; i < ncopperBlobs; i++) {
        // Skip invalid blobs quickly
        if (copperBlobs[i].label == 0 || copperBlobs[i].area < MIN_VALID_AREA)
            continue;
            
        // Check distance with fewer operations
        const int dx = copperBlobs[i].xc - blob->xc;
        const int dy = copperBlobs[i].yc - blob->yc;
        const int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            const float diameter = CalculateCircularDiameter(&copperBlobs[i]);
            const float circularity = CalculateCircularity(&copperBlobs[i]);
            
            // Skip non-circular objects
            if (circularity < MIN_VALID_CIRCULARITY)
                continue;
            
            const int correctedYC = copperBlobs[i].yc + (int)(diameter * 0.05f);
            const float tolerance = CalculateAdaptiveTolerance(copperBlobs[i].xc, 
                                                        copperBlobs[i].yc, 
                                                        frameWidth, frameHeight);
            
            // Check if near edge
            const bool isNearEdge = (copperBlobs[i].xc < EDGE_MARGIN || 
                                   copperBlobs[i].yc < EDGE_MARGIN || 
                                   copperBlobs[i].xc > frameWidth - EDGE_MARGIN || 
                                   copperBlobs[i].yc > frameHeight - EDGE_MARGIN);
            
            if (isNearEdge) {
                // Get last coin type at this location
                const int coinType = GetLastCoinTypeAtLocation(copperBlobs[i].xc, copperBlobs[i].yc);
                
                if (coinType >= 1 && coinType <= 3) {
                    // Skip counting if already detected
                    if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, coinType, 1))
                        counters[coinType - 1]++;
                        
                    ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                    return true;
                }
                
                // Calculate best match once
                const float size1Ratio = diameter / D_1CENT;
                const float size2Ratio = diameter / D_2CENT; 
                const float size5Ratio = diameter / D_5CENT;
                
                const float diff1 = fabsf(size1Ratio - 1.0f);
                const float diff2 = fabsf(size2Ratio - 1.0f);
                const float diff5 = fabsf(size5Ratio - 1.0f);
                
                int bestType = (diff1 < diff2 && diff1 < diff5) ? 0 : 
                              (diff2 < diff1 && diff2 < diff5) ? 1 : 2;
                
                if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, bestType + 1, 1))
                    counters[bestType]++;
                    
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            
            // Use precomputed ranges for faster comparison
            const float d1Lower = D_1CENT * (1.0f - tolerance);
            const float d1Upper = D_1CENT * (1.0f + tolerance);
            const float d2Lower = D_2CENT * (1.0f - tolerance);
            const float d2Upper = D_2CENT * (1.0f + tolerance);
            const float d5Lower = D_5CENT * (1.0f - tolerance);
            const float d5Upper = D_5CENT * (1.0f + tolerance);
            
            // Optimized conditions with simpler logic
            if (diameter >= d1Lower && diameter <= d1Upper) {
                if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, 1, 1)) {
                    counters[0]++;
                    printf("[COIN DETECTED] 1 cent | Value: 0.01€ | Diameter: %.1f | Area: %d | Circularity: %.2f\n",
                          diameter, copperBlobs[i].area, circularity);
                }
                
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            else if (diameter >= d2Lower && diameter <= d2Upper) {
                if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, 2, 1)) {
                    counters[1]++;
                    printf("[COIN DETECTED] 2 cents | Value: 0.02€ | Diameter: %.1f | Area: %d | Circularity: %.2f\n",
                          diameter, copperBlobs[i].area, circularity);
                }
                
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            else if (diameter >= d5Lower && diameter <= d5Upper) {
                if (!IsCoinAlreadyDetected(copperBlobs[i].xc, copperBlobs[i].yc, 3, 1)) {
                    counters[2]++;
                    printf("[COIN DETECTED] 5 cents | Value: 0.05€ | Diameter: %.1f | Area: %d | Circularity: %.2f\n",
                          diameter, copperBlobs[i].area, circularity);
                }
                
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
        }
    }
    
    return false;
}

// Complete implementation for DetectGoldCoinsImproved
bool DetectGoldCoinsImproved(OVC *blob, OVC *goldBlobs, int ngoldBlobs, 
                           int *excludeList, int *counters, int distThresholdSq) {
    if (!blob || !goldBlobs || ngoldBlobs <= 0)
        return false;
    
    const int frameWidth = 640;
    const int frameHeight = 480;
    const int EDGE_MARGIN = 90; // Gold coins have larger edge margin
    
    // Precompute diameter ranges for efficient comparison
    const float MIN_VALID_AREA = 6000;
    const float MIN_VALID_CIRCULARITY = 0.75f; // Higher circularity for gold
    
    for (int i = 0; i < ngoldBlobs; i++) {
        // Skip invalid blobs quickly
        if (goldBlobs[i].label == 0 || goldBlobs[i].area < MIN_VALID_AREA)
            continue;
            
        // Check distance with fewer operations
        const int dx = goldBlobs[i].xc - blob->xc;
        const int dy = goldBlobs[i].yc - blob->yc;
        const int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            const float diameter = CalculateCircularDiameter(&goldBlobs[i]);
            const float circularity = CalculateCircularity(&goldBlobs[i]);
            
            // Skip non-circular objects
            if (circularity < MIN_VALID_CIRCULARITY)
                continue;
            
            const float tolerance = CalculateAdaptiveTolerance(goldBlobs[i].xc, 
                                                        goldBlobs[i].yc, 
                                                        frameWidth, frameHeight);
            
            // Check if near edge
            const bool isNearEdge = (goldBlobs[i].xc < EDGE_MARGIN || 
                                   goldBlobs[i].yc < EDGE_MARGIN || 
                                   goldBlobs[i].xc > frameWidth - EDGE_MARGIN || 
                                   goldBlobs[i].yc > frameHeight - EDGE_MARGIN);
            
            if (isNearEdge) {
                // Get last coin type at this location
                const int coinType = GetLastCoinTypeAtLocation(goldBlobs[i].xc, goldBlobs[i].yc);
                
                if (coinType >= 4 && coinType <= 6) {
                    // Skip counting if already detected
                    if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, coinType, 1))
                        counters[coinType - 1]++;
                        
                    ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                    return true;
                }
                
                // Calculate best match
                const float size10Ratio = diameter / D_10CENT;
                const float size20Ratio = diameter / D_20CENT; 
                const float size50Ratio = diameter / D_50CENT;
                
                const float diff10 = fabsf(size10Ratio - 1.0f);
                const float diff20 = fabsf(size20Ratio - 1.0f);
                const float diff50 = fabsf(size50Ratio - 1.0f);
                
                // Index offset for gold coins (3 = 10c, 4 = 20c, 5 = 50c)
                int bestType = (diff10 < diff20 && diff10 < diff50) ? 3 : 
                              (diff20 < diff10 && diff20 < diff50) ? 4 : 5;
                
                if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, bestType + 1, 1))
                    counters[bestType]++;
                    
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            
            // Use precomputed ranges for faster comparison
            const float d10Lower = D_10CENT * (1.0f - tolerance);
            const float d10Upper = D_10CENT * (1.0f + tolerance);
            const float d20Lower = D_20CENT * (1.0f - tolerance);
            const float d20Upper = D_20CENT * (1.0f + tolerance);
            const float d50Lower = D_50CENT * (1.0f - tolerance);
            const float d50Upper = D_50CENT * (1.0f + tolerance);
            
            // Optimized conditions with simpler logic
            if (diameter >= d10Lower && diameter <= d10Upper) {
                if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, 4, 1)) {
                    counters[3]++;
                    printf("[COIN DETECTED] 10 cents | Value: 0.10€ | Diameter: %.1f | Area: %d | Circularity: %.2f\n",
                          diameter, goldBlobs[i].area, circularity);
                }
                
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            else if (diameter >= d20Lower && diameter <= d20Upper) {
                if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, 5, 1)) {
                    counters[4]++;
                    printf("[COIN DETECTED] 20 cents | Value: 0.20€ | Diameter: %.1f | Area: %d | Circularity: %.2f\n",
                          diameter, goldBlobs[i].area, circularity);
                }
                
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            else if (diameter >= d50Lower && diameter <= d50Upper) {
                if (!IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, 6, 1)) {
                    counters[5]++;
                    printf("[COIN DETECTED] 50 cents | Value: 0.50€ | Diameter: %.1f | Area: %d | Circularity: %.2f\n",
                          diameter, goldBlobs[i].area, circularity);
                }
                
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
        }
    }
    
    return false;
}

// New utility function to reduce duplicated code
void CorrectMisidentifiedGoldCoins(int x, int y, int *counters) {
    const int distThresholdSq = 80*80;
    
    for (int i = 0; i < MAX_TRACKED_COINS; i++) {
        if (detectedCoins[i][0] == 0 && detectedCoins[i][1] == 0) 
            continue;
            
        const int dx = detectedCoins[i][0] - x;
        const int dy = detectedCoins[i][1] - y;
        const int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq && detectedCoins[i][2] >= 4 && detectedCoins[i][2] <= 6) {
            const int goldType = detectedCoins[i][2];
            const int goldCounterIdx = goldType - 1;
            
            // Decrease counter if necessary
            if (counters[goldCounterIdx] > 0)
                counters[goldCounterIdx]--;
                
            // Clear entry
            memset(&detectedCoins[i][0], 0, 5 * sizeof(int));
            break;
        }
    }
}

// Streamlined Euro coin detection with fewer debug prints
bool DetectEuroCoinsImproved(OVC *blob, OVC *euroBlobs, int neuroBlobs, 
                           int *excludeList, int *counters, int distThresholdSq) {
    if (!blob || !euroBlobs || neuroBlobs <= 0)
        return false;
    
    // Constants
    const int MAX_VALID_AREA = 100000;  
    const int MIN_VALID_AREA = 6000;
    
    // Track best candidates
    int bestCompleteIndex = -1;
    float bestCompleteDiameter = 0.0f;
    float bestCompleteCircularity = 0.0f;
    
    int bestPartialIndex = -1;
    float bestPartialDiameter = 0.0f;
    int bestPartialArea = 0;
    
    // Single-pass to find best candidates
    for (int i = 0; i < neuroBlobs; i++) {
        // Quick area check
        if (euroBlobs[i].area < MIN_VALID_AREA || euroBlobs[i].area > MAX_VALID_AREA)
            continue;

        const float diameter = CalculateCircularDiameter(&euroBlobs[i]);
        const float circularity = CalculateCircularity(&euroBlobs[i]);
        
        // Complete Euro coins
        if (diameter >= 175.0f && diameter <= 210.0f && circularity > 0.75f) {
            if (bestCompleteIndex == -1 || circularity > bestCompleteCircularity) {
                bestCompleteIndex = i;
                bestCompleteDiameter = diameter;
                bestCompleteCircularity = circularity;
            }
        }
        // Partial Euro coins
        else if (circularity > 0.65f && euroBlobs[i].width >= 130 && euroBlobs[i].height >= 130) {
            if (bestPartialIndex == -1 || euroBlobs[i].area > bestPartialArea) {
                bestPartialIndex = i;
                bestPartialDiameter = diameter;
                bestPartialArea = euroBlobs[i].area;
            }
        }
    }
    
    // Process complete Euro coin first
    if (bestCompleteIndex >= 0) {
        // Determine Euro type
        const bool is2Euro = (bestCompleteDiameter >= 185.0f);
        const int coinType = is2Euro ? 8 : 7;
        const int counterIdx = is2Euro ? 7 : 6;
        
        // Correct any misidentified gold coins
        CorrectMisidentifiedGoldCoins(euroBlobs[bestCompleteIndex].xc, 
                                      euroBlobs[bestCompleteIndex].yc, 
                                      counters);
        
        // Count if not already counted
        if (!IsCoinAlreadyDetected(euroBlobs[bestCompleteIndex].xc, 
                                euroBlobs[bestCompleteIndex].yc, 
                                coinType, 1)) {
            counters[counterIdx]++;
            
            // Log detailed coin information
            const float diameter = bestCompleteDiameter;
            const float circularity = bestCompleteCircularity;
            const int area = euroBlobs[bestCompleteIndex].area;
            
            if (is2Euro) {
                printf("[COIN DETECTED] 2 Euros | Value: 2.00€ | Diameter: %.1f | Area: %d | Circularity: %.2f\n",
                      diameter, area, circularity);
            } else {
                printf("[COIN DETECTED] 1 Euro | Value: 1.00€ | Diameter: %.1f | Area: %d | Circularity: %.2f\n",
                      diameter, area, circularity);
            }
        }
        
        ExcludeCoin(excludeList, euroBlobs[bestCompleteIndex].xc, 
                  euroBlobs[bestCompleteIndex].yc, 0);
        return true;
    }
    
    // Process partial Euro coin if significant
    else if (bestPartialIndex >= 0 && bestPartialArea >= 14000) {
        const int coinType = 8;  // Always 2€ for video
        const int counterIdx = 7;
        
        // Correct any misidentified gold coins
        CorrectMisidentifiedGoldCoins(euroBlobs[bestPartialIndex].xc, 
                                     euroBlobs[bestPartialIndex].yc, 
                                     counters);
        
        // Count if not already counted
        if (!IsCoinAlreadyDetected(euroBlobs[bestPartialIndex].xc, 
                                euroBlobs[bestPartialIndex].yc, 
                                coinType, 1)) {
            counters[counterIdx]++;
            
            // Log detailed coin information
            printf("[COIN DETECTED] 2 Euros (partial) | Value: 2.00€ | Diameter: %.1f | Area: %d | Circularity: %.2f\n",
                  bestPartialDiameter, bestPartialArea, CalculateCircularity(&euroBlobs[bestPartialIndex]));
        }
        
        ExcludeCoin(excludeList, euroBlobs[bestPartialIndex].xc, 
                  euroBlobs[bestPartialIndex].yc, 0);
        return true;
    }
    
    return false;
}

#ifdef __cplusplus
}
#endif
