//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @file vc_filter_functions.cpp
 * @brief Funções de filtragem de imagens.
 *
 * Este ficheiro contém funções para aplicação de filtros passa-baixa,
 * passa-alta e deteção de bordas em imagens.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <opencv2/core/types_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include "vc.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Deteta bordas usando operador de Prewitt.
	 *
	 * Esta função aplica o operador de Prewitt para detetar variações bruscas de intensidade
	 * que correspondem a bordas na imagem.
	 *
	 * @param src Imagem de origem em escala de cinzentos (1 canal)
	 * @param dst Imagem de destino com as bordas detetadas (1 canal)
	 * @param th Limiar para considerar um píxel como parte de uma borda
	 * @return 1 se a operação for bem sucedida, 0 em caso de erro
	 */
	int vc_gray_edge_prewitt(IVC *src, IVC *dst, float th)
	{
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

		for (y = 1; y < height; y++)
		{
			for (x = 1; x < width; x++)
			{
				pos = y * byteperline + x * channels;

				posA = (y - 1) * byteperline + (x - 1) * channels;
				posB = (y - 1) * byteperline + (x)*channels;
				posC = (y - 1) * byteperline + (x + 1) * channels;
				posD = (y)*byteperline + (x - 1) * channels;
				posE = (y)*byteperline + (x + 1) * channels;
				posF = (y + 1) * byteperline + (x - 1) * channels;
				posG = (y + 1) * byteperline + (x)*channels;
				posH = (y + 1) * byteperline + (x + 1) * channels;
				mx = (double)((-1 * data[posA]) + (1 * data[posC]) + (-2 * data[posD]) + (2 * data[posE]) + (-1 * data[posF]) + (1 * data[posH])) / 3.0;
				my = (double)((-1 * data[posA]) + (1 * data[posF]) + (-2 * data[posB]) + (2 * data[posG]) + (-1 * data[posC]) + (1 * data[posH])) / 3.0;

				// Fix: Calculate magnitude from mx and my
				mag = sqrt((mx * mx) + (my * my));

				if (mag > th)
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
