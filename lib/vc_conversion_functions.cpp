//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @file vc_conversion_functions.cpp
 * @brief Funções para conversão entre espaços de cor.
 *
 * Este ficheiro contém funções para converter imagens entre diferentes
 * espaços de cor (RGB, escala de cinzentos, HSV, etc.).
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

// Include all headers before any extern "C" blocks
#include <stdio.h>
#include <math.h>
#include <opencv2/core/types_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include "vc.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Aplica o efeito de negativo numa imagem em escala de cinzentos.
     *
     * Esta função inverte os valores de intensidade de cada píxel da imagem,
     * transformando-a no seu negativo (255 - valor_original).
     *
     * @param srcdst Imagem de entrada e saída em escala de cinzentos (1 canal)
     * @return 1 se a operação for bem sucedida, 0 em caso de erro
     */
    int vc_gray_negative(IVC *srcdst)
    {
        // ***************************************************************
        // * Esta função inverte os valores de uma imagem em tons de cinza,
        // * calculando o complemento (255 - valor) para cada píxel.
        // *
        // * O processo percorre todos os píxeis da imagem e substitui cada
        // * valor pelo seu complemento (255 menos o valor atual).
        // * 
        // * Esta operação é útil para melhorar a visualização de certas
        // * características que possam ser difíceis de perceber na
        // * imagem original.
        // ***************************************************************
        
        unsigned char *data = (unsigned char *)srcdst->data;
        int width = srcdst->width;
        int height = srcdst->height;
        int bytesperline = srcdst->width * srcdst->channels;
        int channel = srcdst->channels;
        int x, y;
        long int pos;

        if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
            return 0;
        if (channel != 1)
            return 0;

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                pos = y * bytesperline + x * channel;

                data[pos] = 255 - data[pos];
            }
        }

        return 1;
    }

    /**
     * @brief Aplica o efeito de negativo numa imagem RGB.
     *
     * Esta função inverte os valores de cada canal RGB de todos os píxeis da imagem,
     * transformando-a no seu negativo (255 - valor_original para cada canal).
     *
     * @param srcdst Imagem de entrada e saída RGB (3 canais)
     * @return 1 se a operação for bem sucedida, 0 em caso de erro
     */
    int vc_rgb_negative(IVC *srcdst)
    {
        // ***************************************************************
        // * Esta função aplica um efeito negativo a uma imagem RGB,
        // * invertendo independentemente os valores dos três canais de cor.
        // *
        // * Para cada píxel da imagem, cada componente (R, G e B) é invertida
        // * através da operação (255 - valor_componente).
        // *
        // * A inversão de cores produz uma imagem onde as áreas claras tornam-se
        // * escuras e vice-versa, enquanto as cores mudam para as suas
        // * complementares (ex: vermelho torna-se ciano).
        // ***************************************************************
        
        unsigned char *data = (unsigned char *)srcdst->data;
        int width = srcdst->width;
        int height = srcdst->height;
        int bytesperline = srcdst->width * srcdst->channels;
        int channel = srcdst->channels;
        int x, y;
        long int pos;

        if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
            return 0;
        if (channel != 3)
            return 0;

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                pos = y * bytesperline + x * channel;

                data[pos] = 255 - data[pos];
                data[pos + 1] = 255 - data[pos + 1];
                data[pos + 2] = 255 - data[pos + 2];
            }
        }

        return 1;
    }

    /**
     * @brief Converte uma imagem RGB para escala de cinzentos.
     *
     * Esta função aplica a fórmula ponderada (R*0.229 + G*0.587 + B*0.114) para
     * converter os valores RGB em valores de intensidade em escala de cinzentos.
     *
     * @param src Imagem de origem em formato RGB (3 canais)
     * @param dst Imagem de destino em escala de cinzentos (1 canal)
     * @return 1 se a operação for bem sucedida, 0 em caso de erro
     */
    int vc_rgb_to_gray(IVC *src, IVC *dst)
    {
        // ***************************************************************
        // * Esta função converte uma imagem colorida RGB para escala de cinzentos,
        // * utilizando a fórmula padrão de luminância perceptual.
        // *
        // * A conversão usa pesos diferentes para cada canal de cor para
        // * respeitar a sensibilidade do olho humano às diferentes cores:
        // * - Vermelho (R): 22.9% da contribuição
        // * - Verde (G): 58.7% da contribuição (maior sensibilidade)
        // * - Azul (B): 11.4% da contribuição
        // *
        // * Para cada píxel, os valores RGB são multiplicados pelos respetivos
        // * pesos e somados para formar um único valor de intensidade em tons de cinza.
        // ***************************************************************
        
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

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                pos_src = y * bytesperline_src + x * channels_src;
                pos_dst = y * bytesperline_dst + x * channels_dst;

                rf = (float)datasrc[pos_src];
                gf = (float)datasrc[pos_src + 1];
                bf = (float)datasrc[pos_src + 2];

                datadst[pos_dst] = (unsigned char)((rf * 0.229) + (gf * 0.587) + (bf * 0.114));
            }
        }
        return 1;
    }

    /**
     * @brief Converte uma imagem RGB para o espaço de cores HSV e aplica segmentação.
     *
     * Esta função converte os valores RGB para HSV e, dependendo do parâmetro 'segmentType',
     * segmenta diferentes tipos de moedas com base nos seus valores HSV.
     *
     * @param srcdst Imagem de entrada RGB e saída da segmentação
     * @param segmentType Tipo de segmentação: 0 para moedas douradas, 1 para moedas de cobre, 2 para moedas de Euro
     * @return 1 se a operação for bem sucedida, 0 em caso de erro
     */
    int vc_rgb_to_hsv(IVC *srcdst, int segmentType)
    {
        // ***************************************************************
        // * Esta função converte uma imagem do espaço de cores RGB para HSV
        // * e depois aplica segmentação para distinguir diferentes tipos de moedas.
        // *
        // * O algoritmo funciona da seguinte forma:
        // * 1. Converte cada píxel de RGB para HSV:
        // *    - Hue (matiz): Representa a cor em ângulos de 0° a 360°
        // *    - Saturation (saturação): Intensidade da cor de 0 a 255
        // *    - Value (valor): Brilho da cor de 0 a 255
        // * 
        // * 2. Depois aplica critérios de segmentação com base no tipo solicitado:
        // *    - segmentType=0: Segmenta moedas douradas (10, 20, 50 cêntimos)
        // *      com valores de matiz entre 40° e 85°, saturação e valor acima de 50
        // *    - segmentType=1: Segmenta moedas de cobre (1, 2, 5 cêntimos)
        // *      com valores de matiz entre 15° e 40° e saturação acima de 80
        // *    - segmentType=2: Segmenta moedas de Euro (1 e 2 euros)
        // *      com baixa saturação para a parte prateada e valores específicos
        // *      de matiz para capturar o anel dourado
        // *
        // * 3. Os píxeis que correspondem aos critérios são marcados como brancos (255),
        // *    enquanto os restantes tornam-se pretos (0), criando uma máscara binária.
        // *
        // * Este processo é fundamental para isolar as moedas de diferentes materiais
        // * para posterior classificação pelo seu tamanho e valor.
        // ***************************************************************
        
        unsigned char *data = (unsigned char *)srcdst->data;
        int width = srcdst->width;
        int height = srcdst->height;
        int bytesperline = srcdst->bytesperline;
        int channels = srcdst->channels;
        int i, size = width * height * channels;
        
        if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL) || channels != 3)
            return 0;

        // Use a single loop and optimize HSV calculation
        for (i = 0; i < size; i += channels) {
            // RGB to HSV conversion
            const float r = (float)data[i];
            const float g = (float)data[i + 1];
            const float b = (float)data[i + 2];
            
            // Find max and min in one pass
            const float rgb_max = fmaxf(r, fmaxf(g, b));
            const float rgb_min = fminf(r, fminf(g, b));
            
            // Value is always the maximum
            const float value = rgb_max;
            
            // Default values for edge cases
            float hue = 0.0f;
            float saturation = 0.0f;
            
            // Only compute saturation and hue if value is non-zero
            if (value > 0.0f) {
                // Saturation calculation
                saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;
                
                // Only compute hue if saturation is non-zero
                if (saturation > 0.0f) {
                    // Simplified hue calculation
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
            
            // Segmentation based on type - optimized for fewer operations
            if (segmentType == 0) { // Gold coins
                const bool isGold = (hue >= 35.0f && hue <= 95.0f && 
                                    saturation >= 40.0f && value >= 40.0f);
                
                // Set all channels at once using ternary to avoid branches
                data[i] = data[i+1] = data[i+2] = isGold ? 255 : 0;
            }
            else if (segmentType == 1) { // Copper coins
                const bool isCopper = (hue >= 10.0f && hue <= 45.0f && saturation >= 70.0f);
                
                data[i] = data[i+1] = data[i+2] = isCopper ? 255 : 0;
            }
            else if (segmentType == 2) { // Euro coins
                // Combine silver and gold detection into one condition
                const bool isEuroCoin = (saturation < 60.0f && value > 80.0f && value < 240.0f) || 
                                       (hue >= 20.0f && hue <= 95.0f && saturation >= 35.0f && value >= 35.0f);
                
                data[i] = data[i+1] = data[i+2] = isEuroCoin ? 255 : 0;
            }
        }

        return 1;
    }

    /**
     * @brief Converte uma imagem em escala de cinzentos para RGB com mapeamento de cores.
     *
     * Esta função mapeia os valores de escala de cinzentos para cores RGB,
     * criando uma representação colorida da imagem original em tons de cinza.
     *
     * @param src Imagem de origem em escala de cinzentos (1 canal)
     * @param dst Imagem de destino RGB (3 canais)
     * @return 1 se a operação for bem sucedida, 0 em caso de erro
     */
    int vc_scale_gray_to_rgb(IVC *src, IVC *dst)
    {
        // ***************************************************************
        // * Esta função transforma uma imagem em escala de cinzentos numa 
        // * imagem colorida RGB através de um mapa de cores personalizado.
        // *
        // * O algoritmo divide os valores de cinzento em quatro intervalos
        // * e atribui a cada intervalo um gradiente de cores diferente:
        // * - [0-64]: Gradiente de azul puro a ciano (azul + verde)
        // * - [65-128]: Gradiente de ciano a verde puro
        // * - [129-192]: Gradiente de verde puro a amarelo
        // * - [193-255]: Gradiente de amarelo a vermelho puro
        // *
        // * Isto cria um efeito semelhante a um mapa de calor, útil para visualizar
        // * diferentes intensidades na imagem de forma mais distintas visualmente.
        // *
        // * Esta técnica é frequentemente utilizada em visualização científica
        // * para destacar detalhes difíceis de perceber em imagens monocromáticas.
        // ***************************************************************
        
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

        if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
            return 0;
        if ((src->width != dst->width) || (src->height != dst->height))
            return 0;
        if ((src->channels != 1) || (dst->channels != 3))
            return 0;

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                pos_src = y * bytesperline_src + x * channels_src;
                pos_dst = y * bytesperline_dst + x * channels_dst;

                if (datasrc[pos_src] <= 64)
                {
                    datadst[pos_dst] = 0;
                    datadst[pos_dst + 1] = datasrc[pos_src] * (255 / 64);
                    datadst[pos_dst + 2] = 255;
                }
                else if (datasrc[pos_src] <= 128)
                {
                    datadst[pos_dst] = 0;
                    datadst[pos_dst + 1] = 255;
                    datadst[pos_dst + 2] = 255 - (datasrc[pos_src] - 64) * (255 / 64);
                }
                else if (datasrc[pos_src] <= 192)
                {
                    datadst[pos_dst] = (datasrc[pos_src] - 128) * (255 / 64);
                    datadst[pos_dst + 1] = 255;
                    datadst[pos_dst + 2] = 0;
                }
                else
                {
                    datadst[pos_dst] = 255;
                    datadst[pos_dst + 1] = 255 - (datasrc[pos_src] - 192) * (255 / 64);
                    datadst[pos_dst + 2] = 0;
                }
            }
        }
        return 1;
    }

    /**
     * @brief Converte uma imagem BGR para RGB.
     *
     * Esta função troca os canais azul e vermelho de uma imagem,
     * útil para conversão entre formatos OpenCV (BGR) e padrão RGB.
     *
     * @param src Imagem de origem em formato BGR (3 canais)
     * @param dst Imagem de destino em formato RGB (3 canais)
     * @return 0 após a operação
     */
    int vc_bgr_to_rgb(IVC *src, IVC *dst)
    {
        // ***************************************************************
        // * Esta função converte uma imagem do formato BGR (Blue-Green-Red)
        // * para o formato RGB (Red-Green-Blue).
        // *
        // * Esta conversão é necessária porque a biblioteca OpenCV utiliza
        // * internamente o formato BGR para armazenar imagens coloridas, enquanto
        // * a maioria dos outros sistemas e algoritmos utilizam o formato RGB.
        // *
        // * O algoritmo simplesmente troca a posição dos canais vermelho (R) e
        // * azul (B) para cada píxel da imagem, mantendo o canal verde (G) na
        // * mesma posição:
        // *   - Canal R de saída recebe o canal B de entrada
        // *   - Canal G mantém-se igual
        // *   - Canal B de saída recebe o canal R de entrada
        // *
        // * Esta conversão é essencial para garantir que as cores sejam 
        // * corretamente interpretadas nos algoritmos subsequentes de processamento.
        // ***************************************************************
        
        unsigned char *datasrc = (unsigned char *)src->data;
        unsigned char *datadst = (unsigned char *)dst->data;
        int x, y, pos;
        int width = src->width;
        int height = src->height;
        int bytesperline = src->bytesperline;
        int channels = src->channels;

        if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
        {
            return 0;
        }

        if (channels != 3)
        {
            return 0;
        }

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                pos = y * bytesperline + x * channels;

                datadst[pos] = datasrc[pos + 2];     // R <- B
                datadst[pos + 1] = datasrc[pos + 1]; // G <- G
                datadst[pos + 2] = datasrc[pos];     // B <- R
            }
        }

        return 0;
    }

#ifdef __cplusplus
}
#endif
