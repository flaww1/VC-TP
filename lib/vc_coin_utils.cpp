/**
 * @file vc_coin_utils.cpp
 * @brief Funções utilitárias para apoio à deteção e classificação de moedas.
 *
 * Este ficheiro contém funções auxiliares dedicadas ao processamento de imagens
 * que servem de suporte às funções principais de identificação de moedas.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <math.h>
#include <limits.h>

#include "vc.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Contador de frames para rastreamento de moedas
static int frameCounter = 0;

// Buffer para rastrear moedas já detectadas (formato: x, y, tipo, frame)
#define MAX_TRACKED_COINS 150  // Increased to track more coins
static int detectedCoins[MAX_TRACKED_COINS][4] = {{0}};

/**
 * @brief Incrementa o contador de frames para rastreamento de moedas
 */
void IncrementFrameCounter()
{
    frameCounter++;
    
    // Reset contador a cada 1000 frames para evitar overflow
    if (frameCounter > 1000) frameCounter = 0;
}

/**
 * @brief Verifica se uma moeda já foi detectada anteriormente
 * @param x Coordenada X da moeda
 * @param y Coordenada Y da moeda
 * @param coinType Tipo de moeda (1-8)
 * @return 1 se já detectada, 0 caso contrário
 */
int IsCoinAlreadyDetected(int x, int y, int coinType)
{
    const int distanceThreshold = 50;  // Limiar de distância para considerar a mesma moeda
    const int distThresholdSq = distanceThreshold * distanceThreshold;
    const int frameMemory = 30;        // Número de frames para lembrar moedas detectadas
    
    for (int i = 0; i < MAX_TRACKED_COINS; i++) {
        // Ignorar entradas vazias
        if (detectedCoins[i][0] == 0 && detectedCoins[i][1] == 0) 
            continue;
            
        // Verificar se é o mesmo tipo de moeda e está próximo
        if (detectedCoins[i][2] == coinType) {
            int dx = detectedCoins[i][0] - x;
            int dy = detectedCoins[i][1] - y;
            int distSq = dx*dx + dy*dy;
            
            // Verificar se está dentro da distância e recente o suficiente
            if (distSq <= distThresholdSq && 
                (frameCounter - detectedCoins[i][3] < frameMemory || 
                detectedCoins[i][3] > frameCounter)) { // Handle counter reset
                
                // Atualiza a posição e frame
                detectedCoins[i][0] = x;
                detectedCoins[i][1] = y;
                detectedCoins[i][3] = frameCounter;
                return 1;  // Já detectada
            }
        }
    }
    
    // Não encontrada, adiciona nova moeda no primeiro slot vazio
    for (int i = 0; i < MAX_TRACKED_COINS; i++) {
        if (detectedCoins[i][0] == 0 && detectedCoins[i][1] == 0) {
            detectedCoins[i][0] = x;
            detectedCoins[i][1] = y;
            detectedCoins[i][2] = coinType;
            detectedCoins[i][3] = frameCounter;
            break;
        }
    }
    
    return 0;  // Nova detecção
}

/**
 * @brief Gere a lista de moedas a excluir da contagem
 * @param excludeList Array para guardar coordenadas das moedas
 * @param xc Coordenada x do centro da moeda
 * @param yc Coordenada y do centro da moeda
 * @param option 0 para adicionar, 1 para remover
 * @return 0
 */
int ExcludeCoin(int *excludeList, int xc, int yc, int option)
{
    if (!excludeList) return 0;
    
    const int PROXIMITY_THRESHOLD_SQ = 30 * 30; // 30 pixels squared distance threshold
    
    if (option == 0) {
        // Add to exclusion list - find first empty slot
        for (int i = 0; i < MAX_COINS; i++) {
            if (excludeList[i * 2] == 0 && excludeList[i * 2 + 1] == 0) {
                excludeList[i * 2] = xc;
                excludeList[i * 2 + 1] = yc;
                break;
            }
        }
    }
    else if (option == 1) {
        // Remove from exclusion list - remove entries close to the specified position
        for (int i = 0; i < MAX_COINS; i++) {
            if (excludeList[i * 2] == 0 && excludeList[i * 2 + 1] == 0) {
                continue;
            }
            
            // Check if this entry is close enough to the specified position
            int dx = excludeList[i * 2] - xc;
            int dy = excludeList[i * 2 + 1] - yc;
            int distSq = dx*dx + dy*dy;
            
            if (distSq <= PROXIMITY_THRESHOLD_SQ) {
                // Clear this entry
                excludeList[i * 2] = 0;
                excludeList[i * 2 + 1] = 0;
            }
        }
    }
    
    return 0;
}

/**
 * @brief Determina a circularidade de um blob para validação de moedas
 *
 * Esta função calcula o quão circular é um blob baseado na sua área e perímetro,
 * ajudando a filtrar objetos que não são moedas.
 *
 * @param blob Estrutura OVC contendo informações do blob
 * @return Valor de circularidade (1.0 = perfeitamente circular)
 */
