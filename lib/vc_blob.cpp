/**
 * @file vc_blob.cpp
 * @brief Funções para análise e rotulagem de blobs (componentes conectados).
 *
 * Este ficheiro contém implementações de funções que permitem identificar
 * e analisar componentes conectados (blobs) em imagens binárias. Estas
 * funções são essenciais para processamento e análise de objetos em visão
 * computacional.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rotula componentes conectados (blobs) numa imagem binária
 *
 * Esta função implementa o algoritmo de rotulagem de componentes conectados
 * para identificar objetos distintos numa imagem binária. O algoritmo percorre
 * a imagem pixel a pixel e atribui rótulos aos pixels dos objetos encontrados.
 * Pixels adjacentes que pertencem ao mesmo objeto recebem o mesmo rótulo.
 *
 * O processo segue os seguintes passos:
 * 1. Copia a imagem binária de origem para o destino
 * 2. Normaliza os valores binários (0 para fundo, 255 para objetos)
 * 3. Limpa as bordas da imagem para evitar problemas com objetos parcialmente visíveis
 * 4. Realiza a primeira passagem, atribuindo rótulos iniciais e construindo uma tabela de equivalência
 * 5. Reorganiza os rótulos baseado nas equivalências encontradas
 * 6. Conta o número de objetos distintos
 *
 * @param src Ponteiro para a imagem binária de origem
 * @param dst Ponteiro para a imagem de destino onde os rótulos serão armazenados
 * @param nlabels Ponteiro para uma variável que receberá o número de objetos encontrados
 * @return Array de estruturas OVC contendo informações básicas sobre cada blob identificado
 */
