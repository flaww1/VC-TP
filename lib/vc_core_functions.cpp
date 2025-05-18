//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @file vc_core_functions.cpp
 * @brief Funções básicas para manipulação de imagens.
 * 
 * Este ficheiro contém funções para criar, libertar e ler imagens,
 * incluindo operações com formatos PBM, PGM e PPM.
 * 
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

// Include all headers before any extern "C" blocks
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <opencv2/core/types_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include "vc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Aloca memória para uma nova imagem.
 * 
 * Esta função cria e inicializa uma estrutura IVC para representar uma imagem,
 * alocando a memória necessária para o buffer de dados de acordo com as dimensões e
 * características especificadas.
 * 
 * @param width Largura da imagem em píxeis
 * @param height Altura da imagem em píxeis
 * @param channels Número de canais por píxel (1 para monocromático, 3 para RGB)
 * @param levels Número de níveis por canal (1 para binário, 255 para escala de cinzentos)
 * @return Ponteiro para a estrutura IVC alocada ou NULL em caso de erro
 */
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}

/**
 * @brief Liberta a memória alocada para uma imagem.
 * 
 * Esta função liberta a memória alocada tanto para o buffer de dados da imagem
 * quanto para a própria estrutura IVC.
 * 
 * @param image Ponteiro para a estrutura IVC a ser libertada
 * @return NULL após a libertação bem-sucedida
 */
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}

/**
 * @brief Obtém um token de texto de um ficheiro PBM/PGM/PPM.
 * 
 * Esta função auxiliar lê caracteres de um ficheiro até encontrar um token válido,
 * ignorando espaços em branco e linhas de comentários (que começam com #).
 * 
 * @param file Ponteiro para o ficheiro aberto
 * @param tok Buffer para armazenar o token lido
 * @param len Tamanho máximo do buffer
 * @return Ponteiro para o buffer com o token lido
 */
char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;

	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}

	t = tok;

	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if(c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}

/**
 * @brief Converte dados de unsigned char para formato de bits (para PBM).
 * 
 * Esta função converte uma representação de um byte por píxel para uma representação
 * de bit por píxel (compacta), utilizada em ficheiros PBM binários.
 * 
 * @param datauchar Array com dados originais (um byte por píxel)
 * @param databit Buffer de destino para os dados em formato de bits
 * @param width Largura da imagem em píxeis
 * @param height Altura da imagem em píxeis
 * @return Número total de bytes na representação compacta
 */
long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}

/**
 * @brief Converte dados em formato de bits para unsigned char (de PBM).
 * 
 * Esta função converte uma representação compacta de bit por píxel para a
 * representação de um byte por píxel usada na estrutura IVC.
 * 
 * @param databit Buffer com os dados em formato de bits
 * @param datauchar Buffer de destino (um byte por píxel)
 * @param width Largura da imagem em píxeis
 * @param height Altura da imagem em píxeis
 */
void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}

/**
 * @brief Lê uma imagem de um ficheiro PBM, PGM ou PPM.
 * 
 * Esta função suporta a leitura dos formatos Netpbm:
 * - PBM (P4): Formato binário para imagens a preto e branco
 * - PGM (P5): Formato em escala de cinzentos
 * - PPM (P6): Formato colorido RGB
 * 
 * @param filename Caminho para o ficheiro a ler
 * @return Ponteiro para a estrutura IVC com a imagem lida ou NULL em caso de erro
 */
IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	if((file = fopen(filename, "rb")) != NULL)
	{
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }
		else if(strcmp(tok, "P5") == 0) channels = 1;
		else if(strcmp(tok, "P6") == 0) channels = 3;
		else
		{
			#ifdef VC_DEBUG
			printf("ERRO -> vc_read_image():\n\tO ficheiro não é um PBM, PGM ou PPM válido.\n\tMagic number errado!\n");
			#endif

			fclose(file);
			return NULL;
		}

		if(levels == 1)
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERRO -> vc_read_image():\n\tO ficheiro não é um PBM válido.\n\tDimensões inválidas!\n");
				#endif

				fclose(file);
				return NULL;
			}

			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\ncanais=%d l=%d a=%d níveis=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				#ifdef VC_DEBUG
				printf("ERRO -> vc_read_image():\n\tFim de ficheiro prematuro.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERRO -> vc_read_image():\n\tO ficheiro não é um PGM ou PPM válido.\n\tDimensões ou níveis inválidos!\n");
				#endif

				fclose(file);
				return NULL;
			}

			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\ncanais=%d l=%d a=%d níveis=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				#ifdef VC_DEBUG
				printf("ERRO -> vc_read_image():\n\tFim de ficheiro prematuro.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
		#ifdef VC_DEBUG
		printf("ERRO -> vc_read_image():\n\tFicheiro não encontrado.\n");
		#endif
	}

	return image;
}

/**
 * @brief Grava uma imagem em formato PBM, PGM ou PPM.
 * 
 * Esta função permite salvar imagens nos formatos Netpbm:
 * - PBM (P4): Formato binário para imagens a preto e branco
 * - PGM (P5): Formato em escala de cinzentos
 * - PPM (P6): Formato colorido RGB
 * 
 * @param filename Caminho para o ficheiro a escrever
 * @param image Ponteiro para a estrutura IVC contendo a imagem
 * @return 1 se a operação for bem sucedida, 0 em caso de erro
 */
int vc_write_image(char *filename, IVC *image)
{
    FILE *file = NULL;
    unsigned char *tmp;
    long int totalbytes, sizeofbinarydata;
    int i, chan;
    
    if (image == NULL) return 0;
    if ((image->width <= 0) || (image->height <= 0) || (image->data == NULL)) return 0;
    if (filename == NULL) return 0;
    
    if ((file = fopen(filename, "wb")) != NULL)
    {
        if (image->levels == 1)
        {
            // PBM - Binary format (P4)
            fprintf(file, "P4\n");
            fprintf(file, "%d %d\n", image->width, image->height);
            
            // Calculate binary data size and allocate temporary buffer
            sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
            if ((tmp = (unsigned char *)malloc(sizeofbinarydata)) == NULL)
            {
                fclose(file);
                return 0;
            }
            
            // Convert 1 byte per pixel to binary format
            totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
            if (totalbytes != sizeofbinarydata)
            {
                #ifdef VC_DEBUG
                printf("ERRO -> vc_write_image():\n\tConversão para formato binário falhou!\n");
                #endif
                
                free(tmp);
                fclose(file);
                return 0;
            }
            
            // Write binary data to file
            if ((i = fwrite(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
            {
                #ifdef VC_DEBUG
                printf("ERRO -> vc_write_image():\n\tFalha na escrita dos dados da imagem!\n");
                #endif
                
                free(tmp);
                fclose(file);
                return 0;
            }
            
            free(tmp);
        }
        else
        {
            // PGM/PPM - Binary format (P5/P6)
            chan = image->channels;
            if (chan == 1)
                fprintf(file, "P5\n");    // PGM
            else
                fprintf(file, "P6\n");    // PPM
            
            fprintf(file, "%d %d\n", image->width, image->height);
            fprintf(file, "%d\n", image->levels);
            
            // Write pixel data to file
            totalbytes = image->width * image->height * chan;
            if ((i = fwrite(image->data, sizeof(unsigned char), totalbytes, file)) != totalbytes)
            {
                #ifdef VC_DEBUG
                printf("ERRO -> vc_write_image():\n\tFalha na escrita dos dados da imagem!\n");
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

// All functions in this file are used in the application

#ifdef __cplusplus
}
#endif
