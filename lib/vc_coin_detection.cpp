/**
 * @file vc_coin_detection.cpp
 * @brief Functions specifically for coin detection and classification.
 *
 * This file contains specialized functions to detect and classify
 * different types of coins (bronze, gold, and Euro).
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

// Add function prototype for the missing function
int CheckForDifferentColoredCenter(IVC *frame, int centerX, int centerY, float sampleRadius, bool expectSilverCenter);

/**
 * @brief Checks if an area has a different colored center than its outer part
 * 
 * @param frame Original color frame
 * @param centerX Center X coordinate
 * @param centerY Center Y coordinate
 * @param sampleRadius Radius to sample pixels
 * @param expectSilverCenter true if expecting silver center, false if expecting gold center
 * @return 1 if different colored center detected, 0 otherwise
 */
int CheckForDifferentColoredCenter(IVC *frame, int centerX, int centerY, float sampleRadius, bool expectSilverCenter)
{
    if (!frame || !frame->data || frame->channels != 3)
        return 0;
        
    unsigned char *data = (unsigned char *)frame->data;
    int width = frame->width;
    int height = frame->height;
    int bytesPerLine = frame->width * frame->channels;
    
    // Define color thresholds for gold and silver
    const int SILVER_THRESHOLD = 30;  // RGB value difference threshold for silver
    const int GOLD_THRESHOLD = 50;    // RGB value difference threshold for gold
    
    // Bounds checking
    if (centerX < sampleRadius || centerY < sampleRadius || 
        centerX >= width - sampleRadius || centerY >= height - sampleRadius)
        return 0;
        
    // Sample points in the center
    int centerCount = 0;
    int centerGoldCount = 0;
    int centerSilverCount = 0;
    
    // Sample pixels in a grid pattern within the center
    for (int y = centerY - (int)(sampleRadius * 0.5f); y <= centerY + (int)(sampleRadius * 0.5f); y += 2) {
        for (int x = centerX - (int)(sampleRadius * 0.5f); x <= centerX + (int)(sampleRadius * 0.5f); x += 2) {
            int pos = y * bytesPerLine + x * 3;
            int r = data[pos];
            int g = data[pos + 1];
            int b = data[pos + 2];
            
            centerCount++;
            
            // Check if it's silver (all RGB channels close to each other)
            if (abs(r - g) < SILVER_THRESHOLD && abs(r - b) < SILVER_THRESHOLD && abs(g - b) < SILVER_THRESHOLD) {
                centerSilverCount++;
            }
            // Check if it's gold (higher R and G values compared to B)
            else if (r > b + GOLD_THRESHOLD && g > b + GOLD_THRESHOLD && abs(r - g) < GOLD_THRESHOLD) {
                centerGoldCount++;
            }
        }
    }
    
    // Determine if the center is primarily silver or gold
    float silverRatio = (float)centerSilverCount / centerCount;
    float goldRatio = (float)centerGoldCount / centerCount;
    
    // If we expect a silver center, check silver ratio; otherwise check gold ratio
    if (expectSilverCenter) {
        return (silverRatio > 0.4f) ? 1 : 0;
    } else {
        return (goldRatio > 0.4f) ? 1 : 0;
    }
}

/**
 * @brief Checks if an area has a different colored center than its outer part
 * 
 * @param frame Original color frame
 * @param centerX Center X coordinate
 * @param nlabels2 Number of gold blobs
 * @param excludeList List to track excluded coin positions
 * @param frameCoins Array to count detected coins
 * @param distanceThresholdSq Square of distance threshold for proximity
 * @return true if a coin was detected, false otherwise
 */
