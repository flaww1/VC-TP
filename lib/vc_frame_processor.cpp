/**
 * @file vc_frame_processor.cpp
 * @brief Funções principais para processamento de frames e deteção de moedas.
 *
 * Este ficheiro implementa o processamento completo de frames para deteção,
 * classificação e contagem de moedas em sequências de vídeo. Coordena o fluxo
 * de processamento desde a conversão de cor até à análise de objetos.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h> // Necessário para INT_MAX

#include "vc.h"

#ifdef __cplusplus
extern "C" {
#endif

// Referência externa ao array de moedas rastreadas
extern int MAX_TRACKED_COINS;
extern int detectedCoins[150][5]; // [x, y, tipoMoeda, frameDetectado, contabilizada]

/**
 * @brief Processa um frame para detetar e classificar moedas
 *
 * Esta função implementa o fluxo completo de processamento para deteção de moedas
 * num frame de vídeo. O processo inclui:
 * - Conversão de espaços de cor (BGR para RGB, RGB para HSV)
 * - Segmentação das imagens para diferentes tipos de moedas (douradas, cobre, euro)
 * - Deteção e análise de blobs
 * - Classificação das moedas detetadas
 * - Visualização dos resultados no frame original
 * - Contabilização das moedas por tipo e valor
 *
 * A função utiliza várias imagens temporárias para processar diferentes características
 * das moedas, explorando propriedades de cor e forma para a classificação.
 * 
 * @param frame Frame principal para análise (entrada e saída para visualização)
 * @param frame2 Frame secundário para análise complementar
 * @param excludeList Lista de coordenadas de moedas a excluir da análise
 * @param coinCounts Array com contadores para cada tipo de moeda
 */
