/**
 * @file vc_coin.cpp
 * @brief Funções consolidadas para deteção e classificação de moedas.
 *
 * Este ficheiro contém implementações de funções que permitem detetar e
 * classificar moedas em imagens, com suporte para rastreamento entre frames
 * e funções de utilidade para cálculos geométricos.
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

// Constantes para diâmetros de moedas (em pixels)
const float DIAM_1CENT = 122.0f;
const float DIAM_2CENT = 135.0f;
const float DIAM_5CENT = 152.0f;
const float DIAM_10CENT = 143.0f;
const float DIAM_20CENT = 160.0f;
const float DIAM_50CENT = 174.0f;
const float DIAM_1EURO = 185.0f;
const float DIAM_2EURO = 195.0f;
const float BASE_TOLERANCE = 0.08f;

// Variáveis globais para rastreamento de moedas
static int frameCountValue = 0;  // Contador de frames
int MAX_TRACKED_COINS = 150;
int detectedCoins[150][5] = {{0}}; // [x, y, tipoMoeda, frameDetectado, contabilizada]

/**
 * @brief Incrementa ou reinicia o contador de frames
 *
 * Esta função é utilizada para manter um contador global de frames,
 * que serve como referência temporal para o sistema de rastreamento
 * de moedas.
 *
 * @param reset Se for 1, reinicia o contador; se for 0, incrementa-o
 */
void frameCounter(int reset) {
    if (reset) {
        frameCountValue = 0;
    } else {
        frameCountValue++;
        // Reinicia em 1000 para evitar overflow
        if (frameCountValue > 1000) frameCountValue = 0;
    }
}

/**
 * @brief Obtém o valor atual do contador de frames
 *
 * @return O valor atual do contador de frames
 */
int getFrameCount() {
    return frameCountValue;
}

/**
 * @brief Calcula a circularidade de um blob
 * 
 * Esta função implementa o cálculo de circularidade para um blob,
 * que é usado como medida de quão próximo o objeto está de uma forma circular.
 * A circularidade é calculada como 4π × área / perímetro².
 * 
 * @param blob Ponteiro para a estrutura do blob
 * @return Valor de circularidade entre 0 e 1, onde 1 representa um círculo perfeito
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
 * @brief Calcula o diâmetro de um blob circular
 *
 * Estima o diâmetro de um blob assumindo que este tem forma circular,
 * utilizando a fórmula derivada da área de um círculo (A = πr²).
 * 
 * @param blob Ponteiro para a estrutura do blob
 * @return Diâmetro estimado do blob em pixels
 */
float getDiameter(OVC *blob) {
    return 2.0f * sqrtf((float)blob->area / 3.14159f);
}

/**
 * @brief Calcula uma tolerância adaptativa baseada na proximidade às bordas da imagem
 *
 * Esta função aumenta a tolerância para a classificação de moedas quando estas
 * estão próximas às bordas da imagem, compensando distorções de perspetiva.
 * 
 * @param xc Coordenada X do centro do blob
 * @param yc Coordenada Y do centro do blob
 * @param frameWidth Largura da imagem
 * @param frameHeight Altura da imagem
 * @return Valor de tolerância adaptado
 */
float adaptTolerance(int xc, int yc, int frameWidth, int frameHeight) {
    float tolerance = BASE_TOLERANCE;
    const int edgeMargin = 50;
    
    // Encontra a distância mínima a qualquer borda
    float minDist = fminf(fminf((float)xc, (float)(frameWidth - xc)), 
                         fminf((float)yc, (float)(frameHeight - yc)));
    
    // Aumenta a tolerância próximo às bordas
    if (minDist < edgeMargin)
        tolerance *= 1.0f + 0.5f * (1.0f - (minDist / edgeMargin));
    
    return tolerance;
}

/**
 * @brief Regista e verifica se uma moeda já foi detetada anteriormente
 *
 * Esta função implementa um mecanismo de rastreamento de moedas entre frames,
 * evitando que a mesma moeda seja contada múltiplas vezes ao longo do tempo.
 * 
 * @param x Coordenada X do centro da moeda
 * @param y Coordenada Y do centro da moeda
 * @param coinType Tipo de moeda (1=1c, 2=2c, 3=5c, etc.)
 * @param countIt Indica se deve marcar a moeda como contabilizada
 * @return 1 se a moeda já foi contada, 0 caso contrário
 */