bool DetectGoldCoins(OVC *mainBlob, OVC *goldBlobs, int nlabels2, int *excludeList, int frameCoins[], int distanceThresholdSq)
{
    const int GOLD_AREA_MIN = 8000;
    const float GOLD_CIRC_THRESHOLD = 0.80f;
    const float TOLERANCE = 0.08f;   // 8% tolerance

    // Gold coin diameter constants
    const float D_50CENT = 174.0f;
    const float D_20CENT = 160.0f;
    const float D_10CENT = 143.0f;
    
    // Check if gold blob array is valid
    if (!goldBlobs || nlabels2 <= 0)
        return false;
    
    for (int i2 = 0; i2 < nlabels2; i2++) {
        OVC *goldBlob = &goldBlobs[i2];
        
        // Skip invalid blobs
        if (goldBlob->label == 0 || goldBlob->area <= GOLD_AREA_MIN)
            continue;
            
        // Check proximity
        int dx = goldBlob->xc - mainBlob->xc;
        int dy = goldBlob->yc - mainBlob->yc;
        int distance_squared = dx * dx + dy * dy;
        
        if (distance_squared > distanceThresholdSq)
            continue;
            
        // Compute shape metrics only if proximity check passes
        float circularity = CalculateCircularity(goldBlob);
        
        if (circularity <= GOLD_CIRC_THRESHOLD || !IsValidCoinShape(goldBlob))
            continue;
            
        // Compute circular diameter once
        float circularDiameter = CalculateCircularDiameter(goldBlob);
        
        // Classify gold coins by size and check if it's been detected before
        int coinType = 0;
        bool isNewDetection = false;
        
        if (MatchesCoinSize(goldBlob, D_50CENT, TOLERANCE)) {
            coinType = 6; // 50 cents = type 6
            if (IsCoinAlreadyDetected(goldBlob->xc, goldBlob->yc, coinType) == 0) {
                isNewDetection = true;
                frameCoins[5]++;
                printf("NOVA MOEDA: 50 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    circularDiameter, goldBlob->area, circularity);
            } else {
                printf("[já detectada] 50 centimos em %d,%d\n", goldBlob->xc, goldBlob->yc);
            }
        }
        else if (MatchesCoinSize(goldBlob, D_20CENT, TOLERANCE)) {
            coinType = 5; // 20 cents = type 5
            if (IsCoinAlreadyDetected(goldBlob->xc, goldBlob->yc, coinType) == 0) {
                isNewDetection = true;
                frameCoins[4]++;
                printf("NOVA MOEDA: 20 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    circularDiameter, goldBlob->area, circularity);
            } else {
                printf("[já detectada] 20 centimos em %d,%d\n", goldBlob->xc, goldBlob->yc);
            }
        }
        else if (MatchesCoinSize(goldBlob, D_10CENT, TOLERANCE)) {
            coinType = 4; // 10 cents = type 4
            if (IsCoinAlreadyDetected(goldBlob->xc, goldBlob->yc, coinType) == 0) {
                isNewDetection = true;
                frameCoins[3]++;
                printf("NOVA MOEDA: 10 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    circularDiameter, goldBlob->area, circularity);
            } else {
                printf("[já detectada] 10 centimos em %d,%d\n", goldBlob->xc, goldBlob->yc);
            }
        }
        
        if (coinType > 0) {
            // Always add to exclusion list regardless of whether it's new or not
            ExcludeCoin(excludeList, goldBlob->xc, goldBlob->yc, 0);
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Detects bronze/copper coins (1c, 2c, 5c)
 * 
 * @param mainBlob Main blob to check against
 * @param bronzeBlobs Array of bronze coin blobs
 * @param nlabels3 Number of bronze blobs
 * @param excludeList List to track excluded coin positions
 * @param frameCoins Array to count detected coins
 * @param distanceThresholdSq Square of distance threshold for proximity
 * @return true if a coin was detected, false otherwise
 */
bool DetectBronzeCoins(OVC *mainBlob, OVC *bronzeBlobs, int nlabels3, int *excludeList, int frameCoins[], int distanceThresholdSq)
{
    const int AREA_MIN_THRESHOLD = 9000;
    const float COPPER_CIRC_THRESHOLD = 0.75f;
    const float TOLERANCE = 0.08f;   // 8% tolerance

    // Bronze coin diameter constants
    const float D_5CENT = 152.0f;
    const float D_2CENT = 135.0f;
    const float D_1CENT = 122.0f;
    
    // Check if bronze blob array is valid
    if (!bronzeBlobs || nlabels3 <= 0)
        return false;
    
    for (int i4 = 0; i4 < nlabels3; i4++) {
        OVC *copperBlob = &bronzeBlobs[i4];
        
        // Skip small blobs
        if (copperBlob->area <= AREA_MIN_THRESHOLD)
            continue;
            
        // Check proximity
        int dx = copperBlob->xc - mainBlob->xc;
        int dy = copperBlob->yc - mainBlob->yc;
        int distance_squared = dx * dx + dy * dy;
        
        if (distance_squared > distanceThresholdSq)
            continue;
            
        // Compute circularity
        float circularity = CalculateCircularity(copperBlob);
        
        if (circularity <= COPPER_CIRC_THRESHOLD)
            continue;
            
        // Compute circular diameter once
        float circularDiameter = CalculateCircularDiameter(copperBlob);
        
        // Calculate corrected Y-center with 5% vertical correction for shadow compensation
        int correctedYC = copperBlob->yc + (int)(circularDiameter * 0.05f);
        
        // Classify copper coins by size and check if already detected
        int coinType = 0;
        bool isNewDetection = false;
        
        if (MatchesCoinSize(copperBlob, D_5CENT, TOLERANCE)) {
            coinType = 3; // 5 cents = type 3
            if (IsCoinAlreadyDetected(copperBlob->xc, correctedYC, coinType) == 0) {
                isNewDetection = true;
                frameCoins[2]++;
                printf("NOVA MOEDA: 5 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    circularDiameter, copperBlob->area, circularity);
            } else {
                printf("[já detectada] 5 centimos em %d,%d\n", copperBlob->xc, copperBlob->yc);
            }
        }
        else if (MatchesCoinSize(copperBlob, D_2CENT, TOLERANCE)) {
            coinType = 2; // 2 cents = type 2
            if (IsCoinAlreadyDetected(copperBlob->xc, correctedYC, coinType) == 0) {
                isNewDetection = true;
                frameCoins[1]++;
                printf("NOVA MOEDA: 2 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    circularDiameter, copperBlob->area, circularity);
            } else {
                printf("[já detectada] 2 centimos em %d,%d\n", copperBlob->xc, copperBlob->yc);
            }
        }
        else if (MatchesCoinSize(copperBlob, D_1CENT, TOLERANCE)) {
            coinType = 1; // 1 cent = type 1
            if (IsCoinAlreadyDetected(copperBlob->xc, correctedYC, coinType) == 0) {
                isNewDetection = true;
                frameCoins[0]++;
                printf("NOVA MOEDA: 1 centimo | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    circularDiameter, copperBlob->area, circularity);
            } else {
                printf("[já detectada] 1 centimo em %d,%d\n", copperBlob->xc, copperBlob->yc);
            }
        }
        
        if (coinType > 0) {
            // Always add to exclusion list regardless of whether it's new or not
            ExcludeCoin(excludeList, copperBlob->xc, correctedYC, 0);
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Improved detection of Euro coins (1€ and 2€)
 * 
 * This function examines the relationship between gold and silver components
 * to accurately identify Euro coins regardless of which part is detected first.
 * 
 * @param frame Original frame for pixel color analysis
 * @param goldBlobs Array of gold coin blobs
 * @param nGoldBlobs Number of gold blobs
 * @param silverBlobs Array of silver coin blobs
 * @param nSilverBlobs Number of silver blobs
 * @param excludeList List to track excluded coin positions
 * @param coinCounts Array to count detected coins
 */
void DetectEuroCoins(IVC *frame, OVC *goldBlobs, int nGoldBlobs, 
                  OVC *silverBlobs, int nSilverBlobs, 
                  int *excludeList, int *coinCounts)
{
    // Coin diameter constants (reference values in pixels)
    const float D_1EURO = 185.0f;             // 1€ - 23.25mm
    const float D_2EURO = 205.0f;             // 2€ - 25.75mm
    const float SIZE_TOLERANCE = 0.10f;       // 10% tolerance
    
    // Track processed coins to avoid double counting
    bool *processedGold = (bool*)calloc(nGoldBlobs, sizeof(bool));
    bool *processedSilver = (bool*)calloc(nSilverBlobs, sizeof(bool));
    
    if (!processedGold || !processedSilver) {
        // Memory allocation failed
        if (processedGold) free(processedGold);
        if (processedSilver) free(processedSilver);
        return;
    }
    
    // First pass: Try to detect 1€ coins (gold outside, silver inside)
    for (int i = 0; i < nGoldBlobs; i++) {
        // Skip small or invalid blobs
        if (goldBlobs[i].label == 0 || goldBlobs[i].area < 8000 || processedGold[i]) 
            continue;
        
        float goldDiameter = CalculateCircularDiameter(&goldBlobs[i]);
        float goldCircularity = CalculateCircularity(&goldBlobs[i]);
        
        // Check if this gold blob is approximately the size of a 1€ coin
        bool sizeMatch1Euro = fabsf(goldDiameter - D_1EURO) <= (D_1EURO * SIZE_TOLERANCE);
        
        // Only proceed if it's circular and size matches approximately
        if (goldCircularity < 0.75f || !sizeMatch1Euro)
            continue;
            
        // Now look for a silver blob inside this gold blob
        for (int j = 0; j < nSilverBlobs; j++) {
            if (silverBlobs[j].label == 0 || silverBlobs[j].area < 1000 || processedSilver[j])
                continue;
                
            // Check if silver blob is inside the gold blob (centers are close)
            int dx = silverBlobs[j].xc - goldBlobs[i].xc;
            int dy = silverBlobs[j].yc - goldBlobs[i].yc;
            float distance = sqrtf(dx*dx + dy*dy);
            
            // Silver center should be inside the gold ring
            float silverDiameter = CalculateCircularDiameter(&silverBlobs[j]);
            float silverRadiusPercent = silverDiameter / goldDiameter;
            
            // For 1€, the silver center should be approximately 50-60% of the total diameter
            if (distance < (goldDiameter * 0.25f) && silverRadiusPercent > 0.40f && silverRadiusPercent < 0.70f) {
                // Found a 1€ coin with gold outside, silver inside
                int coinType = 7; // 1€ = type 7
                
                if (IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, coinType) == 0) {
                    printf("NOVA MOEDA: 1 EURO (contorno AZUL) | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                           goldDiameter, goldBlobs[i].area, goldCircularity);
                    coinCounts[6]++; // Update the count of 1 Euro coins
                } else {
                    printf("[já detectada] 1 EURO em %d,%d\n", goldBlobs[i].xc, goldBlobs[i].yc);
                }
                
                // Mark both blobs as processed
                processedGold[i] = true;
                processedSilver[j] = true;
                
                // Exclude this coin
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                break;
            }
        }
    }
    
    // Second pass: Try to detect 2€ coins (silver outside, gold inside)
    for (int i = 0; i < nSilverBlobs; i++) {
        // Skip small or invalid blobs
        if (silverBlobs[i].label == 0 || silverBlobs[i].area < 8000 || processedSilver[i]) 
            continue;
        
        float silverDiameter = CalculateCircularDiameter(&silverBlobs[i]);
        float silverCircularity = CalculateCircularity(&silverBlobs[i]);
        
        // Check if this silver blob is approximately the size of a 2€ coin
        bool sizeMatch2Euro = fabsf(silverDiameter - D_2EURO) <= (D_2EURO * SIZE_TOLERANCE);
        
        // Only proceed if it's circular and size matches approximately
        if (silverCircularity < 0.75f || !sizeMatch2Euro)
            continue;
            
        // Now look for a gold blob inside this silver blob
        for (int j = 0; j < nGoldBlobs; j++) {
            if (goldBlobs[j].label == 0 || goldBlobs[j].area < 1000 || processedGold[j])
                continue;
                
            // Check if gold blob is inside the silver blob (centers are close)
            int dx = goldBlobs[j].xc - silverBlobs[i].xc;
            int dy = goldBlobs[j].yc - silverBlobs[i].yc;
            float distance = sqrtf(dx*dx + dy*dy);
            
            // Gold center should be inside the silver ring
            float goldDiameter = CalculateCircularDiameter(&goldBlobs[j]);
            float goldRadiusPercent = goldDiameter / silverDiameter;
            
            // For 2€, the gold center should be approximately 60-75% of the total diameter
            if (distance < (silverDiameter * 0.25f) && goldRadiusPercent > 0.50f && goldRadiusPercent < 0.80f) {
                // Found a 2€ coin with silver outside, gold inside
                int coinType = 8; // 2€ = type 8
                
                if (IsCoinAlreadyDetected(silverBlobs[i].xc, silverBlobs[i].yc, coinType) == 0) {
                    printf("NOVA MOEDA: 2 EURO (contorno AZUL) | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                           silverDiameter, silverBlobs[i].area, silverCircularity);
                    coinCounts[7]++; // Update the count of 2 Euro coins
                } else {
                    printf("[já detectada] 2 EURO em %d,%d\n", silverBlobs[i].xc, silverBlobs[i].yc);
                }
                
                // Mark both blobs as processed
                processedSilver[i] = true;
                processedGold[j] = true;
                
                // Exclude this coin
                ExcludeCoin(excludeList, silverBlobs[i].xc, silverBlobs[i].yc, 0);
                break;
            }
        }
    }
    
    // Fallback method for cases where the inner part wasn't correctly detected
    // This handles cases where only the outer part is visible
    
    // Look for unprocessed gold blobs that might be 1€ coins
    for (int i = 0; i < nGoldBlobs; i++) {
        if (goldBlobs[i].label == 0 || goldBlobs[i].area < 8000 || processedGold[i]) 
            continue;
            
        float diameter = CalculateCircularDiameter(&goldBlobs[i]);
        float circularity = CalculateCircularity(&goldBlobs[i]);
        
        // Check if it's circular enough and matches 1€ size
        if (circularity >= 0.80f && fabsf(diameter - D_1EURO) <= (D_1EURO * SIZE_TOLERANCE)) {
            // Check if this area actually has a different color in the center
            // by sampling pixels near the center
            int hasColoredCenter = CheckForDifferentColoredCenter(frame, goldBlobs[i].xc, goldBlobs[i].yc, 
                                                                diameter * 0.3f, true);
            
            if (hasColoredCenter) {
                int coinType = 7; // 1€
                
                if (IsCoinAlreadyDetected(goldBlobs[i].xc, goldBlobs[i].yc, coinType) == 0) {
                    printf("NOVA MOEDA (fallback): 1 EURO | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                           diameter, goldBlobs[i].area, circularity);
                    coinCounts[6]++;
                } else {
                    printf("[já detectada] 1 EURO em %d,%d\n", goldBlobs[i].xc, goldBlobs[i].yc);
                }
                
                ExcludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                processedGold[i] = true;
            }
        }
    }
    
    // Look for unprocessed silver blobs that might be 2€ coins
    for (int i = 0; i < nSilverBlobs; i++) {
        if (silverBlobs[i].label == 0 || silverBlobs[i].area < 8000 || processedSilver[i]) 
            continue;
            
        float diameter = CalculateCircularDiameter(&silverBlobs[i]);
        float circularity = CalculateCircularity(&silverBlobs[i]);
        
        // Check if it's circular enough and matches 2€ size
        if (circularity >= 0.80f && fabsf(diameter - D_2EURO) <= (D_2EURO * SIZE_TOLERANCE)) {
            // Check if this area actually has a different color in the center
            // by sampling pixels near the center
            int hasColoredCenter = CheckForDifferentColoredCenter(frame, silverBlobs[i].xc, silverBlobs[i].yc, 
                                                                diameter * 0.3f, false);
            
            if (hasColoredCenter) {
                int coinType = 8; // 2€
                
                if (IsCoinAlreadyDetected(silverBlobs[i].xc, silverBlobs[i].yc, coinType) == 0) {
                    printf("NOVA MOEDA (fallback): 2 EURO | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                           diameter, silverBlobs[i].area, circularity);
                    coinCounts[7]++;
                } else {
                    printf("[já detectada] 2 EURO em %d,%d\n", silverBlobs[i].xc, silverBlobs[i].yc);
                }
                
                ExcludeCoin(excludeList, silverBlobs[i].xc, silverBlobs[i].yc, 0);
                processedSilver[i] = true;
            }
        }
    }
    
    // Free the memory
    free(processedGold);
    free(processedSilver);
}

/**
 * @brief Detecta se uma moeda é bimetálica (1€ ou 2€)
 */
int DetectBimetalCoin(IVC *src, int xc, int yc, int radius, OVC *silverBlobs, int nSilverBlobs)
{
    // Validação de parâmetros
    if (!src || !silverBlobs || nSilverBlobs <= 0 || radius <= 0)
        return 0;
    
    // Constantes para detecção de moedas - valores específicos para precisão
    const float INNER_RATIO_1EURO = 0.5f;     // Centro de prata ~50% do diâmetro (1€)
    const float INNER_RATIO_2EURO = 0.6f;     // Centro de ouro ~60% do diâmetro (2€)
    const float INNER_RATIO_TOLERANCE = 0.12f; // Tolerância para variação de proporção
    
    // Diâmetros conhecidos das moedas (em pixels) - calibrados para a câmera
    const float D_1EURO = 185.0f;             // 1€ - 23.25mm
    const float D_2EURO = 205.0f;             // 2€ - 25.75mm
    const float SIZE_TOLERANCE = 0.10f;       // 10% de tolerância no tamanho
    
    // Tolerâncias para centralização
    const float CENTER_TOLERANCE = 0.25f;     // Deslocamento máximo do centro
    
    // Parâmetros de área mínima para evitar ruído
    const int MIN_SILVER_AREA = 500;          // Área mínima para centro de prata
    
    // Variáveis para armazenar o melhor candidato
    float bestScore = 0.0f;
    int bestType = 0;
    float coinDiameter = radius * 2.0f;
    
    // Calcula centro e raio quadrados para eliminar operações de raiz quadrada
    int xcSq = xc * xc;
    int ycSq = yc * yc;
    int maxDistSq = (int)(radius * CENTER_TOLERANCE) * (int)(radius * CENTER_TOLERANCE);
    
    // Percorre todos os blobs de prata
    for (int i = 0; i < nSilverBlobs; i++) {
        // Filtra blobs muito pequenos
        if (silverBlobs[i].area < MIN_SILVER_AREA)
            continue;
        
        // Calcula quadrado da distância (evita raiz quadrada)
        int dx = silverBlobs[i].xc - xc;
        int dy = silverBlobs[i].yc - yc;
        int distSq = dx*dx + dy*dy;
        
        // Verifica se o centro está próximo o suficiente
        if (distSq > maxDistSq)
            continue;
        
        // Calcula o diâmetro do blob de prata
        float silverDiameter = CalculateCircularDiameter(&silverBlobs[i]);
        float diameterRatio = silverDiameter / (2.0f * radius);
        
        float score1Euro = 0.0f, score2Euro = 0.0f;
        
        // 1 EURO: Centro prateado com borda dourada
        if (diameterRatio <= 0.7f) {  // Certifica-se que não é a parte externa do 2€
            score1Euro = (1.0f - fabsf(diameterRatio - INNER_RATIO_1EURO) / INNER_RATIO_TOLERANCE) * 0.6f +
                      (1.0f - fabsf(coinDiameter - D_1EURO) / (D_1EURO * SIZE_TOLERANCE)) * 0.4f;
        }
        
        // 2 EURO: Centro dourado com borda prateada
        if (diameterRatio >= 0.8f) {  // Grande o suficiente para ser a borda externa do 2€
            score2Euro = (1.0f - fabsf(coinDiameter - D_2EURO) / (D_2EURO * SIZE_TOLERANCE));
        }
        
        // Ajusta pontuações para ficarem no intervalo [0,1]
        score1Euro = score1Euro < 0.0f ? 0.0f : (score1Euro > 1.0f ? 1.0f : score1Euro);
        score2Euro = score2Euro < 0.0f ? 0.0f : (score2Euro > 1.0f ? 1.0f : score2Euro);
        
        // Atualiza melhor candidato
        if (score1Euro > bestScore && score1Euro > 0.65f) {
            bestScore = score1Euro;
            bestType = 1;
        }
        
        if (score2Euro > bestScore && score2Euro > 0.65f) {
            bestScore = score2Euro;
            bestType = 2;
        }
        
        // Classificação imediata se a pontuação for muito alta
        if (bestScore > 0.85f)
            return bestType;
    }
    
    // Classificação baseada apenas no diâmetro total quando não encontrou padrão bimetálico claro
    if (bestType == 0) {
        float size1EuroError = fabsf(coinDiameter - D_1EURO) / D_1EURO;
        float size2EuroError = fabsf(coinDiameter - D_2EURO) / D_2EURO;
        
        if (size1EuroError < SIZE_TOLERANCE && size1EuroError < size2EuroError)
            return 1;
        else if (size2EuroError < SIZE_TOLERANCE)
            return 2;
    }
    
    return bestType;
}

#ifdef __cplusplus
}
#endif
