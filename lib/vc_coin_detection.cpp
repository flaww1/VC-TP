/**
 * @file vc_coin_detection.cpp
 * @brief Functions specifically for coin detection and classification.
 *
 * This file contains specialized functions to detect and classify
 * different types of coins (bronze and gold).
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

// Include all headers before any extern "C" blocks
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

/**
 * @brief Detecta moedas douradas - improved version with better edge detection
 */
bool DetectGoldCoins(OVC *blob, OVC *goldBlobs, int ngoldBlobs, int *excludeList, int *counters, int distThresholdSq)
{
    if (!blob || !goldBlobs || ngoldBlobs <= 0)
        return false;
        
    // Dimensões esperadas das moedas douradas (diâmetros em pixels)
    const float D_50CENT = 174.0f;
    const float D_20CENT = 160.0f;
    const float D_10CENT = 143.0f;
    const float TOLERANCE = 0.08f;
    
    // Edge proximity check parameter - distance from blob center to frame edge
    const int EDGE_MARGIN = 90; // Half the diameter of the largest coin + margin
    
    for (int i = 0; i < ngoldBlobs; i++) {
        // Verificar se é um blob válido
        if (goldBlobs[i].label == 0 || goldBlobs[i].area < 7000)
            continue;
            
        // Verificar se os centros estão próximos
        int dx = goldBlobs[i].xc - blob->xc;
        int dy = goldBlobs[i].yc - blob->yc;
        int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            // Encontrou blob de ouro próximo
            float diameter = CalculateCircularDiameter(&goldBlobs[i]);
            
            // Check if blob is near the edge of the frame - same for all gold coins
            bool isNearEdge = false;
            if (goldBlobs[i].xc < EDGE_MARGIN || goldBlobs[i].yc < EDGE_MARGIN || 
                goldBlobs[i].xc > 640 - EDGE_MARGIN || goldBlobs[i].yc > 480 - EDGE_MARGIN) {
                isNearEdge = true;
                
                // For edge coins, try to maintain previous frame's classification
                int coinType = GetLastCoinTypeAtLocation(goldBlobs[i].xc, goldBlobs[i].yc);
                
                // If we have a previous classification and it's a gold coin, use it
                if (coinType >= 4 && coinType <= 6) {
                    counters[coinType - 1]++;
                    ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                    return true;
                }
                
                // If no previous classification, make a best guess based on diameter
                int bestGuessType;
                if (diameter <= D_10CENT * 1.15f) {
                    bestGuessType = 3; // 10c
                } else if (diameter <= D_20CENT * 1.15f) {
                    bestGuessType = 4; // 20c
                } else {
                    bestGuessType = 5; // 50c
                }
                
                counters[bestGuessType]++;
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            
            // Regular classification for non-edge coins - use more precise bounds
            if (diameter >= D_10CENT * 0.92f && diameter <= D_10CENT * 1.08f) {
                // 10 cêntimos (índice 3)
                counters[3]++;
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                printf("NOVA MOEDA: 10 centimos (contorno DOURADO) | Diâm: %.1f | Área: %d\n",
                    diameter, goldBlobs[i].area);
                return true;
            }
            else if (diameter >= D_20CENT * 0.92f && diameter <= D_20CENT * 1.08f) {
                // 20 cêntimos (índice 4)
                counters[4]++;
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                printf("NOVA MOEDA: 20 centimos (contorno DOURADO) | Diâm: %.1f | Área: %d\n",
                    diameter, goldBlobs[i].area);
                return true;
            }
            else if (diameter >= D_50CENT * 0.92f && diameter <= D_50CENT * 1.08f) {
                // 50 cêntimos (índice 5)
                counters[5]++;
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                printf("NOVA MOEDA: 50 centimos (contorno DOURADO) | Diâm: %.1f | Área: %d\n",
                    diameter, goldBlobs[i].area);
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief Detecta moedas de bronze/cobre - improved with better edge detection
 */
bool DetectBronzeCoins(OVC *blob, OVC *copperBlobs, int ncopperBlobs, int *excludeList, int *counters, int distThresholdSq)
{
    if (!blob || !copperBlobs || ncopperBlobs <= 0)
        return false;
        
    // Dimensões esperadas das moedas de cobre (diâmetros em pixels)
    const float D_5CENT = 152.0f;
    const float D_2CENT = 135.0f;
    const float D_1CENT = 122.0f;
    const float TOLERANCE = 0.08f;
    
    // Edge proximity check parameter - distance from blob center to frame edge
    const int EDGE_MARGIN = 80; // Half the diameter of the largest copper coin + margin
    
    for (int i = 0; i < ncopperBlobs; i++) {
        // Verificar se é um blob válido
        if (copperBlobs[i].label == 0 || copperBlobs[i].area < 7000)
            continue;
            
        // Verificar se os centros estão próximos
        int dx = copperBlobs[i].xc - blob->xc;
        int dy = copperBlobs[i].yc - blob->yc;
        int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            // Encontrou blob de cobre próximo
            float diameter = CalculateCircularDiameter(&copperBlobs[i]);
            
            // Shadow correction
            int correctedYC = copperBlobs[i].yc + (int)(diameter * 0.05f);
            
            // Check if blob is near the edge of the frame - same for all copper coins
            bool isNearEdge = false;
            if (copperBlobs[i].xc < EDGE_MARGIN || copperBlobs[i].yc < EDGE_MARGIN || 
                copperBlobs[i].xc > 640 - EDGE_MARGIN || copperBlobs[i].yc > 480 - EDGE_MARGIN) {
                isNearEdge = true;
                
                // For edge coins, try to maintain previous frame's classification
                int coinType = GetLastCoinTypeAtLocation(copperBlobs[i].xc, copperBlobs[i].yc);
                
                // If we have a previous classification and it's a copper coin, use it
                if (coinType >= 1 && coinType <= 3) {
                    counters[coinType - 1]++;
                    ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                    return true;
                }
                
                // If no previous classification, make a best guess based on diameter
                int bestGuessType;
                if (diameter <= D_1CENT * 1.15f) {
                    bestGuessType = 0; // 1c
                } else if (diameter <= D_2CENT * 1.15f) {
                    bestGuessType = 1; // 2c
                } else {
                    bestGuessType = 2; // 5c
                }
                
                counters[bestGuessType]++;
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            
            // Regular classification for non-edge coins - use more precise bounds
            if (diameter >= D_1CENT * 0.92f && diameter <= D_1CENT * 1.08f) {
                // 1 cêntimo (índice 0)
                counters[0]++;
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                printf("NOVA MOEDA: 1 centimo (contorno COBRE) | Diâm: %.1f | Área: %d\n",
                    diameter, copperBlobs[i].area);
                return true;
            }
            else if (diameter >= D_2CENT * 0.92f && diameter <= D_2CENT * 1.08f) {
                // 2 cêntimos (índice 1)
                counters[1]++;
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                printf("NOVA MOEDA: 2 centimos (contorno COBRE) | Diâm: %.1f | Área: %d\n",
                    diameter, copperBlobs[i].area);
                return true;
            }
            else if (diameter >= D_5CENT * 0.92f && diameter <= D_5CENT * 1.08f) {
                // 5 cêntimos (índice 2)
                counters[2]++;
                ExcludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                printf("NOVA MOEDA: 5 centimos (contorno COBRE) | Diâm: %.1f | Área: %d\n",
                    diameter, copperBlobs[i].area);
                return true;
            }
        }
    }
    
    return false;
}

#ifdef __cplusplus
}
#endif