float CalculateCircularity(OVC *blob)
{
    // Circularidade = 4 * π * área / (perímetro)²
    // Um círculo perfeito tem valor 1.0
    // Valores menores indicam formas menos circulares
    if (blob->perimeter == 0)
        return 0.0f;

    float circularity = (4.0f * 3.14159f * (float)blob->area) / ((float)(blob->perimeter * blob->perimeter));

    // Limita o valor máximo a 1.0 (pode exceder devido a aproximações)
    if (circularity > 1.0f)
        circularity = 1.0f;

    return circularity;
}

/**
 * @brief Calcula o diâmetro de um blob de moeda usando ajuste circular em vez de caixa delimitadora
 *
 * Esta função estima o diâmetro de um objeto circular de forma mais precisa,
 * usando a área do blob para calcular o diâmetro de um círculo equivalente.
 *
 * @param blob Ponteiro para a estrutura do blob
 * @return Diâmetro estimado com base na área circular
 */
float CalculateCircularDiameter(OVC *blob)
{
    // Área de um círculo = π * r² onde r é o raio
    // Então diâmetro = 2 * sqrt(área / π)
    float area = (float)blob->area;
    return 2.0f * sqrtf(area / 3.14159f);
}

/**
 * @brief Determina se as dimensões físicas de um blob correspondem a uma moeda específica
 *
 * @param blob Ponteiro para a estrutura do blob
 * @param targetDiameter O diâmetro esperado para este tipo de moeda
 * @param tolerance Porcentagem de desvio aceitável do alvo (ex.: 0.08 = 8%)
 * @return true se o blob corresponder às dimensões da moeda, false caso contrário
 */
bool MatchesCoinSize(OVC *blob, float targetDiameter, float tolerance)
{
    // Calcula o diâmetro circular a partir da área
    float diameter = CalculateCircularDiameter(blob);

    // Verifica se o diâmetro está dentro da faixa de tolerância
    float minAcceptable = targetDiameter * (1.0f - tolerance);
    float maxAcceptable = targetDiameter * (1.0f + tolerance);

    return (diameter >= minAcceptable && diameter <= maxAcceptable);
}

/**
 * @brief Verifica se um blob tem uma forma consistente de moeda
 *
 * Esta função avalia a circularidade e a área esperada para determinar
 * se um blob é uma moeda válida, usando o diâmetro circular calculado.
 *
 * @param blob Estrutura OVC contendo informações do blob
 * @return true se o blob for uma moeda válida, false caso contrário
 */
bool IsValidCoinShape(OVC *blob)
{
    // Para uma moeda válida, a forma deve ser quase circular
    // e a área deve corresponder ao esperado para um círculo com esse diâmetro

    // Use CalculateCircularDiameter para um cálculo mais preciso
    float diameter = CalculateCircularDiameter(blob);
    float radius = diameter / 2.0f;
    
    // Calcula a área esperada para um círculo com este diâmetro
    float expectedArea = 3.14159f * radius * radius;
    
    // Verifica a proporção da área real em relação à área esperada
    float areaRatio = (float)blob->area / expectedArea;
    if (areaRatio < 0.85f || areaRatio > 1.15f)
        return false;
    
    // Verifica se o bounding box não é muito não-quadrado
    float width = (float)blob->width;
    float height = (float)blob->height;
    float aspectRatio = width / height;
    
    // Permite alguma variação na forma do bounding box
    if (aspectRatio < 0.85f || aspectRatio > 1.15f)
        return false;
    
    // Verifica a circularidade usando a função existente
    float circularity = CalculateCircularity(blob);
    if (circularity < 0.75f)
        return false;
        
    return true;
}

/**
 * @brief Filtra blobs de moedas de cobre com base na circularidade e reporta moedas detectadas
 */
