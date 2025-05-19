/**
 * @file vc_coin_detection.cpp
 * @brief Algoritmos específicos para deteção e classificação de moedas.
 *
 * Este ficheiro implementa funções especializadas para detetar diferentes tipos
 * de moedas em imagens, classificando-as de acordo com o seu tamanho e cor.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vc.h"

#ifdef __cplusplus
extern "C" {
#endif

// Constantes externas de vc_coin.cpp - diâmetros de referência para cada moeda
extern const float DIAM_1CENT;
extern const float DIAM_2CENT;
extern const float DIAM_5CENT;
extern const float DIAM_10CENT;
extern const float DIAM_20CENT;
extern const float DIAM_50CENT;
extern const float DIAM_1EURO;
extern const float DIAM_2EURO;

// Declarações de funções externas necessárias para este ficheiro
int getCoinTypeAtLocation(int x, int y);
void correctGoldCoins(int x, int y, int *counters);

/**
 * @brief Deteta moedas de cobre (1, 2, 5 cêntimos)
 *
 * Esta função analisa os blobs identificados para encontrar moedas de cobre,
 * comparando o seu diâmetro com os valores de referência. Incorpora mecanismos
 * para lidar com moedas parcialmente visíveis nas bordas da imagem.
 *
 * @param blob Ponteiro para o blob atual em análise
 * @param copperBlobs Array de blobs candidatos a moedas de cobre
 * @param ncopperBlobs Número de blobs candidatos a moedas de cobre
 * @param excludeList Lista de moedas já excluídas da análise
 * @param counters Array de contadores para cada tipo de moeda
 * @param distThresholdSq Limiar de distância ao quadrado para associação entre blobs
 * @return true se uma moeda de cobre for identificada, false caso contrário
 */
