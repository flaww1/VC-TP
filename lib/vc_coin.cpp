/**
 * @file vc_coin.cpp
 * @brief Consolidated functions for coin detection and classification.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#include "vc.h"

#ifdef __cplusplus
extern "C" {
#endif

// Constants for coin diameters (in pixels) - renamed for consistency
const float DIAM_1CENT = 122.0f;
const float DIAM_2CENT = 135.0f;
const float DIAM_5CENT = 152.0f;
const float DIAM_10CENT = 143.0f;
const float DIAM_20CENT = 160.0f;
const float DIAM_50CENT = 174.0f;
const float DIAM_1EURO = 185.0f;
const float DIAM_2EURO = 195.0f;
const float BASE_TOLERANCE = 0.08f;

// Global variables for coin tracking
static int frameCountValue = 0;  // Renamed to avoid conflict
int MAX_TRACKED_COINS = 150;
int detectedCoins[150][5] = {{0}}; // [x, y, coinType, frameDetected, counted]

/**
 * @brief Increment or reset the frame counter
 */
void frameCounter(int reset) {
    if (reset) {
        frameCountValue = 0;
    } else {
        frameCountValue++;
        // Reset at 1000 to avoid overflow
        if (frameCountValue > 1000) frameCountValue = 0;
    }
}

/**
 * @brief Get the current frame count
 */
int getFrameCount() {
    return frameCountValue;
}

/**
 * @brief Calculate the circularity of a blob
 * 
 * @details Centralized implementation of the circularity calculation
 * that is used across the entire project
 */
float getCircularity(OVC *blob) {
    if (blob->perimeter <= 0)
        return 0.0f;

    static const float PI = 3.14159f;
    
    float circularity = (4.0f * PI * (float)blob->area) / 
                      ((float)(blob->perimeter * blob->perimeter));

    return (circularity > 1.0f) ? 1.0f : circularity;
}

/**
 * @brief Calculate the diameter of a circular blob
 */
float getDiameter(OVC *blob) {
    return 2.0f * sqrtf((float)blob->area / 3.14159f);
}

/**
 * @brief Calculate adaptive tolerance based on proximity to frame edge
 */
float adaptTolerance(int xc, int yc, int frameWidth, int frameHeight) {
    float tolerance = BASE_TOLERANCE;
    const int edgeMargin = 50;
    
    // Find minimum distance to any edge
    float minDist = fminf(fminf((float)xc, (float)(frameWidth - xc)), 
                         fminf((float)yc, (float)(frameHeight - yc)));
    
    // Increase tolerance near edges
    if (minDist < edgeMargin)
        tolerance *= 1.0f + 0.5f * (1.0f - (minDist / edgeMargin));
    
    return tolerance;
}

/**
 * @brief Track if a coin has been detected already
 */
int trackCoin(int x, int y, int coinType, int countIt) {
    const int distThreshold = (coinType >= 7) ? 75 : 50;
    const int distThresholdSq = distThreshold * distThreshold;
    const int frameMemory = (coinType >= 7) ? 120 : 60;
    const int currentFrame = getFrameCount();
    
    int existingIndex = -1;
    int emptyIndex = -1;
    
    // Handle Euro coins replacing gold coins
    if (coinType >= 7) {
        for (int i = 0; i < MAX_TRACKED_COINS; i++) {
            if (detectedCoins[i][0] == 0 && detectedCoins[i][1] == 0)
                continue;
                
            if (detectedCoins[i][2] >= 4 && detectedCoins[i][2] <= 6) {
                const int dx = detectedCoins[i][0] - x;
                const int dy = detectedCoins[i][1] - y;
                const int distSq = dx*dx + dy*dy;
                
                if (distSq <= 85*85) {
                    memset(&detectedCoins[i][0], 0, 5 * sizeof(int));
                    break;
                }
            }
        }
    }
    
    // Find existing or empty slot
    for (int i = 0; i < MAX_TRACKED_COINS; i++) {
        if (detectedCoins[i][0] == 0 && detectedCoins[i][1] == 0) {
            if (emptyIndex == -1) emptyIndex = i;
            continue;
        }
        
        const int dx = detectedCoins[i][0] - x;
        const int dy = detectedCoins[i][1] - y;
        const int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            // Valid frame window
            if ((currentFrame - detectedCoins[i][3] < frameMemory) || 
                detectedCoins[i][3] > currentFrame) {
                
                existingIndex = i;
                
                // Handle Euro replacing gold
                if (coinType >= 7 && detectedCoins[i][2] >= 4 && detectedCoins[i][2] <= 6) {
                    detectedCoins[i][2] = coinType;
                }
                
                break;
            }
        }
    }
    
    // Update existing or add new entry
    if (existingIndex >= 0) {
        // Update position and timestamp
        detectedCoins[existingIndex][0] = x;
        detectedCoins[existingIndex][1] = y;
        detectedCoins[existingIndex][3] = currentFrame;
        
        // Handle counting
        if (countIt && detectedCoins[existingIndex][4] == 0) {
            detectedCoins[existingIndex][4] = 1;
            return 0;
        }
        
        return detectedCoins[existingIndex][4];
    }
    
    // Add new entry if possible
    if (emptyIndex >= 0) {
        detectedCoins[emptyIndex][0] = x;
        detectedCoins[emptyIndex][1] = y;
        detectedCoins[emptyIndex][2] = coinType;
        detectedCoins[emptyIndex][3] = currentFrame;
        detectedCoins[emptyIndex][4] = countIt ? 1 : 0;
    }
    
    return 0;
}