void FilterCopperCoinBlobs(OVC *blobs, int nlabels)
{
    // Constants for coin classification - accurate measurements
    const float D_5CENT = 152.0f; // 5 cents is the largest copper coin
    const float D_2CENT = 135.0f; // 2 cents is medium sized
    const float D_1CENT = 122.0f; // 1 cent is the smallest copper coin
    const float TOLERANCE = 0.08f; // Tighter tolerance for better accuracy
    
    for (int i = 0; i < nlabels; i++) {
        float circularity = CalculateCircularity(&blobs[i]);
        float diameter = CalculateCircularDiameter(&blobs[i]);
        
        // More aggressive filtering - higher circularity requirement
        if (circularity < 0.7f) {
            blobs[i].label = 0;
            continue;
        }
        
        // Log each valid copper coin with clearer size categorization
        if (blobs[i].area > 7000) {
            // Use more precise size ranges with less overlap
            if (diameter >= D_1CENT * 0.92f && diameter <= D_1CENT * 1.08f) {
                printf("IDENTIFICADO: 1 centimo | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    diameter, blobs[i].area, circularity);
            }
            else if (diameter >= D_2CENT * 0.92f && diameter <= D_2CENT * 1.08f) {
                printf("IDENTIFICADO: 2 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    diameter, blobs[i].area, circularity);
            }
            else if (diameter >= D_5CENT * 0.92f && diameter <= D_5CENT * 1.08f) {
                printf("IDENTIFICADO: 5 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    diameter, blobs[i].area, circularity);
            }
        }
    }
}

/**
 * @brief Filtra blobs de moedas douradas com base na circularidade e área, e reporta moedas detectadas
 */
void FilterGoldCoinBlobs(OVC *blobs, int nlabels)
{
    // Constants for coin classification - accurate measurements
    const float D_50CENT = 174.0f; // 50 cents is the largest gold coin
    const float D_20CENT = 160.0f; // 20 cents is medium sized
    const float D_10CENT = 143.0f; // 10 cents is the smallest gold coin
    const float TOLERANCE = 0.08f; // Tighter tolerance for better accuracy
    
    // Edge proximity check parameter - distance from blob center to frame edge
    const int EDGE_MARGIN = 90; // Half the diameter of the largest coin + margin
    
    for (int i = 0; i < nlabels; i++) {
        float circularity = CalculateCircularity(&blobs[i]);
        float diameter = CalculateCircularDiameter(&blobs[i]);
        
        // More aggressive filtering - higher circularity requirement for gold coins
        if (circularity < 0.7f) {
            blobs[i].label = 0;
            continue;
        }
        
        // Check if blob is near the edge of the frame
        bool isNearEdge = false;
        if (blobs[i].xc < EDGE_MARGIN || blobs[i].yc < EDGE_MARGIN || 
            blobs[i].xc > 640 - EDGE_MARGIN || blobs[i].yc > 480 - EDGE_MARGIN) {
            isNearEdge = true;
        }
        
        // Log each valid gold coin with clearer size categorization - but skip edge coins
        if (blobs[i].area > 8000 && !isNearEdge) {
            if (diameter >= D_10CENT * 0.92f && diameter <= D_10CENT * 1.08f) {
                printf("IDENTIFICADO: 10 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    diameter, blobs[i].area, circularity);
            }
            else if (diameter >= D_20CENT * 0.92f && diameter <= D_20CENT * 1.08f) {
                printf("IDENTIFICADO: 20 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    diameter, blobs[i].area, circularity);
            }
            else if (diameter >= D_50CENT * 0.92f && diameter <= D_50CENT * 1.08f) {
                printf("IDENTIFICADO: 50 centimos | Diâm: %.1f | Área: %d | Circularity: %.2f\n",
                    diameter, blobs[i].area, circularity);
            }
        }
    }
}

/**
 * @brief Gets the most recent coin type detected at a specific location
 * This helps maintain coin type consistency when coins are partially visible
 * @param x X coordinate
 * @param y Y coordinate
 * @return Coin type (1-8) or 0 if no coin was detected at that location
 */
int GetLastCoinTypeAtLocation(int x, int y)
{
    const int distanceThreshold = 50;
    const int distThresholdSq = distanceThreshold * distanceThreshold;
    
    // Search through all tracked coins for the nearest match
    int nearestType = 0;
    int nearestDistSq = INT_MAX;
    
    for (int i = 0; i < MAX_TRACKED_COINS; i++) {
        // Skip empty slots
        if (detectedCoins[i][0] == 0 && detectedCoins[i][1] == 0)
            continue;
        
        // Calculate distance to this tracked coin
        int dx = detectedCoins[i][0] - x;
        int dy = detectedCoins[i][1] - y;
        int distSq = dx*dx + dy*dy;
        
        // If this is closer than our current best match
        if (distSq < nearestDistSq && distSq <= distThresholdSq) {
            nearestDistSq = distSq;
            nearestType = detectedCoins[i][2];
        }
    }
    
    return nearestType;
}

/**
 * @brief Desenha visualizações de moedas - simplified version that only handles gold and copper coins
 */
void DrawCoins(IVC *frame, OVC *goldBlobs, OVC *copperBlobs, 
              int nGoldBlobs, int nCopperBlobs, 
              OVC *silverBlobs, int nSilverBlobs)
{
    // Draw copper coins first (typically smaller denominations)
    if (copperBlobs && nCopperBlobs > 0) {
        DrawBoundingBoxes(frame, copperBlobs, nCopperBlobs, 0);  // 0 = copper coins
    }
    
    // Draw gold coins second (larger denominations)
    if (goldBlobs && nGoldBlobs > 0) {
        DrawBoundingBoxes(frame, goldBlobs, nGoldBlobs, 1);  // 1 = gold coins
    }
    
    // Silver coins are ignored completely
}

#ifdef __cplusplus
}
#endif