bool detectCopperCoins(OVC *blob, OVC *copperBlobs, int ncopperBlobs, 
                     int *excludeList, int *counters, int distThresholdSq) {
    if (!blob || !copperBlobs || ncopperBlobs <= 0)
        return false;
    
    const int frameWidth = 640;
    const int frameHeight = 480;
    const int EDGE_MARGIN = 80;
    
    const float MIN_VALID_AREA = 6000;
    const float MIN_VALID_CIRCULARITY = 0.70f;
    
    for (int i = 0; i < ncopperBlobs; i++) {
        // Ignora blobs inválidos
        if (copperBlobs[i].label == 0 || copperBlobs[i].area < MIN_VALID_AREA)
            continue;
            
        // Verifica a distância entre os centros dos blobs
        const int dx = copperBlobs[i].xc - blob->xc;
        const int dy = copperBlobs[i].yc - blob->yc;
        const int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            const float diameter = getDiameter(&copperBlobs[i]);
            const float circularity = getCircularity(&copperBlobs[i]);
            
            // Ignora objetos com baixa circularidade
            if (circularity < MIN_VALID_CIRCULARITY)
                continue;
            
            const int correctedYC = copperBlobs[i].yc + (int)(diameter * 0.05f);
            const float tolerance = adaptTolerance(copperBlobs[i].xc, 
                                              copperBlobs[i].yc, 
                                              frameWidth, frameHeight);
            
            // Verifica se a moeda está próxima à borda da imagem
            const bool isNearEdge = (copperBlobs[i].xc < EDGE_MARGIN || 
                                   copperBlobs[i].yc < EDGE_MARGIN || 
                                   copperBlobs[i].xc > frameWidth - EDGE_MARGIN || 
                                   copperBlobs[i].yc > frameHeight - EDGE_MARGIN);
            
            if (isNearEdge) {
                // Obtém o último tipo de moeda nesta localização
                const int coinType = getCoinTypeAtLocation(copperBlobs[i].xc, copperBlobs[i].yc);
                
                if (coinType >= 1 && coinType <= 3) {
                    // Evita contagem duplicada se a moeda já foi detetada
                    if (!trackCoin(copperBlobs[i].xc, copperBlobs[i].yc, coinType, 1))
                        counters[coinType - 1]++;
                        
                    excludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                    return true;
                }
                
                // Calcula a melhor correspondência com base na razão de tamanho
                const float size1Ratio = diameter / DIAM_1CENT;
                const float size2Ratio = diameter / DIAM_2CENT; 
                const float size5Ratio = diameter / DIAM_5CENT;
                
                const float diff1 = fabsf(size1Ratio - 1.0f);
                const float diff2 = fabsf(size2Ratio - 1.0f);
                const float diff5 = fabsf(size5Ratio - 1.0f);
                
                // Determina o tipo de moeda mais provável
                int bestType = (diff1 < diff2 && diff1 < diff5) ? 0 : 
                              (diff2 < diff1 && diff2 < diff5) ? 1 : 2;
                
                if (!trackCoin(copperBlobs[i].xc, copperBlobs[i].yc, bestType + 1, 1))
                    counters[bestType]++;
                    
                excludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            
            // Calcula os intervalos de comparação para cada tipo de moeda
            const float d1Lower = DIAM_1CENT * (1.0f - tolerance);
            const float d1Upper = DIAM_1CENT * (1.0f + tolerance);
            const float d2Lower = DIAM_2CENT * (1.0f - tolerance);
            const float d2Upper = DIAM_2CENT * (1.0f + tolerance);
            const float d5Lower = DIAM_5CENT * (1.0f - tolerance);
            const float d5Upper = DIAM_5CENT * (1.0f + tolerance);
            
            // Verifica correspondência para cada tipo de moeda de cobre
            if (diameter >= d1Lower && diameter <= d1Upper) {
                if (!trackCoin(copperBlobs[i].xc, copperBlobs[i].yc, 1, 1)) {
                    counters[0]++;
                    printf("[MOEDA] 1 cêntimo | €0.01 | Diâm: %.1f | Área: %d | Circularidade: %.2f\n",
                          diameter, copperBlobs[i].area, circularity);
                }
                
                excludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            else if (diameter >= d2Lower && diameter <= d2Upper) {
                if (!trackCoin(copperBlobs[i].xc, copperBlobs[i].yc, 2, 1)) {
                    counters[1]++;
                    printf("[MOEDA] 2 cêntimos | €0.02 | Diâm: %.1f | Área: %d | Circularidade: %.2f\n",
                          diameter, copperBlobs[i].area, circularity);
                }
                
                excludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
            else if (diameter >= d5Lower && diameter <= d5Upper) {
                if (!trackCoin(copperBlobs[i].xc, copperBlobs[i].yc, 3, 1)) {
                    counters[2]++;
                    printf("[MOEDA] 5 cêntimos | €0.05 | Diâm: %.1f | Área: %d | Circularidade: %.2f\n",
                          diameter, copperBlobs[i].area, circularity);
                }
                
                excludeCoin(excludeList, copperBlobs[i].xc, correctedYC, 0);
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief Deteta moedas douradas (10, 20, 50 cêntimos)
 *
 * Esta função processa blobs para identificar moedas douradas, classificando-as
 * com base no seu diâmetro. Utiliza limiares específicos para moedas douradas e
 * implementa estratégias para lidar com moedas próximas às bordas da imagem.
 * 
 * @param blob Ponteiro para o blob atual em análise
 * @param goldBlobs Array de blobs candidatos a moedas douradas
 * @param ngoldBlobs Número de blobs candidatos a moedas douradas
 * @param excludeList Lista de moedas já excluídas da análise
 * @param counters Array de contadores para cada tipo de moeda
 * @param distThresholdSq Limiar de distância ao quadrado para associação entre blobs
 * @return true se uma moeda dourada for identificada, false caso contrário
 */
bool detectGoldCoins(OVC *blob, OVC *goldBlobs, int ngoldBlobs, 
                   int *excludeList, int *counters, int distThresholdSq) {
    if (!blob || !goldBlobs || ngoldBlobs <= 0)
        return false;
    
    const int frameWidth = 640;
    const int frameHeight = 480;
    const int EDGE_MARGIN = 90; // Margem maior para moedas douradas
    
    // Pré-calcula intervalos de diâmetro para comparação eficiente
    const float MIN_VALID_AREA = 6000;
    const float MIN_VALID_CIRCULARITY = 0.75f; // Circularidade maior para moedas douradas
    
    for (int i = 0; i < ngoldBlobs; i++) {
        // Ignora blobs inválidos rapidamente
        if (goldBlobs[i].label == 0 || goldBlobs[i].area < MIN_VALID_AREA)
            continue;
            
        // Verifica distância com menos operações
        const int dx = goldBlobs[i].xc - blob->xc;
        const int dy = goldBlobs[i].yc - blob->yc;
        const int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            const float diameter = getDiameter(&goldBlobs[i]);
            const float circularity = getCircularity(&goldBlobs[i]);
            
            // Ignora objetos não circulares
            if (circularity < MIN_VALID_CIRCULARITY)
                continue;
            
            const float tolerance = adaptTolerance(goldBlobs[i].xc, 
                                              goldBlobs[i].yc, 
                                              frameWidth, frameHeight);
            
            // Verifica se a moeda está próxima à borda da imagem
            const bool isNearEdge = (goldBlobs[i].xc < EDGE_MARGIN || 
                                   goldBlobs[i].yc < EDGE_MARGIN || 
                                   goldBlobs[i].xc > frameWidth - EDGE_MARGIN || 
                                   goldBlobs[i].yc > frameHeight - EDGE_MARGIN);
            
            if (isNearEdge) {
                // Obtém o último tipo de moeda nesta localização
                const int coinType = getCoinTypeAtLocation(goldBlobs[i].xc, goldBlobs[i].yc);
                
                if (coinType >= 4 && coinType <= 6) {
                    // Evita contagem duplicada se a moeda já foi detetada
                    if (!trackCoin(goldBlobs[i].xc, goldBlobs[i].yc, coinType, 1))
                        counters[coinType - 1]++;
                        
                    excludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                    return true;
                }
                
                // Calcula a melhor correspondência com base na razão de tamanho
                const float size10Ratio = diameter / DIAM_10CENT;
                const float size20Ratio = diameter / DIAM_20CENT; 
                const float size50Ratio = diameter / DIAM_50CENT;
                
                const float diff10 = fabsf(size10Ratio - 1.0f);
                const float diff20 = fabsf(size20Ratio - 1.0f);
                const float diff50 = fabsf(size50Ratio - 1.0f);
                
                // Índice com offset para moedas douradas (3 = 10c, 4 = 20c, 5 = 50c)
                int bestType = (diff10 < diff20 && diff10 < diff50) ? 3 : 
                              (diff20 < diff10 && diff20 < diff50) ? 4 : 5;
                
                if (!trackCoin(goldBlobs[i].xc, goldBlobs[i].yc, bestType + 1, 1))
                    counters[bestType]++;
                    
                excludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            
            // Utiliza intervalos pré-calculados para comparação rápida
            const float d10Lower = DIAM_10CENT * (1.0f - tolerance);
            const float d10Upper = DIAM_10CENT * (1.0f + tolerance);
            const float d20Lower = DIAM_20CENT * (1.0f - tolerance);
            const float d20Upper = DIAM_20CENT * (1.0f + tolerance);
            const float d50Lower = DIAM_50CENT * (1.0f - tolerance);
            const float d50Upper = DIAM_50CENT * (1.0f + tolerance);
            
            // Condições otimizadas com lógica mais simples
            if (diameter >= d10Lower && diameter <= d10Upper) {
                if (!trackCoin(goldBlobs[i].xc, goldBlobs[i].yc, 4, 1)) {
                    counters[3]++;
                    printf("[MOEDA] 10 cêntimos | €0.10 | Diâm: %.1f | Área: %d | Circularidade: %.2f\n",
                          diameter, goldBlobs[i].area, circularity);
                }
                
                excludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            else if (diameter >= d20Lower && diameter <= d20Upper) {
                if (!trackCoin(goldBlobs[i].xc, goldBlobs[i].yc, 5, 1)) {
                    counters[4]++;
                    printf("[MOEDA] 20 cêntimos | €0.20 | Diâm: %.1f | Área: %d | Circularidade: %.2f\n",
                          diameter, goldBlobs[i].area, circularity);
                }
                
                excludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
            else if (diameter >= d50Lower && diameter <= d50Upper) {
                if (!trackCoin(goldBlobs[i].xc, goldBlobs[i].yc, 6, 1)) {
                    counters[5]++;
                    printf("[MOEDA] 50 cêntimos | €0.50 | Diâm: %.1f | Área: %d | Circularidade: %.2f\n",
                          diameter, goldBlobs[i].area, circularity);
                }
                
                excludeCoin(excludeList, goldBlobs[i].xc, goldBlobs[i].yc, 0);
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief Deteta moedas de Euro (1, 2 euros)
 *
 * Esta função analisa blobs para identificar moedas de Euro, que são bicolores
 * e maiores. Utiliza um algoritmo de dois passos: primeiro procura moedas completas
 * com boa circularidade, depois procura moedas parciais que podem estar parcialmente
 * visíveis na imagem.
 * 
 * @param blob Ponteiro para o blob atual em análise
 * @param euroBlobs Array de blobs candidatos a moedas de Euro
 * @param neuroBlobs Número de blobs candidatos a moedas de Euro
 * @param excludeList Lista de moedas já excluídas da análise
 * @param counters Array de contadores para cada tipo de moeda
 * @param distThresholdSq Limiar de distância ao quadrado para associação entre blobs
 * @return true se uma moeda de Euro for identificada, false caso contrário
 */
bool detectEuroCoins(OVC *blob, OVC *euroBlobs, int neuroBlobs, 
                   int *excludeList, int *counters, int distThresholdSq) {
    if (!blob || !euroBlobs || neuroBlobs <= 0)
        return false;
    
    // Constantes
    const int MAX_VALID_AREA = 100000;  
    const int MIN_VALID_AREA = 6000;
    
    // Rastreia os melhores candidatos
    int bestCompleteIndex = -1;
    float bestCompleteDiameter = 0.0f;
    float bestCompleteCircularity = 0.0f;
    
    int bestPartialIndex = -1;
    float bestPartialDiameter = 0.0f;
    int bestPartialArea = 0;
    
    // Passagem única para encontrar os melhores candidatos
    for (int i = 0; i < neuroBlobs; i++) {
        // Verificação rápida de área
        if (euroBlobs[i].area < MIN_VALID_AREA || euroBlobs[i].area > MAX_VALID_AREA)
            continue;

        const float diameter = getDiameter(&euroBlobs[i]);
        const float circularity = getCircularity(&euroBlobs[i]);
        
        // Moedas de Euro completas
        if (diameter >= 175.0f && diameter <= 210.0f && circularity > 0.75f) {
            if (bestCompleteIndex == -1 || circularity > bestCompleteCircularity) {
                bestCompleteIndex = i;
                bestCompleteDiameter = diameter;
                bestCompleteCircularity = circularity;
            }
        }
        // Moedas de Euro parciais
        else if (circularity > 0.65f && euroBlobs[i].width >= 130 && euroBlobs[i].height >= 130) {
            if (bestPartialIndex == -1 || euroBlobs[i].area > bestPartialArea) {
                bestPartialIndex = i;
                bestPartialDiameter = diameter;
                bestPartialArea = euroBlobs[i].area;
            }
        }
    }
    
    // Processa primeiro a moeda de Euro completa
    if (bestCompleteIndex >= 0) {
        // Determina o tipo de Euro
        const bool is2Euro = (bestCompleteDiameter >= 185.0f);
        const int coinType = is2Euro ? 8 : 7;
        const int counterIdx = is2Euro ? 7 : 6;
        
        // Corrige moedas douradas identificadas incorretamente
        correctGoldCoins(euroBlobs[bestCompleteIndex].xc, 
                        euroBlobs[bestCompleteIndex].yc, 
                        counters);
        
        // Contabiliza se ainda não foi contada
        if (!trackCoin(euroBlobs[bestCompleteIndex].xc, 
                     euroBlobs[bestCompleteIndex].yc, 
                     coinType, 1)) {
            counters[counterIdx]++;
            
            // Regista informações detalhadas da moeda
            const float diameter = bestCompleteDiameter;
            const float circularity = bestCompleteCircularity;
            const int area = euroBlobs[bestCompleteIndex].area;
            
            if (is2Euro) {
                printf("[MOEDA] 2 Euros | €2.00 | Diâm: %.1f | Área: %d | Circ: %.2f\n",
                      diameter, area, circularity);
            } else {
                printf("[MOEDA] 1 Euro | €1.00 | Diâm: %.1f | Área: %d | Circ: %.2f\n",
                      diameter, area, circularity);
            }
        }
        
        excludeCoin(excludeList, euroBlobs[bestCompleteIndex].xc, 
                  euroBlobs[bestCompleteIndex].yc, 0);
        return true;
    }
    
    // Processa moeda de Euro parcial se for significativa
    else if (bestPartialIndex >= 0 && bestPartialArea >= 14000) {
        const int coinType = 8;  
        const int counterIdx = 7;
        
        // Corrige moedas douradas identificadas incorretamente
        correctGoldCoins(euroBlobs[bestPartialIndex].xc, 
                       euroBlobs[bestPartialIndex].yc, 
                       counters);
        
        // Contabiliza se ainda não foi contada
        if (!trackCoin(euroBlobs[bestPartialIndex].xc, 
                     euroBlobs[bestPartialIndex].yc, 
                     coinType, 1)) {
            counters[counterIdx]++;
            
            // Regista informações detalhadas da moeda
            printf("[MOEDA] 2 Euros (parcial) | €2.00 | Diâm: %.1f | Área: %d | Circ: %.2f\n",
                  bestPartialDiameter, bestPartialArea, 
                  getCircularity(&euroBlobs[bestPartialIndex]));
        }
        
        excludeCoin(excludeList, euroBlobs[bestPartialIndex].xc, 
                  euroBlobs[bestPartialIndex].yc, 0);
        return true;
    }
    
    return false;
}

#ifdef __cplusplus
}
#endif