/**
 * @brief Get last detected coin type at a location
 */
int getCoinTypeAtLocation(int x, int y) {
    const int distThresholdSq = 50*50;
    int nearestType = 0;
    int nearestDistSq = INT_MAX;
    
    for (int i = 0; i < MAX_TRACKED_COINS; i++) {
        if (detectedCoins[i][0] == 0 && detectedCoins[i][1] == 0)
            continue;
        
        int dx = detectedCoins[i][0] - x;
        int dy = detectedCoins[i][1] - y;
        int distSq = dx*dx + dy*dy;
        
        if (distSq < nearestDistSq && distSq <= distThresholdSq) {
            nearestDistSq = distSq;
            nearestType = detectedCoins[i][2];
        }
    }
    
    return nearestType;
}

/**
 * @brief Add or remove a coin from exclusion list
 */
int excludeCoin(int *excludeList, int xc, int yc, int option) {
    if (!excludeList) return 0;
    
    const int PROXIMITY_THRESHOLD_SQ = 30 * 30;
    
    if (option == 0) {
        // Add to exclusion list
        for (int i = 0; i < MAX_COINS; i++) {
            if (excludeList[i * 2] == 0 && excludeList[i * 2 + 1] == 0) {
                excludeList[i * 2] = xc;
                excludeList[i * 2 + 1] = yc;
                break;
            }
        }
    }
    else if (option == 1) {
        // Remove from exclusion list
        for (int i = 0; i < MAX_COINS; i++) {
            if (excludeList[i * 2] == 0 && excludeList[i * 2 + 1] == 0) {
                continue;
            }
            
            int dx = excludeList[i * 2] - xc;
            int dy = excludeList[i * 2 + 1] - yc;
            int distSq = dx*dx + dy*dy;
            
            if (distSq <= PROXIMITY_THRESHOLD_SQ) {
                excludeList[i * 2] = 0;
                excludeList[i * 2 + 1] = 0;
            }
        }
    }
    
    return 0;
}

/**
 * @brief Correct misidentified gold coins when Euro coins are detected
 */
