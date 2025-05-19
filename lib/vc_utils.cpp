/**
 * @file vc_utils.cpp
 * @brief Funções utilitárias para processamento de imagem e visão computacional.
 *
 * Este ficheiro contém implementações de funções auxiliares que suportam
 * os algoritmos principais de processamento de imagem, incluindo cálculos
 * geométricos e funções de comparação entre objetos.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Verifica se uma coordenada está dentro dos limites do frame
 * 
 * Esta função determina se um ponto específico está contido dentro das
 * dimensões da imagem, considerando opcionalmente uma margem de segurança.
 * É útil para validar coordenadas antes de aceder a pixels da imagem.
 * 
 * @param x Coordenada X do ponto
 * @param y Coordenada Y do ponto
 * @param width Largura do frame
 * @param height Altura do frame
 * @param margin Margem de segurança opcional (padrão 0)
 * @return true se a coordenada estiver dentro do frame
 */
bool isInFrame(int x, int y, int width, int height, int margin) {
    return (x >= margin && x < width - margin && 
            y >= margin && y < height - margin);
}

/**
 * @brief Calcula a distância Euclidiana entre dois pontos
 * 
 * Esta função implementa a fórmula da distância Euclidiana para determinar
 * a distância exata entre dois pontos no espaço 2D. Utiliza a raiz quadrada,
 * o que a torna mais precisa, mas computacionalmente mais intensiva.
 * 
 * @param x1 Coordenada X do primeiro ponto
 * @param y1 Coordenada Y do primeiro ponto
 * @param x2 Coordenada X do segundo ponto
 * @param y2 Coordenada Y do segundo ponto
 * @return Distância entre os pontos
 */
float distance(int x1, int y1, int x2, int y2) {
    const int dx = x2 - x1;
    const int dy = y2 - y1;
    return sqrtf((float)(dx*dx + dy*dy));
}

/**
 * @brief Calcula o quadrado da distância Euclidiana entre dois pontos
 * 
 * Esta função é uma versão mais eficiente da função de distância, pois evita
 * o cálculo da raiz quadrada. É útil quando apenas se pretende comparar
 * distâncias, sem necessidade de obter o valor exato.
 * 
 * @param x1 Coordenada X do primeiro ponto
 * @param y1 Coordenada Y do primeiro ponto
 * @param x2 Coordenada X do segundo ponto
 * @param y2 Coordenada Y do segundo ponto
 * @return Quadrado da distância entre os pontos
 */
int distanceSquared(int x1, int y1, int x2, int y2) {
    const int dx = x2 - x1;
    const int dy = y2 - y1;
    return dx*dx + dy*dy;
}

/**
 * @brief Calcula a área de interseção entre duas caixas delimitadoras
 * 
 * Esta função determina a área de sobreposição entre dois retângulos,
 * o que é útil para calcular métricas como IoU (Intersection over Union)
 * utilizadas em algoritmos de deteção de objetos.
 * 
 * @param x1 Coordenada X do canto superior esquerdo da primeira caixa
 * @param y1 Coordenada Y do canto superior esquerdo da primeira caixa
 * @param w1 Largura da primeira caixa
 * @param h1 Altura da primeira caixa
 * @param x2 Coordenada X do canto superior esquerdo da segunda caixa
 * @param y2 Coordenada Y do canto superior esquerdo da segunda caixa
 * @param w2 Largura da segunda caixa
 * @param h2 Altura da segunda caixa
 * @return Área de interseção
 */
int intersectionArea(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    int xOverlap = VC_MAX(0, VC_MIN(x1 + w1, x2 + w2) - VC_MAX(x1, x2));
    int yOverlap = VC_MAX(0, VC_MIN(y1 + h1, y2 + h2) - VC_MAX(y1, y2));
    return xOverlap * yOverlap;
}

/**
 * @brief Calcula a Interseção sobre União (IoU) de duas caixas delimitadoras
 * 
 * Esta função implementa o cálculo do IoU, uma métrica fundamental em visão
 * computacional que mede o grau de sobreposição entre duas regiões. O valor
 * resultante varia entre 0 (sem sobreposição) e 1 (sobreposição completa).
 * 
 * @param box1 Primeira caixa delimitadora
 * @param box2 Segunda caixa delimitadora
 * @return Valor de IoU entre 0.0 e 1.0
 */
float calculateIoU(OVC *box1, OVC *box2) {
    // Calcula a área de interseção
    int intersectArea = intersectionArea(
        box1->x, box1->y, box1->width, box1->height,
        box2->x, box2->y, box2->width, box2->height
    );
    
    // Calcula a área de união
    int area1 = box1->width * box1->height;
    int area2 = box2->width * box2->height;
    int unionArea = area1 + area2 - intersectArea;
    
    // Retorna o IoU
    if (unionArea > 0)
        return (float)intersectArea / (float)unionArea;
    return 0.0f;
}

/**
 * @brief Verifica se dois blobs provavelmente representam o mesmo objeto
 * 
 * Esta função utiliza múltiplos critérios, incluindo posição, tamanho e forma,
 * para determinar se dois blobs provavelmente correspondem ao mesmo objeto físico.
 * É útil para rastrear objetos entre frames consecutivos ou para consolidar
 * deteções redundantes.
 * 
 * @param blob1 Primeiro blob
 * @param blob2 Segundo blob
 * @param maxDistSq Distância quadrada máxima permitida
 * @return true se os blobs provavelmente representam o mesmo objeto
 */
bool isSameObject(OVC *blob1, OVC *blob2, int maxDistSq) {
    // Verifica a distância entre os centros
    int distSq = distanceSquared(blob1->xc, blob1->yc, blob2->xc, blob2->yc);
    if (distSq > maxDistSq)
        return false;
    
    // Verifica a similaridade de tamanho
    float sizeDiff = fabsf((float)blob1->area - (float)blob2->area) / 
                     (float)VC_MAX(blob1->area, blob2->area);
    if (sizeDiff > 0.5f)
        return false;
        
    // Verifica a similaridade de forma
    float circ1 = getCircularity(blob1); // Função externa definida em vc_coin.cpp
    float circ2 = getCircularity(blob2);
    if (fabsf(circ1 - circ2) > 0.3f)
        return false;
    
    // Verifica a sobreposição usando IoU
    float iou = calculateIoU(blob1, blob2);
    if (iou > 0.2f)
        return true;
        
    // Se chegámos até aqui e os blobs estão muito próximos, consideramo-los o mesmo
    if (distSq < maxDistSq / 4)
        return true;
        
    return false;
}

#ifdef __cplusplus
}
#endif