OVC *blobLabel(IVC *src, IVC *dst, int *nlabels) {
    unsigned char *datasrc = (unsigned char *)src->data;
    unsigned char *datadst = (unsigned char *)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->width * src->channels;
    int channels = src->channels;
    int x, y, a, b;
    long int i, size;
    long int posX, posA, posB, posC, posD;
    int maxlabels = width * height / 4;
    
    if (maxlabels < 256)
        maxlabels = 256;
        
    // Aloca memória para a tabela de rótulos
    int* labeltable = (int*)calloc(maxlabels, sizeof(int));
    if (!labeltable)
        return NULL;
        
    int label = 1; // Rótulo inicial
    int num, tmplabel;
    OVC *blobs; // Array de blobs a retornar

    // Verificação de erros nos parâmetros de entrada
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return NULL;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
    if (channels != 1) return NULL;

    // Copia os dados da imagem binária para a imagem de destino
    memcpy(datadst, datasrc, bytesperline * height);

    // Garante valores binários adequados (0 para fundo, 255 para primeiro plano)
    for (i = 0, size = bytesperline * height; i < size; i++) {
        if (datadst[i] != 0) datadst[i] = 255;
    }

    // Limpa as bordas da imagem para evitar problemas
    for (y = 0; y < height; y++) {
        datadst[y * bytesperline + 0] = 0;
        datadst[y * bytesperline + (width - 1)] = 0;
    }
    for (x = 0; x < width; x++) {
        datadst[0 * bytesperline + x] = 0;
        datadst[(height - 1) * bytesperline + x] = 0;
    }

    // Executa a rotulagem dos componentes conectados
    for (y = 1; y < height - 1; y++) {
        for (x = 1; x < width - 1; x++) {
            // Posições do kernel:
            // A B C
            // D X
            posA = (y - 1) * bytesperline + (x - 1); // A
            posB = (y - 1) * bytesperline + x;       // B
            posC = (y - 1) * bytesperline + (x + 1); // C
            posD = y * bytesperline + (x - 1);       // D
            posX = y * bytesperline + x;             // X

            // Se o pixel atual for de primeiro plano
            if (datadst[posX] != 0) {
                if ((datadst[posA] == 0) && (datadst[posB] == 0) && 
                    (datadst[posC] == 0) && (datadst[posD] == 0)) {
                    // Atribui um novo rótulo se não houver vizinhos rotulados
                    datadst[posX] = label;
                    labeltable[label] = label;
                    label++;
                }
                else {
                    // Encontra o rótulo mínimo entre os vizinhos
                    num = 255;
                    if (datadst[posA] != 0) num = labeltable[datadst[posA]];
                    if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) 
                        num = labeltable[datadst[posB]];
                    if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) 
                        num = labeltable[datadst[posC]];
                    if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) 
                        num = labeltable[datadst[posD]];

                    // Atribui o rótulo mínimo ao pixel atual
                    datadst[posX] = num;
                    labeltable[num] = num;

                    // Atualiza a tabela de rótulos para manter a consistência
                    if (datadst[posA] != 0) {
                        if (labeltable[datadst[posA]] != num) {
                            tmplabel = labeltable[datadst[posA]];
                            for (a = 1; a < label; a++) {
                                if (labeltable[a] == tmplabel) {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                    if (datadst[posB] != 0) {
                        if (labeltable[datadst[posB]] != num) {
                            tmplabel = labeltable[datadst[posB]];
                            for (a = 1; a < label; a++) {
                                if (labeltable[a] == tmplabel) {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                    if (datadst[posC] != 0) {
                        if (labeltable[datadst[posC]] != num) {
                            tmplabel = labeltable[datadst[posC]];
                            for (a = 1; a < label; a++) {
                                if (labeltable[a] == tmplabel) {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                    if (datadst[posD] != 0) {
                        if (labeltable[datadst[posD]] != num) {
                            tmplabel = labeltable[datadst[posD]];
                            for (a = 1; a < label; a++) {
                                if (labeltable[a] == tmplabel) {
                                    labeltable[a] = num;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Re-rotula a imagem usando os rótulos finais
    for (y = 1; y < height - 1; y++) {
        for (x = 1; x < width - 1; x++) {
            posX = y * bytesperline + x;
            if (datadst[posX] != 0) {
                datadst[posX] = labeltable[datadst[posX]];
            }
        }
    }

    // Conta o número de blobs distintos
    // Passo 1: Remove rótulos duplicados
    for (a = 1; a < label - 1; a++) {
        for (b = a + 1; b < label; b++) {
            if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
        }
    }
    
    // Passo 2: Conta os rótulos e organiza a tabela
    *nlabels = 0;
    for (a = 1; a < label; a++) {
        if (labeltable[a] != 0) {
            labeltable[*nlabels] = labeltable[a];
            (*nlabels)++;
        }
    }

    // Se não foram encontrados blobs
    if (*nlabels == 0) {
        free(labeltable);
        return NULL;
    }

    // Cria a lista de blobs para retorno
    blobs = (OVC *)calloc(*nlabels, sizeof(OVC));
    if (blobs != NULL) {
        for (a = 0; a < *nlabels; a++) 
            blobs[a].label = labeltable[a];
    }
    
    free(labeltable);
    return blobs;
}

/**
 * @brief Extrai informações detalhadas sobre os blobs rotulados
 *
 * Esta função analisa uma imagem já rotulada e calcula diversas propriedades
 * para cada blob identificado, incluindo:
 * 
 * - Área (número de pixels que compõem o blob)
 * - Perímetro (número de pixels na fronteira do blob)
 * - Centro de massa (coordenadas médias dos pixels do blob)
 * - Caixa delimitadora (coordenadas e dimensões do menor retângulo que contém o blob)
 * 
 * O algoritmo percorre cada pixel da imagem uma vez, atualizando as estatísticas
 * de cada blob quando encontra um pixel correspondente. Verifica se um pixel
 * faz parte do perímetro ao examinar seus vizinhos imediatos.
 *
 * @param src Ponteiro para a imagem rotulada de entrada
 * @param blobs Array de estruturas OVC a serem preenchidas com informações
 * @param nblobs Número de blobs identificados
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int blobInfo(IVC *src, OVC *blobs, int nblobs) {
    unsigned char *data = (unsigned char *)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, i;
    long int pos;
    int xmin, ymin, xmax, ymax;
    long int sumx, sumy;

    // Verificação de erros nos parâmetros de entrada
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (channels != 1) return 0;

    // Calcula as propriedades de cada blob
    for (i = 0; i < nblobs; i++) {
        xmin = width - 1;
        ymin = height - 1;
        xmax = 0;
        ymax = 0;

        sumx = 0;
        sumy = 0;

        blobs[i].area = 0;
        blobs[i].perimeter = 0;

        // Processa cada pixel da imagem
        for (y = 1; y < height - 1; y++) {
            for (x = 1; x < width - 1; x++) {
                pos = y * bytesperline + x;

                if (data[pos] == blobs[i].label) {
                    // Incrementa a área do blob
                    blobs[i].area++;

                    // Acumula coordenadas para cálculo do centro de massa
                    sumx += x;
                    sumy += y;

                    // Atualiza os limites da caixa delimitadora
                    if (xmin > x) xmin = x;
                    if (ymin > y) ymin = y;
                    if (xmax < x) xmax = x;
                    if (ymax < y) ymax = y;

                    // Verifica se é um pixel de perímetro
                    // (se pelo menos um dos vizinhos não pertence ao blob)
                    if ((data[pos - 1] != blobs[i].label) || 
                        (data[pos + 1] != blobs[i].label) || 
                        (data[pos - bytesperline] != blobs[i].label) || 
                        (data[pos + bytesperline] != blobs[i].label)) {
                        blobs[i].perimeter++;
                    }
                }
            }
        }

        // Define as propriedades da caixa delimitadora
        blobs[i].x = xmin;
        blobs[i].y = ymin;
        blobs[i].width = (xmax - xmin) + 1;
        blobs[i].height = (ymax - ymin) + 1;

        // Calcula o centro de massa usando VC_MAX para evitar divisão por zero
        blobs[i].xc = sumx / VC_MAX(blobs[i].area, 1);
        blobs[i].yc = sumy / VC_MAX(blobs[i].area, 1);
    }

    return 1;
}

#ifdef __cplusplus
}
#endif
