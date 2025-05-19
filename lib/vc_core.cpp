/**
 * @file vc_core.cpp
 * @brief Funções essenciais para processamento de imagem.
 *
 * Este ficheiro implementa funções fundamentais para manipulação de imagens,
 * incluindo criação, libertação, leitura e escrita de ficheiros de imagem,
 * e várias operações de processamento básicas como conversão entre formatos
 * de cor e manipulações morfológicas.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "vc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Cria uma nova imagem
 *
 * Esta função aloca memória para uma nova estrutura IVC e para os dados
 * da imagem. Inicializa todos os campos da estrutura com os parâmetros
 * fornecidos.
 *
 * @param width Largura da imagem em pixels
 * @param height Altura da imagem em pixels
 * @param channels Número de canais da imagem (1=cinzento, 3=RGB)
 * @param levels Número máximo de níveis de intensidade (normalmente 255)
 * @return Ponteiro para a estrutura IVC criada, ou NULL em caso de erro
 */
IVC *createImage(int width, int height, int channels, int levels) {
    IVC *image = (IVC *) malloc(sizeof(IVC));

    if(image == NULL) return NULL;
    if((levels <= 0) || (levels > 255)) return NULL;

    image->width = width;
    image->height = height;
    image->channels = channels;
    image->levels = levels;
    image->bytesperline = image->width * image->channels;
    image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

    if(image->data == NULL) {
        return freeImage(image);
    }

    return image;
}

/**
 * @brief Liberta a memória alocada para uma imagem
 *
 * Esta função liberta a memória alocada para os dados da imagem
 * e para a estrutura IVC, prevenindo fugas de memória.
 *
 * @param image Ponteiro para a estrutura IVC a ser libertada
 * @return NULL sempre, para facilitar a atribuição após libertação
 */
IVC *freeImage(IVC *image) {
    if(image != NULL) {
        if(image->data != NULL) {
            free(image->data);
            image->data = NULL;
        }

        free(image);
        image = NULL;
    }

    return image;
}

// Função auxiliar para leitura de ficheiros PBM/PGM/PPM
static char *getPBMToken(FILE *file, char *tok, int len) {
    char *t;
    int c;

    for(;;) {
        while(isspace(c = getc(file)));
        if(c != '#') break;
        do c = getc(file);
        while((c != '\n') && (c != EOF));
        if(c == EOF) break;
    }

    t = tok;

    if(c != EOF) {
        do {
            *t++ = c;
            c = getc(file);
        } while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

        if(c == '#') ungetc(c, file);
    }

    *t = 0;

    return tok;
}

// Função auxiliar para conversão entre formatos de bits e bytes
static long int ucharToBit(unsigned char *datauchar, unsigned char *databit, int width, int height) {
    int x, y;
    int countbits;
    long int pos, counttotalbytes;
    unsigned char *p = databit;

    *p = 0;
    countbits = 1;
    counttotalbytes = 0;

    for(y=0; y<height; y++) {
        for(x=0; x<width; x++) {
            pos = width * y + x;

            if(countbits <= 8) {
                *p |= (datauchar[pos] == 0) << (8 - countbits);
                countbits++;
            }
            
            if((countbits > 8) || (x == width - 1)) {
                p++;
                *p = 0;
                countbits = 1;
                counttotalbytes++;
            }
        }
    }

    return counttotalbytes;
}

static void bitToUchar(unsigned char *databit, unsigned char *datauchar, int width, int height) {
    int x, y;
    int countbits;
    long int pos;
    unsigned char *p = databit;

    countbits = 1;

    for(y=0; y<height; y++) {
        for(x=0; x<width; x++) {
            pos = width * y + x;

            if(countbits <= 8) {
                datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
                countbits++;
            }
            
            if((countbits > 8) || (x == width - 1)) {
                p++;
                countbits = 1;
            }
        }
    }
}

/**
 * @brief Lê uma imagem a partir de um ficheiro
 *
 * Esta função lê ficheiros nos formatos PBM, PGM ou PPM (binários e ASCII),
 * que são formatos padrão para imagens em níveis de cinzento e cores RGB.
 * Processa o cabeçalho do ficheiro e aloca memória para a imagem.
 *
 * @param filename Nome do ficheiro a ser lido
 * @return Ponteiro para a estrutura IVC com a imagem carregada, ou NULL em caso de erro
 */