int trackCoin(int x, int y, int coinType, int countIt) {
    const int distThreshold = (coinType >= 7) ? 75 : 50;
    const int distThresholdSq = distThreshold * distThreshold;
    const int frameMemory = (coinType >= 7) ? 120 : 60;
    const int currentFrame = getFrameCount();
    
    int existingIndex = -1;
    int emptyIndex = -1;
    
    // Trata moedas de Euro substituindo moedas douradas
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
    
    // Procura posição existente ou vazia
    for (int i = 0; i < MAX_TRACKED_COINS; i++) {
        if (detectedCoins[i][0] == 0 && detectedCoins[i][1] == 0) {
            if (emptyIndex == -1) emptyIndex = i;
            continue;
        }
        
        const int dx = detectedCoins[i][0] - x;
        const int dy = detectedCoins[i][1] - y;
        const int distSq = dx*dx + dy*dy;
        
        if (distSq <= distThresholdSq) {
            // Janela de frames válida
            if ((currentFrame - detectedCoins[i][3] < frameMemory) || 
                detectedCoins[i][3] > currentFrame) {
                
                existingIndex = i;
                
                // Trata Euro substituindo moedas douradas
                if (coinType >= 7 && detectedCoins[i][2] >= 4 && detectedCoins[i][2] <= 6) {
                    detectedCoins[i][2] = coinType;
                }
                
                break;
            }
        }
    }
    
    // Atualiza entrada existente ou adiciona nova entrada
    if (existingIndex >= 0) {
        // Atualiza posição e timestamp
        detectedCoins[existingIndex][0] = x;
        detectedCoins[existingIndex][1] = y;
        detectedCoins[existingIndex][3] = currentFrame;
        
        // Trata contabilização
        if (countIt && detectedCoins[existingIndex][4] == 0) {
            detectedCoins[existingIndex][4] = 1;
            return 0;
        }
        
        return detectedCoins[existingIndex][4];
    }
    
    // Adiciona nova entrada se possível
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
 * @brief Obtém o último tipo de moeda detetado numa determinada localização
 *
 * Esta função consulta o histórico de moedas detetadas e retorna o tipo
 * da moeda mais próxima à localização especificada.
 * 
 * @param x Coordenada X a verificar
 * @param y Coordenada Y a verificar
 * @return Tipo da moeda (1-8) ou 0 se nenhuma moeda for encontrada
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
 * @brief Adiciona ou remove uma moeda da lista de exclusão
 *
 * Esta função gerencia uma lista de moedas que devem ser ignoradas
 * durante o processamento, útil para evitar reanálise de moedas já
 * classificadas ou para marcar regiões problemáticas.
 * 
 * @param excludeList Ponteiro para a lista de exclusão
 * @param xc Coordenada X do centro da moeda
 * @param yc Coordenada Y do centro da moeda
 * @param option 0 para adicionar à lista, 1 para remover da lista
 * @return 0 após conclusão
 */
int excludeCoin(int *excludeList, int xc, int yc, int option) {
    if (!excludeList) return 0;
    
    const int PROXIMITY_THRESHOLD_SQ = 30 * 30;
    
    if (option == 0) {
        // Adiciona à lista de exclusão
        for (int i = 0; i < MAX_COINS; i++) {
            if (excludeList[i * 2] == 0 && excludeList[i * 2 + 1] == 0) {
                excludeList[i * 2] = xc;
                excludeList[i * 2 + 1] = yc;
                break;
            }
        }
    }
    else if (option == 1) {
        // Remove da lista de exclusão
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
 * @brief Corrige moedas douradas identificadas incorretamente quando moedas de Euro são detetadas
 *
 * Esta função evita contagem duplicada quando uma moeda de Euro é confundida
 * com uma moeda dourada devido a condições de iluminação ou oclusão parcial.
 * 
 * @param x Coordenada X do centro da moeda de Euro
 * @param y Coordenada Y do centro da moeda de Euro
 * @param counters Array de contadores para cada tipo de moeda
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
            
            // Diminui o contador se necessário
            if (counters[goldCounterIdx] > 0)
                counters[goldCounterIdx]--;
                
            // Limpa entrada
            memset(&detectedCoins[i][0], 0, 5 * sizeof(int));
            break;
        }
    }
}