void correctGoldCoins(int x, int y, int *counters) {
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

/**
 * @brief Draw coins with labels on the frame
 */
void drawCoins(IVC *frame, OVC *goldBlobs, OVC *copperBlobs, OVC *euroBlobs,
              int nGoldBlobs, int nCopperBlobs, int nEuroBlobs) {
    unsigned char *data = (unsigned char*)frame->data;
    const int bytesperline = frame->bytesperline;
    const int channels = frame->channels;
    const int width = frame->width;
    const int height = frame->height;

    // Draw Euro coins first (they have priority)
    if (euroBlobs && nEuroBlobs > 0) {
        // Find best Euro blob in one pass
        OVC bestEuro = {0};
        bool foundValidEuro = false;
        
        float bestCircularity = 0.0f;
        int bestArea = 0;
        
        // Constants
        const int MAX_VALID_AREA = 100000;
        const int MIN_VALID_AREA = 12000;
        
        // Find complete Euro coins first
        for (int i = 0; i < nEuroBlobs; i++) {
            if (euroBlobs[i].area > MAX_VALID_AREA)
                continue;
                
            const float diameter = getDiameter(&euroBlobs[i]);
            const float circularity = getCircularity(&euroBlobs[i]);
            
            // Complete Euro detection
            if (diameter >= 175.0f && diameter <= 210.0f && circularity >= 0.75f) {
                // Skip if this is a gold coin position
                const int lastType = getCoinTypeAtLocation(euroBlobs[i].xc, euroBlobs[i].yc);
                if (lastType >= 4 && lastType <= 6)
                    continue;
                    
                memcpy(&bestEuro, &euroBlobs[i], sizeof(OVC));
                bestEuro.label = 999;
                foundValidEuro = true;
                break; // First match is sufficient
            }
        }
        
        // If no complete coins found, look for valid parts
        if (!foundValidEuro) {
            for (int i = 0; i < nEuroBlobs; i++) {
                if (euroBlobs[i].area < MIN_VALID_AREA || euroBlobs[i].area > MAX_VALID_AREA)
                    continue;
                    
                const float circularity = getCircularity(&euroBlobs[i]);
                
                if (circularity >= 0.65f && 
                    euroBlobs[i].width >= 130 && euroBlobs[i].height >= 130 &&
                    euroBlobs[i].area > bestArea) {
                    
                    // Skip if this is a gold coin position
                    const int lastType = getCoinTypeAtLocation(euroBlobs[i].xc, euroBlobs[i].yc);
                    if (lastType >= 4 && lastType <= 6)
                        continue;
                        
                    memcpy(&bestEuro, &euroBlobs[i], sizeof(OVC));
                    bestEuro.label = 999;
                    bestArea = euroBlobs[i].area;
                    foundValidEuro = true;
                }
            }
        }
        
        // Draw Euro coins if found
        if (foundValidEuro) {
            const float diameter = getDiameter(&bestEuro);
            const int radius = (int)(diameter / 2.0f);
            const int centerX = bestEuro.xc;
            const int centerY = bestEuro.yc;
            
            // Skip if out of bounds
            if (centerX < 0 || centerX >= width || centerY < 0 || centerY >= height)
                return;
                
            // Draw circle for Euro coin
            const unsigned char blueColor[3] = {255, 130, 0}; // Blue in BGR
            
            // Draw simple circle
            for (int angle = 0; angle < 360; angle++) {
                float radAngle = angle * (M_PI / 180.0f);
                int x = (int)(centerX + radius * cosf(radAngle));
                int y = (int)(centerY + radius * sinf(radAngle));
                
                // Check boundaries
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    int pos = y * bytesperline + x * channels;
                    data[pos] = blueColor[0];     // B
                    data[pos + 1] = blueColor[1]; // G
                    data[pos + 2] = blueColor[2]; // R
                }
            }
            
            // Draw center dot and labels
            bool is2Euro = (diameter >= 185.0f || bestEuro.area >= 14000);
            
            // Draw a simple center dot
            const int dotRadius = 3;
            for (int y = centerY - dotRadius; y <= centerY + dotRadius; y++) {
                for (int x = centerX - dotRadius; x <= centerX + dotRadius; x++) {
                    if (x >= 0 && x < width && y >= 0 && y < height) {
                        int pos = y * bytesperline + x * channels;
                        data[pos] = 255;     // B
                        data[pos + 1] = 255; // G
                        data[pos + 2] = 255; // R
                    }
                }
            }
            
            // Draw a simplified label
            int textX = centerX - 10;
            int textY = centerY + 30;
            if (textX >= 0 && textX < width - 20 && textY >= 0 && textY < height) {
                // Draw a black background for text
                for (int y = textY - 10; y <= textY + 10; y++) {
                    for (int x = textX - 10; x <= textX + 20; x++) {
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            int pos = y * bytesperline + x * channels;
                            data[pos] = 0;
                            data[pos + 1] = 0;
                            data[pos + 2] = 0;
                        }
                    }
                }
                
                // Draw white text (simplified version)
                for (int y = textY - 5; y <= textY + 5; y++) {
                    for (int x = textX; x <= textX + (is2Euro ? 15 : 10); x++) {
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            int pos = y * bytesperline + x * channels;
                            data[pos] = 255;
                            data[pos + 1] = 255; // G
                            data[pos + 2] = 255; // R
                        }
                    }
                }
            }
        }
    }
    
    // Draw copper coins
    if (copperBlobs && nCopperBlobs > 0) {
        for (int i = 0; i < nCopperBlobs; i++) {
            if (copperBlobs[i].area < 7000 || copperBlobs[i].label == 0)
                continue;
            
            const float diameter = getDiameter(&copperBlobs[i]);
            const int radius = (int)(diameter / 2.0f);
            const int centerX = copperBlobs[i].xc;
            const int centerY = copperBlobs[i].yc;
            
            // Skip if out of bounds
            if (centerX < 0 || centerX >= width || centerY < 0 || centerY >= height)
                continue;
                
            // Draw circle for copper coin
            const unsigned char copperColor[3] = {0, 80, 255}; // Orange/Copper in BGR
            
            // Draw simple circle
            for (int angle = 0; angle < 360; angle += 3) { // Step by 3 degrees for speed
                float radAngle = angle * (M_PI / 180.0f);
                int x = (int)(centerX + radius * cosf(radAngle));
                int y = (int)(centerY + radius * sinf(radAngle));
                
                // Check boundaries
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    int pos = y * bytesperline + x * channels;
                    data[pos] = copperColor[0];     // B
                    data[pos + 1] = copperColor[1]; // G
                    data[pos + 2] = copperColor[2]; // R
                }
            }
            
            // Mark center
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int x = centerX + dx;
                    int y = centerY + dy;
                    if (x >= 0 && x < width && y >= 0 && y < height) {
                        int pos = y * bytesperline + x * channels;
                        data[pos] = 255;     // B
                        data[pos + 1] = 255; // G
                        data[pos + 2] = 255; // R
                    }
                }
            }
            
            // Add a simplified label
            const char *label = (diameter < 130) ? "1c" : 
                              (diameter < 145) ? "2c" : "5c";
            
            // Very basic text rendering at the center
            int textX = centerX - 5;
            int textY = centerY + 20;
            if (textX >= 0 && textX < width - 10 && textY >= 0 && textY < height) {
                // Create a small black background
                for (int dy = -5; dy <= 5; dy++) {
                    for (int dx = -5; dx <= 10; dx++) {
                        int x = textX + dx;
                        int y = textY + dy;
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            int pos = y * bytesperline + x * channels;
                            data[pos] = 0;     // B
                            data[pos + 1] = 0; // G
                            data[pos + 2] = 0; // R
                        }
                    }
                }
                
                // Draw white text (very simplified)
                for (int dy = -4; dy <= 4; dy++) {
                    for (int dx = -4; dx <= 9; dx++) {
                        int x = textX + dx;
                        int y = textY + dy;
                        if ((dx == -4 || dx == 9 || dy == -4 || dy == 4) && 
                            x >= 0 && x < width && y >= 0 && y < height) {
                            int pos = y * bytesperline + x * channels;
                            data[pos] = 255;     // B
                            data[pos + 1] = 255; // G
                            data[pos + 2] = 255; // R
                        }
                    }
                }
            }
        }
    }
    
    // Draw gold coins
    if (goldBlobs && nGoldBlobs > 0) {
        for (int i = 0; i < nGoldBlobs; i++) {
            if (goldBlobs[i].area < 7000 || goldBlobs[i].label == 0)
                continue;
            
            const float diameter = getDiameter(&goldBlobs[i]);
            const int radius = (int)(diameter / 2.0f);
            const int centerX = goldBlobs[i].xc;
            const int centerY = goldBlobs[i].yc;
            
            // Skip if out of bounds
            if (centerX < 0 || centerX >= width || centerY < 0 || centerY >= height)
                continue;
                
            // Draw circle for gold coin
            const unsigned char goldColor[3] = {0, 215, 255}; // Yellow/Gold in BGR
            
            // Draw simplified circle
            for (int angle = 0; angle < 360; angle += 3) {
                float radAngle = angle * (M_PI / 180.0f);
                int x = (int)(centerX + radius * cosf(radAngle));
                int y = (int)(centerY + radius * sinf(radAngle));
                
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    int pos = y * bytesperline + x * channels;
                    data[pos] = goldColor[0];
                    data[pos + 1] = goldColor[1];
                    data[pos + 2] = goldColor[2];
                }
            }
            
            // Mark center
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int x = centerX + dx;
                    int y = centerY + dy;
                    if (x >= 0 && x < width && y >= 0 && y < height) {
                        int pos = y * bytesperline + x * channels;
                        data[pos] = 255;
                        data[pos + 1] = 255;
                        data[pos + 2] = 255;
                    }
                }
            }
            
            // Simple label based on size
            const char *label = (diameter < 150) ? "10c" : 
                              (diameter < 170) ? "20c" : "50c";
            
            // Very basic text rendering
            int textX = centerX - 8;
            int textY = centerY + 20;
            if (textX >= 0 && textX < width - 15 && textY >= 0 && textY < height) {
                // Black background
                for (int dy = -5; dy <= 5; dy++) {
                    for (int dx = -5; dx <= 15; dx++) {
                        int x = textX + dx;
                        int y = textY + dy;
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            int pos = y * bytesperline + x * channels;
                            data[pos] = 0;
                            data[pos + 1] = 0;
                            data[pos + 2] = 0;
                        }
                    }
                }
                
                // White text outline
                for (int dy = -4; dy <= 4; dy++) {
                    for (int dx = -4; dx <= 14; dx++) {
                        int x = textX + dx;
                        int y = textY + dy;
                        if ((dx == -4 || dx == 14 || dy == -4 || dy == 4) &&
                            x >= 0 && x < width && y >= 0 && y < height) {
                            int pos = y * bytesperline + x * channels;
                            data[pos] = 255;
                            data[pos + 1] = 255;
                            data[pos + 2] = 255;
                        }
                    }
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