IVC *readImage(char *filename) {
    FILE *file = NULL;
    IVC *image = NULL;
    unsigned char *tmp;
    char tok[20];
    long int size, sizeofbinarydata;
    int width, height, channels;
    int levels = 255;
    int v;

    if((file = fopen(filename, "rb")) != NULL) {
        getPBMToken(file, tok, sizeof(tok));

        if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }
        else if(strcmp(tok, "P5") == 0) channels = 1;
        else if(strcmp(tok, "P6") == 0) channels = 3;
        else {
            #ifdef VC_DEBUG
            printf("ERRO -> readImage():\n\tO ficheiro não é PBM, PGM ou PPM válido.\n\tNúmero mágico incorreto!\n");
            #endif

            fclose(file);
            return NULL;
        }

        if(levels == 1) {
            // Formato PBM (binário)
            if(sscanf(getPBMToken(file, tok, sizeof(tok)), "%d", &width) != 1 ||
               sscanf(getPBMToken(file, tok, sizeof(tok)), "%d", &height) != 1) {
                #ifdef VC_DEBUG
                printf("ERRO -> readImage():\n\tO ficheiro não é PBM válido.\n\tDimensões inválidas!\n");
                #endif

                fclose(file);
                return NULL;
            }

            image = createImage(width, height, channels, levels);
            if(image == NULL) return NULL;

            sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
            tmp = (unsigned char *) malloc(sizeofbinarydata);
            if(tmp == NULL) return 0;

            #ifdef VC_DEBUG
            printf("\ncanais=%d largura=%d altura=%d níveis=%d\n", image->channels, image->width, image->height, levels);
            #endif

            if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata) {
                #ifdef VC_DEBUG
                printf("ERRO -> readImage():\n\tFim prematuro do ficheiro.\n");
                #endif

                freeImage(image);
                fclose(file);
                free(tmp);
                return NULL;
            }

            bitToUchar(tmp, image->data, image->width, image->height);

            free(tmp);
        }
        else {
            // Formato PGM/PPM
            if(sscanf(getPBMToken(file, tok, sizeof(tok)), "%d", &width) != 1 ||
               sscanf(getPBMToken(file, tok, sizeof(tok)), "%d", &height) != 1 ||
               sscanf(getPBMToken(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255) {
                #ifdef VC_DEBUG
                printf("ERRO -> readImage():\n\tO ficheiro não é PGM ou PPM válido.\n\tDimensões ou níveis inválidos!\n");
                #endif

                fclose(file);
                return NULL;
            }

            image = createImage(width, height, channels, levels);
            if(image == NULL) return NULL;

            #ifdef VC_DEBUG
            printf("\ncanais=%d largura=%d altura=%d níveis=%d\n", image->channels, image->width, image->height, levels);
            #endif

            size = image->width * image->height * image->channels;

            if((v = fread(image->data, sizeof(unsigned char), size, file)) != size) {
                #ifdef VC_DEBUG
                printf("ERRO -> readImage():\n\tFim prematuro do ficheiro.\n");
                #endif

                freeImage(image);
                fclose(file);
                return NULL;
            }
        }

        fclose(file);
    }
    else {
        #ifdef VC_DEBUG
        printf("ERRO -> readImage():\n\tFicheiro não encontrado.\n");
        #endif
    }

    return image;
}