/**
 * @brief Desenha as moedas com etiquetas no frame
 *
 * Esta função visualiza as moedas detetadas na imagem original,
 * desenhando círculos coloridos e rótulos que indicam o tipo de moeda.
 * As cores variam conforme o tipo: cobre, dourado ou bicolor (Euro).
 * 
 * @param frame Ponteiro para a imagem onde as moedas serão desenhadas
 * @param goldBlobs Array de blobs de moedas douradas
 * @param copperBlobs Array de blobs de moedas de cobre
 * @param euroBlobs Array de blobs de moedas de Euro
 * @param nGoldBlobs Número de blobs dourados
 * @param nCopperBlobs Número de blobs de cobre
 * @param nEuroBlobs Número de blobs de Euro
 */
void drawCoins(IVC *frame, OVC *goldBlobs, OVC *copperBlobs, OVC *euroBlobs,
              int nGoldBlobs, int nCopperBlobs, int nEuroBlobs) {
    unsigned char *data = (unsigned char*)frame->data;
    const int bytesperline = frame->bytesperline;
    const int channels = frame->channels;
    const int width = frame->width;
    const int height = frame->height;

    // Desenha primeiro as moedas de Euro (têm prioridade)
    if (euroBlobs && nEuroBlobs > 0) {
        // Encontra o melhor blob de Euro em uma única passagem
        OVC bestEuro = {0};
        bool foundValidEuro = false;
        
        float bestCircularity = 0.0f;
        int bestArea = 0;
        
        // Constantes
        const int MAX_VALID_AREA = 100000;
        const int MIN_VALID_AREA = 12000;
        
        // Primeiro procura moedas de Euro completas
        for (int i = 0; i < nEuroBlobs; i++) {
            if (euroBlobs[i].area > MAX_VALID_AREA)
                continue;
                
            const float diameter = getDiameter(&euroBlobs[i]);
            const float circularity = getCircularity(&euroBlobs[i]);
            
            // Deteção de Euro completo
            if (diameter >= 175.0f && diameter <= 210.0f && circularity >= 0.75f) {
                // Ignora se esta for uma posição de moeda dourada
                const int lastType = getCoinTypeAtLocation(euroBlobs[i].xc, euroBlobs[i].yc);
                if (lastType >= 4 && lastType <= 6)
                    continue;
                    
                memcpy(&bestEuro, &euroBlobs[i], sizeof(OVC));
                bestEuro.label = 999;
                foundValidEuro = true;
                break; // Primeira correspondência é suficiente
            }
        }
        
        // Se nenhuma moeda completa for encontrada, procura por partes válidas
        if (!foundValidEuro) {
            for (int i = 0; i < nEuroBlobs; i++) {
                if (euroBlobs[i].area < MIN_VALID_AREA || euroBlobs[i].area > MAX_VALID_AREA)
                    continue;
                    
                const float circularity = getCircularity(&euroBlobs[i]);
                
                if (circularity >= 0.65f && 
                    euroBlobs[i].width >= 130 && euroBlobs[i].height >= 130 &&
                    euroBlobs[i].area > bestArea) {
                    
                    // Ignora se esta for uma posição de moeda dourada
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
        
        // Desenha moedas de Euro se encontradas
        if (foundValidEuro) {
            const float diameter = getDiameter(&bestEuro);
            const int radius = (int)(diameter / 2.0f);
            const int centerX = bestEuro.xc;
            const int centerY = bestEuro.yc;
            
            // Ignora se fora dos limites
            if (centerX < 0 || centerX >= width || centerY < 0 || centerY >= height)
                return;
                
            // Desenha círculo para moeda de Euro
            const unsigned char blueColor[3] = {255, 130, 0}; // Azul em BGR
            
            // Desenha círculo simples
            for (int angle = 0; angle < 360; angle++) {
                float radAngle = angle * (M_PI / 180.0f);
                int x = (int)(centerX + radius * cosf(radAngle));
                int y = (int)(centerY + radius * sinf(radAngle));
                
                // Verifica limites da imagem
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    int pos = y * bytesperline + x * channels;
                    data[pos] = blueColor[0];     // B
                    data[pos + 1] = blueColor[1]; // G
                    data[pos + 2] = blueColor[2]; // R
                }
            }
            
            // Desenha ponto central e rótulos
            bool is2Euro = (diameter >= 185.0f || bestEuro.area >= 14000);
            
            // Desenha um ponto central simples
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
            
            // Desenha um rótulo simplificado
            int textX = centerX - 10;
            int textY = centerY + 30;
            if (textX >= 0 && textX < width - 20 && textY >= 0 && textY < height) {
                // Desenha um fundo preto para o texto
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
                
                // Desenha texto branco (versão simplificada)
                for (int y = textY - 5; y <= textY + 5; y++) {
                    for (int x = textX; x <= textX + (is2Euro ? 15 : 10); x++) {
                        if (x >= 0 && x < width && y >= 0 && y < height) {
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
    
    // Desenha moedas de cobre
    if (copperBlobs && nCopperBlobs > 0) {
        for (int i = 0; i < nCopperBlobs; i++) {
            if (copperBlobs[i].area < 7000 || copperBlobs[i].label == 0)
                continue;
            
            const float diameter = getDiameter(&copperBlobs[i]);
            const int radius = (int)(diameter / 2.0f);
            const int centerX = copperBlobs[i].xc;
            const int centerY = copperBlobs[i].yc;
            
            // Ignora se fora dos limites
            if (centerX < 0 || centerX >= width || centerY < 0 || centerY >= height)
                continue;
                
            // Desenha círculo para moeda de cobre
            const unsigned char copperColor[3] = {0, 80, 255}; // Laranja/Cobre em BGR
            
            // Desenha círculo simples
            for (int angle = 0; angle < 360; angle += 3) { // Incremento de 3 graus para velocidade
                float radAngle = angle * (M_PI / 180.0f);
                int x = (int)(centerX + radius * cosf(radAngle));
                int y = (int)(centerY + radius * sinf(radAngle));
                
                // Verifica limites da imagem
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    int pos = y * bytesperline + x * channels;
                    data[pos] = copperColor[0];     // B
                    data[pos + 1] = copperColor[1]; // G
                    data[pos + 2] = copperColor[2]; // R
                }
            }
            
            // Marca o centro
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
            
            // Adiciona um rótulo simplificado
            const char *label = (diameter < 130) ? "1c" : 
                              (diameter < 145) ? "2c" : "5c";
            
            // Renderização de texto muito básica no centro
            int textX = centerX - 5;
            int textY = centerY + 20;
            if (textX >= 0 && textX < width - 10 && textY >= 0 && textY < height) {
                // Cria um pequeno fundo preto
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
                
                // Desenha texto branco (muito simplificado)
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
    
    // Desenha moedas douradas
    if (goldBlobs && nGoldBlobs > 0) {
        for (int i = 0; i < nGoldBlobs; i++) {
            if (goldBlobs[i].area < 7000 || goldBlobs[i].label == 0)
                continue;
            
            const float diameter = getDiameter(&goldBlobs[i]);
            const int radius = (int)(diameter / 2.0f);
            const int centerX = goldBlobs[i].xc;
            const int centerY = goldBlobs[i].yc;
            
            // Ignora se fora dos limites
            if (centerX < 0 || centerX >= width || centerY < 0 || centerY >= height)
                continue;
                
            // Desenha círculo para moeda dourada
            const unsigned char goldColor[3] = {0, 215, 255}; // Amarelo/Dourado em BGR
            
            // Desenha círculo simplificado
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
            
            // Marca o centro
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
            
            // Rótulo simples baseado no tamanho
            const char *label = (diameter < 150) ? "10c" : 
                              (diameter < 170) ? "20c" : "50c";
            
            // Renderização de texto muito básica
            int textX = centerX - 8;
            int textY = centerY + 20;
            if (textX >= 0 && textX < width - 15 && textY >= 0 && textY < height) {
                // Fundo preto
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
                
                // Contorno do texto branco
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