void processFrame(IVC *frame, IVC *frame2, int *excludeList, int *coinCounts) {
    // Incrementa o contador de frames
    frameCounter(0);
    
    // Validação básica dos parâmetros
    if (!frame || !frame2 || !excludeList || !coinCounts) 
        return;

    // Obtém dimensões do frame
    const int width = frame->width;
    const int height = frame->height;
    const int channels = frame->channels;
    const int size = width * height * channels;
    
    // Aloca buffers para imagens temporárias
    IVC *rgbImage = createImage(width, height, channels, 255);
    IVC *hsvImage = createImage(width, height, channels, 255);
    IVC *hsvImage2 = createImage(width, height, channels, 255);
    IVC *hsvImage3 = createImage(width, height, channels, 255);
    
    IVC *grayImage = createImage(width, height, 1, 255);
    IVC *grayImage2 = createImage(width, height, 1, 255);
    IVC *grayImage3 = createImage(width, height, 1, 255);
    IVC *grayImage4 = createImage(width, height, 1, 255);
    
    IVC *binaryImage = createImage(width, height, 1, 255);
    IVC *binaryImage2 = createImage(width, height, 1, 255);
    IVC *binaryImage3 = createImage(width, height, 1, 255);
    IVC *binaryImage4 = createImage(width, height, 1, 255);
    
    // Verifica falhas na alocação
    if (!rgbImage || !hsvImage || !hsvImage2 || !hsvImage3 ||
        !grayImage || !grayImage2 || !grayImage3 || !grayImage4 ||
        !binaryImage || !binaryImage2 || !binaryImage3 || !binaryImage4) {
        
        // Liberta imagens alocadas
        if (rgbImage) freeImage(rgbImage);
        if (hsvImage) freeImage(hsvImage);
        if (hsvImage2) freeImage(hsvImage2);
        if (hsvImage3) freeImage(hsvImage3);
        if (grayImage) freeImage(grayImage);
        if (grayImage2) freeImage(grayImage2);
        if (grayImage3) freeImage(grayImage3);
        if (grayImage4) freeImage(grayImage4);
        if (binaryImage) freeImage(binaryImage);
        if (binaryImage2) freeImage(binaryImage2);
        if (binaryImage3) freeImage(binaryImage3);
        if (binaryImage4) freeImage(binaryImage4);
        
        return;
    }

    // Converte BGR para RGB de forma eficiente
    bgr2rgb(frame, rgbImage);
    
    // Processa moedas douradas (10c, 20c, 50c)
    memcpy(hsvImage->data, rgbImage->data, size);
    rgb2hsv(hsvImage, 0);  
    rgb2gray(hsvImage, grayImage2);
    gray2binary(grayImage2, binaryImage2, 110);
    binaryOpen(binaryImage2, grayImage2, 7);

    // Processa moedas de cobre (1c, 2c, 5c)
    bgr2rgb(frame2, rgbImage);
    memcpy(hsvImage2->data, rgbImage->data, size);
    rgb2hsv(hsvImage2, 1);
    rgb2gray(hsvImage2, grayImage3);
    gray2binary(grayImage3, binaryImage3, 80);
    binaryOpen(binaryImage3, grayImage3, 3);

    // Processa moedas de Euro (1€, 2€)
    bgr2rgb(frame, rgbImage);
    memcpy(hsvImage3->data, rgbImage->data, size);
    rgb2hsv(hsvImage3, 2);
    rgb2gray(hsvImage3, grayImage4);
    gray2binary(grayImage4, binaryImage4, 90);
    binaryOpen(binaryImage4, grayImage4, 3);
    
    // Extrai imagem em níveis de cinzento para deteção geral de blobs
    rgb2gray(rgbImage, grayImage);
    gray2binary(grayImage, binaryImage, 150);
    binaryOpen(binaryImage, binaryImage, 3);
    binaryClose(binaryImage, binaryImage, 5);

    // Deteção de blobs
    int nlabels = 0, nlabels2 = 0, nlabels3 = 0, nlabels4 = 0;
    OVC *blobs = NULL, *blobs2 = NULL, *blobs3 = NULL, *blobs4 = NULL;
    
    // Só prossegue se conseguir extrair os blobs principais
    blobs = blobLabel(binaryImage, binaryImage, &nlabels);
    if (blobs && nlabels > 0) {
        blobInfo(binaryImage, blobs, nlabels);
        
        // Processa blobs de moedas douradas
        blobs2 = blobLabel(grayImage2, grayImage2, &nlabels2);
        if (blobs2 && nlabels2 > 0) {
            blobInfo(grayImage2, blobs2, nlabels2);
        }
        
        // Processa blobs de moedas de cobre
        blobs3 = blobLabel(grayImage3, grayImage3, &nlabels3);
        if (blobs3 && nlabels3 > 0) {
            blobInfo(grayImage3, blobs3, nlabels3);
        }
        
        // Processa blobs de moedas de Euro
        blobs4 = blobLabel(grayImage4, grayImage4, &nlabels4);
        if (blobs4 && nlabels4 > 0) {
            blobInfo(grayImage4, blobs4, nlabels4);
        }
        
        // Processa os objetos detetados - versão simplificada
        for (int i = 0; i < nlabels; i++) {
            // Ignora blobs pequenos
            if (blobs[i].area < 9000 || blobs[i].area >= 30000 || blobs[i].width > 220) {
                continue;
            }
            
            // Constantes
            const int DISTANCE_THRESHOLD_SQ = 30 * 30;
            
            // Verifica se este blob está na lista de exclusão
            bool isExcluded = false;
            for (int j = 0; j < MAX_COINS; j++) {
                if (excludeList[j * 2] == 0 && excludeList[j * 2 + 1] == 0)
                    continue;
                    
                int dx = excludeList[j * 2] - blobs[i].xc;
                int dy = excludeList[j * 2 + 1] - blobs[i].yc;
                int distance_squared = dx * dx + dy * dy;
                
                if (distance_squared <= DISTANCE_THRESHOLD_SQ) {
                    isExcluded = true;
                    break;
                }
            }
            
            if (isExcluded)
                continue;
                
            // Tenta detetar moedas
            bool coinFound = false;
            
            // Tenta detetar moedas de Euro primeiro (têm prioridade)
            if (blobs4 && nlabels4 > 0) {
                coinFound = detectEuroCoins(&blobs[i], blobs4, nlabels4, excludeList, coinCounts, DISTANCE_THRESHOLD_SQ);
            }
            
            // Tenta detetar moedas douradas em segundo
            if (!coinFound && blobs2 && nlabels2 > 0) {
                coinFound = detectGoldCoins(&blobs[i], blobs2, nlabels2, excludeList, coinCounts, DISTANCE_THRESHOLD_SQ);
            }
            
            // Tenta detetar moedas de cobre por último
            if (!coinFound && blobs3 && nlabels3 > 0) {
                coinFound = detectCopperCoins(&blobs[i], blobs3, nlabels3, excludeList, coinCounts, DISTANCE_THRESHOLD_SQ);
            }
        }
        
        // Desenha visualizações no frame
        drawCoins(frame, blobs2, blobs3, blobs4, nlabels2, nlabels3, nlabels4);
    }

    // Mostra resumo das contagens atuais a cada 30 frames
    int currentFrame = getFrameCount();
    if (currentFrame % 30 == 0) {
        float total = coinCounts[0] * 0.01f + coinCounts[1] * 0.02f + 
                     coinCounts[2] * 0.05f + coinCounts[3] * 0.10f + 
                     coinCounts[4] * 0.20f + coinCounts[5] * 0.50f +
                     coinCounts[6] * 1.00f + coinCounts[7] * 2.00f;
        
        printf("\n[RESUMO DE MOEDAS] Frame %d\n", currentFrame);
        printf("1c: %d (%.2f€), 2c: %d (%.2f€), 5c: %d (%.2f€)\n", 
               coinCounts[0], coinCounts[0] * 0.01f,
               coinCounts[1], coinCounts[1] * 0.02f,
               coinCounts[2], coinCounts[2] * 0.05f);
        printf("10c: %d (%.2f€), 20c: %d (%.2f€), 50c: %d (%.2f€)\n", 
               coinCounts[3], coinCounts[3] * 0.10f,
               coinCounts[4], coinCounts[4] * 0.20f,
               coinCounts[5], coinCounts[5] * 0.50f);
        printf("1€: %d (%.2f€), 2€: %d (%.2f€)\n", 
               coinCounts[6], coinCounts[6] * 1.00f,
               coinCounts[7], coinCounts[7] * 2.00f);
        printf("Total de moedas: %d | Valor total: %.2f EUR\n", 
               coinCounts[0] + coinCounts[1] + coinCounts[2] + 
               coinCounts[3] + coinCounts[4] + coinCounts[5] +
               coinCounts[6] + coinCounts[7], total);
    }

    // Limpeza de memória
    if (blobs) free(blobs);
    if (blobs2) free(blobs2);
    if (blobs3) free(blobs3);
    if (blobs4) free(blobs4);
    
    freeImage(rgbImage);
    freeImage(hsvImage);
    freeImage(hsvImage2);
    freeImage(hsvImage3);
    freeImage(grayImage);
    freeImage(grayImage2);
    freeImage(grayImage3);
    freeImage(grayImage4);
    freeImage(binaryImage);
    freeImage(binaryImage2);
    freeImage(binaryImage3);
    freeImage(binaryImage4);
}

#ifdef __cplusplus
}
#endif
