//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @file vc_binary_functions.cpp
 * @brief Funções para processamento de imagens binárias.
 * 
 * Este ficheiro contém funções para binarização, operações morfológicas
 * e processamento de imagens binárias.
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
extern "C" {
#endif

/**
 * @brief Binariza uma imagem em escala de cinzentos utilizando um limiar definido.
 * 
 * Esta função transforma uma imagem em escala de cinzentos numa imagem binária,
 * atribuindo o valor 255 (branco) a píxeis com intensidade superior ao limiar
 * e 0 (preto) aos restantes.
 * 
 * @param src Imagem de origem em escala de cinzentos (1 canal)
 * @param dst Imagem de destino binária (1 canal)
 * @param threshold Valor de limiar para a binarização
 * @return 1 se a operação for bem sucedida, 0 em caso de erro
 */
int vc_gray_to_binary(IVC *src, IVC *dst, int threshold) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y*bytesperline_src + x*channels_src;
			pos_dst = y*bytesperline_dst + x*channels_dst;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

/**
 * @brief Binariza uma imagem em escala de cinzentos utilizando a média global como limiar.
 * 
 * Esta função calcula a média de intensidade de todos os píxeis da imagem e
 * usa esse valor como limiar para a binarização.
 * 
 * @param src Imagem de origem em escala de cinzentos (1 canal)
 * @param dst Imagem de destino binária (1 canal)
 * @return 1 se a operação for bem sucedida, 0 em caso de erro
 */
int vc_gray_to_binary_global_mean(IVC *src, IVC *dst) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;
	float threshold=0;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;

	for ( y = 0; y < height; y++)
	{
		for  (x = 0; x < width; x++)
		{
			pos_src = y*bytesperline_src + x*channels_src;
			threshold += datasrc[pos_src];
		}
	}

	threshold = threshold / (width*height);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y*bytesperline_src + x*channels_src;
			pos_dst = y*bytesperline_dst + x*channels_dst;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}
		}
	}
	return 1;
}

/**
 * @brief Binariza uma imagem em escala de cinzentos utilizando o método do ponto médio.
 * 
 * Esta função aplica uma binarização adaptativa utilizando o ponto médio 
 * (média entre os valores máximo e mínimo) numa vizinhança definida pelo kernel.
 * 
 * @param src Imagem de origem em escala de cinzentos (1 canal)
 * @param dst Imagem de destino binária (1 canal)
 * @param kernel Tamanho do kernel (vizinhança) para calcular o ponto médio
 * @return 1 se a operação for bem sucedida, 0 em caso de erro
 */
int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	float threshold = 0;
	int max, min;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;
	kernel *= 0.5;
	for (y = kernel; y < height-kernel; y++)
	{
		for (x = kernel; x < width-kernel; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;

			for (max=0,min=255,y2 = y-kernel; y2 <= y+kernel; y2++)
			{
				for (x2 = x-kernel; x2 <= x+kernel; x2++)
				{
					pos_src = y2*bytesperline_src + x2*channels_src;
					if (datasrc[pos_src] > max) { max = datasrc[pos_src]; }
					else if (datadst[pos_src] < min) { min = datasrc[pos_src]; }
				}
			}
			threshold = 0.5*(max + min);

			pos_src = y*bytesperline_src + x*channels_src;
			if (datasrc[pos_src] > threshold) {
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

/**
 * @brief Aplica a operação morfológica de dilatação numa imagem binária.
 * 
 * Esta função dilata os objetos (regiões brancas) numa imagem binária
 * utilizando um elemento estruturante quadrado de tamanho kernel.
 * 
 * @param src Imagem de origem binária (1 canal)
 * @param dst Imagem de destino binária (1 canal)
 * @param kernel Tamanho do elemento estruturante para a dilatação
 * @return 1 se a operação for bem sucedida, 0 em caso de erro
 */
int vc_binary_dilate(IVC *src, IVC *dst, int kernel) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	int verifica;
	kernel *= 0.5;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;


	for (y = kernel; y < height-kernel; y++)
	{
		for (x = kernel; x < width-kernel; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;

			verifica = 0;

			for (y2 = y-kernel; y2 <= y+kernel; y2++)
			{
				for (x2 = x-kernel; x2 <= x+kernel; x2++)
				{
					pos_src = y2*bytesperline_src + x2*channels_src;
					if (datasrc[pos_src] == 255) { verifica = 1; }
				}
			}

			if (verifica == 1) { datadst[pos_dst] = 255; }
			else { datadst[pos_dst] = 0; }
		}
	}

	return 1;
}

/**
 * @brief Aplica a operação morfológica de erosão numa imagem binária.
 * 
 * Esta função erode os objetos (regiões brancas) numa imagem binária
 * utilizando um elemento estruturante quadrado de tamanho kernel.
 * 
 * @param src Imagem de origem binária (1 canal)
 * @param dst Imagem de destino binária (1 canal)
 * @param kernel Tamanho do elemento estruturante para a erosão
 * @return 1 se a operação for bem sucedida, 0 em caso de erro
 */
int vc_binary_erode(IVC *src, IVC *dst, int kernel) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	int verifica;
	kernel *= 0.5;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;


	for (y = kernel; y < height - kernel; y++)
	{
		for (x = kernel; x < width - kernel; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;

			verifica = 0;

			for (y2 = y - kernel; y2 <= y + kernel; y2++)
			{
				for (x2 = x - kernel; x2 <= x + kernel; x2++)
				{
					pos_src = y2*bytesperline_src + x2*channels_src;
					if (datasrc[pos_src] == 0) { verifica = 1; }
				}
			}

			if (verifica == 1) { datadst[pos_dst] = 0; }
			else { datadst[pos_dst] = 255; }
		}
	}

	return 1;
}

/**
 * @brief Aplica a operação morfológica de abertura numa imagem binária.
 * 
 * Esta função realiza uma erosão seguida de uma dilatação, o que ajuda
 * a remover pequenos objetos e ruídos mantendo o tamanho dos objetos maiores.
 * 
 * @param src Imagem de origem binária (1 canal)
 * @param dst Imagem de destino binária (1 canal)
 * @param kernel Tamanho do elemento estruturante para a abertura
 * @return 1 se a operação for bem sucedida, 0 em caso de erro
 */
int vc_binary_open(IVC *src, IVC *dst, int kernel) {
	int verifica=1;
	IVC *dstTemp = vc_image_new(src->width, src->height, src->channels, src->levels);

	verifica &= vc_binary_erode(src, dstTemp, kernel);
	verifica &= vc_binary_dilate(dstTemp, dst, kernel);

	vc_image_free(dstTemp);

	return verifica;
}

/**
 * @brief Aplica a operação morfológica de fecho numa imagem binária.
 * 
 * Esta função realiza uma dilatação seguida de uma erosão, o que ajuda
 * a preencher pequenos buracos e unir regiões próximas.
 * 
 * @param src Imagem de origem binária (1 canal)
 * @param dst Imagem de destino binária (1 canal)
 * @param kernel Tamanho do elemento estruturante para o fecho
 * @return 1 se a operação for bem sucedida, 0 em caso de erro
 */
int vc_binary_close(IVC *src, IVC *dst, int kernel) {
	int verifica = 1;
	IVC *dstTemp = vc_image_new(src->width, src->height, src->channels, src->levels);

	verifica &= vc_binary_dilate(src, dstTemp, kernel);
	verifica &= vc_binary_erode(dstTemp, dst, kernel);

	vc_image_free(dstTemp);

	return verifica;
}

#ifdef __cplusplus
}
#endif