/**
 * @brief Escreve uma imagem para um ficheiro
 *
 * Esta função grava a imagem em formato PBM, PGM ou PPM, dependendo do
 * número de canais e níveis de intensidade da imagem. Gera os cabeçalhos
 * apropriados e escreve os dados para o ficheiro.
 *
 * @param filename Nome do ficheiro a ser criado
 * @param image Ponteiro para a estrutura IVC com a imagem a ser gravada
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int writeImage(char *filename, IVC *image) {
    FILE *file = NULL;
    unsigned char *tmp;
    long int totalbytes, sizeofbinarydata;
    int i, channels;
    
    if(image == NULL) return 0;
    if((image->width <= 0) || (image->height <= 0) || (image->data == NULL)) return 0;
    if(filename == NULL) return 0;
    
    if((file = fopen(filename, "wb")) != NULL) {
        if(image->levels == 1) {
            // Escreve ficheiro PBM
            fprintf(file, "P4\n");
            fprintf(file, "%d %d\n", image->width, image->height);
            
            sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
            tmp = (unsigned char *)malloc(sizeofbinarydata);
            if(tmp == NULL) {
                fclose(file);
                return 0;
            }
            
            totalbytes = ucharToBit(image->data, tmp, image->width, image->height);
            
            if(totalbytes != sizeofbinarydata) {
                #ifdef VC_DEBUG
                printf("ERRO -> writeImage():\n\tErro ao converter para ficheiro PBM.\n");
                #endif
                
                free(tmp);
                fclose(file);
                return 0;
            }
            
            if((i = fwrite(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata) {
                #ifdef VC_DEBUG
                printf("ERRO -> writeImage():\n\tErro ao escrever ficheiro PBM.\n");
                #endif
                
                free(tmp);
                fclose(file);
                return 0;
            }
            
            free(tmp);
        }
        else {
            channels = image->channels;
            
            // Escreve ficheiro PGM ou PPM
            if(channels == 1)
                fprintf(file, "P5\n");
            else
                fprintf(file, "P6\n");
                
            fprintf(file, "%d %d\n", image->width, image->height);
            fprintf(file, "%d\n", image->levels);
            
            totalbytes = image->width * image->height * channels;
            
            if((i = fwrite(image->data, sizeof(unsigned char), totalbytes, file)) != totalbytes) {
                #ifdef VC_DEBUG
                printf("ERRO -> writeImage():\n\tErro ao escrever ficheiro PGM/PPM.\n");
                #endif
                
                fclose(file);
                return 0;
            }
        }
        
        fclose(file);
        return 1;
    }
    
    return 0;
}

/**
 * @brief Converte uma imagem do formato BGR para RGB
 *
 * Esta função troca os canais B e R de uma imagem colorida, mantendo o canal G.
 * Útil para converter entre formatos de cores diferentes como os usados pelo
 * OpenCV (BGR) e padrões mais comuns como RGB.
 * 
 * @param src Ponteiro para a imagem de origem
 * @param dst Ponteiro para a imagem de destino
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int bgr2rgb(IVC *src, IVC *dst) {
    unsigned char *datasrc = (unsigned char *)src->data;
    unsigned char *datadst = (unsigned char *)dst->data;
    int x, y, pos;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) {
        return 0;
    }

    if (channels != 3) {
        return 0;
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;

            datadst[pos] = datasrc[pos + 2];     // R <- B
            datadst[pos + 1] = datasrc[pos + 1]; // G <- G
            datadst[pos + 2] = datasrc[pos];     // B <- R
        }
    }

    return 1;
}

/**
 * @brief Converte uma imagem RGB para níveis de cinzento
 *
 * Esta função converte uma imagem colorida em RGB para uma imagem em
 * níveis de cinzento, utilizando a média ponderada dos canais conforme
 * a perceção humana de luminosidade (0.299*R + 0.587*G + 0.114*B).
 * 
 * @param src Ponteiro para a imagem RGB de origem
 * @param dst Ponteiro para a imagem em níveis de cinzento de destino
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int rgb2gray(IVC *src, IVC *dst) {
    unsigned char *datasrc = (unsigned char *)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char *datadst = (unsigned char *)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int x, y;
    long int pos_src, pos_dst;
    float rf, gf, bf;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
        return 0;
    if ((src->width != dst->width) || (src->height != dst->height))
        return 0;
    if ((src->channels != 3) || (dst->channels != 1))
        return 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;

            rf = (float)datasrc[pos_src];
            gf = (float)datasrc[pos_src + 1];
            bf = (float)datasrc[pos_src + 2];

            datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
        }
    }
    return 1;
}

/**
 * @brief Converte RGB para HSV e segmenta por tipo de moeda
 *
 * Esta função converte cada pixel de uma imagem RGB para o espaço de cores HSV
 * e segmenta a imagem com base em intervalos específicos de matiz, saturação e valor
 * para identificar diferentes tipos de moedas: douradas, cobre ou de Euro.
 * 
 * @param srcdst Ponteiro para a imagem (entrada em RGB, saída segmentada)
 * @param segmentType Tipo de segmentação: 0=moedas douradas, 1=moedas de cobre, 2=moedas de Euro
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int rgb2hsv(IVC *srcdst, int segmentType) {
    unsigned char *data = (unsigned char *)srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->bytesperline;
    int channels = srcdst->channels;
    int i, size = width * height * channels;
    
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL) || channels != 3)
        return 0;

    // Usa um único ciclo e otimiza o cálculo de HSV
    for (i = 0; i < size; i += channels) {
        // Conversão RGB para HSV
        const float r = (float)data[i];
        const float g = (float)data[i + 1];
        const float b = (float)data[i + 2];
        
        // Encontra máximo e mínimo numa única passagem
        const float rgb_max = fmaxf(r, fmaxf(g, b));
        const float rgb_min = fminf(r, fminf(g, b));
        
        // Valor é sempre o máximo
        const float value = rgb_max;
        
        // Valores padrão para casos especiais
        float hue = 0.0f;
        float saturation = 0.0f;
        
        // Só calcula saturação e matiz se o valor não for zero
        if (value > 0.0f) {
            // Cálculo da saturação
            saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;
            
            // Só calcula matiz se a saturação não for zero
            if (saturation > 0.0f) {
                // Cálculo simplificado da matiz
                const float delta = rgb_max - rgb_min;
                
                if (rgb_max == r) {
                    hue = (g >= b) ? 
                        60.0f * (g - b) / delta : 
                        360.0f + 60.0f * (g - b) / delta;
                } else if (rgb_max == g) {
                    hue = 120.0f + 60.0f * (b - r) / delta;
                } else { // rgb_max == b
                    hue = 240.0f + 60.0f * (r - g) / delta;
                }
            }
        }
        
        // Segmentação baseada no tipo - otimizada para menos operações
        if (segmentType == 0) { // Moedas douradas
            const bool isGold = (hue >= 35.0f && hue <= 95.0f && 
                                saturation >= 40.0f && value >= 40.0f);
            
            // Define todos os canais de uma vez usando operador ternário para evitar ramificações
            data[i] = data[i+1] = data[i+2] = isGold ? 255 : 0;
        }
        else if (segmentType == 1) { // Moedas de cobre
            const bool isCopper = (hue >= 10.0f && hue <= 45.0f && saturation >= 70.0f);
            
            data[i] = data[i+1] = data[i+2] = isCopper ? 255 : 0;
        }
        else if (segmentType == 2) { // Moedas de Euro
            // Combina deteção de prata e dourado numa única condição
            const bool isEuroCoin = (saturation < 60.0f && value > 80.0f && value < 240.0f) || 
                                   (hue >= 20.0f && hue <= 95.0f && saturation >= 35.0f && value >= 35.0f);
            
            data[i] = data[i+1] = data[i+2] = isEuroCoin ? 255 : 0;
        }
    }

    return 1;
}

/**
 * @brief Converte uma imagem em níveis de cinzento para binária
 *
 * Esta função aplica um limiar para transformar uma imagem em níveis de cinzento
 * numa imagem binária. Pixels com intensidade acima do limiar serão brancos (255),
 * e pixels com intensidade abaixo serão pretos (0).
 * 
 * @param src Ponteiro para a imagem em níveis de cinzento de origem
 * @param dst Ponteiro para a imagem binária de destino
 * @param threshold Valor do limiar (0-255)
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int gray2binary(IVC *src, IVC *dst, int threshold) {
    unsigned char *datasrc = (unsigned char *)src->data;
    unsigned char *datadst = (unsigned char *)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline_src = src->bytesperline;
    int bytesperline_dst = dst->bytesperline;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    int x, y;
    long int pos_src, pos_dst;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;

            if (datasrc[pos_src] >= threshold)
                datadst[pos_dst] = 255;
            else
                datadst[pos_dst] = 0;
        }
    }

    return 1;
}

/**
 * @brief Aplica operação de abertura binária (erosão seguida de dilatação)
 *
 * Esta função realiza uma abertura morfológica numa imagem binária, que é útil para
 * remover pequenos objetos e ruídos, mantendo a forma e tamanho dos objetos maiores.
 * Consiste numa erosão seguida de uma dilatação com o mesmo elemento estruturante.
 * 
 * @param src Ponteiro para a imagem binária de origem
 * @param dst Ponteiro para a imagem binária de destino
 * @param kernel Tamanho do elemento estruturante (kernel)
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int binaryOpen(IVC *src, IVC *dst, int kernel) {
    // Cria imagem temporária para resultado intermediário
    IVC *temp = createImage(src->width, src->height, src->channels, src->levels);
    if (!temp) return 0;
    
    // Erosão binária
    unsigned char *datasrc = (unsigned char *)src->data;
    unsigned char *datatemp = (unsigned char *)temp->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, kx, ky;
    long int pos;
    bool flagWhite;
    
    // Verifica entrada
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) {
        freeImage(temp);
        return 0;
    }
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) {
        freeImage(temp);
        return 0;
    }
    if (channels != 1) {
        freeImage(temp);
        return 0;
    }
    
    // Garante que o tamanho do kernel é válido (ímpar)
    if (kernel % 2 == 0) kernel++;
    int kernelOffset = kernel / 2;
    
    // Primeiro passo: Erosão
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;
            
            flagWhite = true;
            
            // Aplica kernel
            for (ky = -kernelOffset; ky <= kernelOffset; ky++) {
                for (kx = -kernelOffset; kx <= kernelOffset; kx++) {
                    int ny = y + ky;
                    int nx = x + kx;
                    
                    // Ignora pixels fora dos limites da imagem
                    if (ny < 0 || ny >= height || nx < 0 || nx >= width) continue;
                    
                    if (datasrc[ny * bytesperline + nx * channels] == 0) {
                        flagWhite = false;
                        break;
                    }
                }
                if (!flagWhite) break;
            }
            
            datatemp[pos] = flagWhite ? 255 : 0;
        }
    }
    
    // Segundo passo: Dilatação
    unsigned char *datadst = (unsigned char *)dst->data;
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;
            
            flagWhite = false;
            
            // Aplica kernel
            for (ky = -kernelOffset; ky <= kernelOffset; ky++) {
                for (kx = -kernelOffset; kx <= kernelOffset; kx++) {
                    int ny = y + ky;
                    int nx = x + kx;
                    
                    // Ignora pixels fora dos limites da imagem
                    if (ny < 0 || ny >= height || nx < 0 || nx >= width) continue;
                    
                    if (datatemp[ny * bytesperline + nx * channels] == 255) {
                        flagWhite = true;
                        break;
                    }
                }
                if (flagWhite) break;
            }
            
            datadst[pos] = flagWhite ? 255 : 0;
        }
    }
    
    freeImage(temp);
    return 1;
}

/**
 * @brief Aplica operação de fecho binário (dilatação seguida de erosão)
 *
 * Esta função realiza um fecho morfológico numa imagem binária, útil para fechar
 * pequenos buracos e unir objetos próximos mantendo a forma geral. Consiste numa
 * dilatação seguida de uma erosão com o mesmo elemento estruturante.
 * 
 * @param src Ponteiro para a imagem binária de origem
 * @param dst Ponteiro para a imagem binária de destino
 * @param kernel Tamanho do elemento estruturante (kernel)
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int binaryClose(IVC *src, IVC *dst, int kernel) {
    // Cria imagem temporária para resultado intermediário
    IVC *temp = createImage(src->width, src->height, src->channels, src->levels);
    if (!temp) return 0;
    
    // Dilatação binária
    unsigned char *datasrc = (unsigned char *)src->data;
    unsigned char *datatemp = (unsigned char *)temp->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, kx, ky;
    long int pos;
    bool flagWhite;
    
    // Verifica entrada
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) {
        freeImage(temp);
        return 0;
    }
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) {
        freeImage(temp);
        return 0;
    }
    if (channels != 1) {
        freeImage(temp);
        return 0;
    }
    
    // Garante que o tamanho do kernel é válido (ímpar)
    if (kernel % 2 == 0) kernel++;
    int kernelOffset = kernel / 2;
    
    // Primeiro passo: Dilatação
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;
            
            flagWhite = false;
            
            // Aplica kernel
            for (ky = -kernelOffset; ky <= kernelOffset; ky++) {
                for (kx = -kernelOffset; kx <= kernelOffset; kx++) {
                    int ny = y + ky;
                    int nx = x + kx;
                    
                    // Ignora pixels fora dos limites da imagem
                    if (ny < 0 || ny >= height || nx < 0 || nx >= width) continue;
                    
                    if (datasrc[ny * bytesperline + nx * channels] == 255) {
                        flagWhite = true;
                        break;
                    }
                }
                if (flagWhite) break;
            }
            
            datatemp[pos] = flagWhite ? 255 : 0;
        }
    }
    
    // Segundo passo: Erosão
    unsigned char *datadst = (unsigned char *)dst->data;
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;
            
            flagWhite = true;
            
            // Aplica kernel
            for (ky = -kernelOffset; ky <= kernelOffset; ky++) {
                for (kx = -kernelOffset; kx <= kernelOffset; kx++) {
                    int ny = y + ky;
                    int nx = x + kx;
                    
                    // Ignora pixels fora dos limites da imagem
                    if (ny < 0 || ny >= height || nx < 0 || nx >= width) continue;
                    
                    if (datatemp[ny * bytesperline + nx * channels] == 0) {
                        flagWhite = false;
                        break;
                    }
                }
                if (!flagWhite) break;
            }
            
            datadst[pos] = flagWhite ? 255 : 0;
        }
    }
    
    freeImage(temp);
    return 1;
}

/**
 * @brief Deteção de contornos utilizando o operador de Prewitt
 *
 * Esta função implementa a deteção de contornos utilizando o operador de Prewitt,
 * que calcula gradientes horizontais e verticais para encontrar transições abruptas
 * de intensidade na imagem. É útil para identificar bordas de objetos.
 * 
 * @param src Ponteiro para a imagem em níveis de cinzento de origem
 * @param dst Ponteiro para a imagem binária de destino com os contornos
 * @param threshold Limiar para identificação dos contornos
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int detectEdges(IVC *src, IVC *dst, float threshold) {
    unsigned char *data = (unsigned char *)src->data;
    int width = src->width;
    int height = src->height;
    int byteperline = src->width * src->channels;
    int channels = src->channels;
    int x, y;
    long int pos;
    long int posA, posB, posC, posD, posE, posF, posG, posH;
    double mag, mx, my;

    if ((width <= 0) || (height <= 0) || (src->data == NULL))
        return 0;
    if (channels != 1)
        return 0;

    for (y = 1; y < height; y++) {
        for (x = 1; x < width; x++) {
            pos = y * byteperline + x * channels;

            posA = (y - 1) * byteperline + (x - 1) * channels;
            posB = (y - 1) * byteperline + (x)*channels;
            posC = (y - 1) * byteperline + (x + 1) * channels;
            posD = (y)*byteperline + (x - 1) * channels;
            posE = (y)*byteperline + (x + 1) * channels;
            posF = (y + 1) * byteperline + (x - 1) * channels;
            posG = (y + 1) * byteperline + (x)*channels;
            posH = (y + 1) * byteperline + (x + 1) * channels;
            
            // Gradiente horizontal
            mx = (double)((-1 * data[posA]) + (1 * data[posC]) + 
                         (-2 * data[posD]) + (2 * data[posE]) + 
                         (-1 * data[posF]) + (1 * data[posH])) / 3.0;
                         
            // Gradiente vertical
            my = (double)((-1 * data[posA]) + (1 * data[posF]) + 
                         (-2 * data[posB]) + (2 * data[posG]) + 
                         (-1 * data[posC]) + (1 * data[posH])) / 3.0;

            // Magnitude do gradiente
            mag = sqrt((mx * mx) + (my * my));

            // Aplica limiar
            if (mag > threshold)
                dst->data[pos] = 255;
            else
                dst->data[pos] = 0;
        }
    }
    return 1;
}

#ifdef __cplusplus
}
#endif
