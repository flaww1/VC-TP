//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @file vc.h
 * @brief Biblioteca de funções para processamento de imagem e visão por computador.
 *
 * Este ficheiro contém declarações para todas as funções e estruturas de dados
 * utilizadas no projeto de deteção e classificação de moedas através de visão
 * por computador.
 *
 * @author 
 * Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 */

#ifndef VC_H
#define VC_H

#ifdef __cplusplus
extern "C" {
#endif

// Bandeira de depuração
#define VC_DEBUG

// Número máximo de moedas a rastrear
#define MAX_COINS 50

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                           MACROS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @brief Retorna o valor máximo entre dois valores
 * 
 * Esta macro compara dois valores e retorna o maior dos dois.
 */
#define VC_MAX(a, b) (a > b ? a : b)

/**
 * @brief Retorna o valor mínimo entre dois valores
 * 
 * Esta macro compara dois valores e retorna o menor dos dois.
 */
#define VC_MIN(a, b) (a < b ? a : b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURAS DE DADOS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @brief Estrutura para armazenar uma imagem
 * 
 * Esta estrutura contém todos os dados necessários para representar
 * uma imagem, incluindo dimensões, número de canais e ponteiro para os dados.
 */
typedef struct {
    unsigned char *data;     /**< Ponteiro para os dados da imagem */
    int width, height;       /**< Dimensões da imagem (largura e altura) */
    int channels;            /**< Número de canais: Binário/Cinza=1; RGB=3 */
    int levels;              /**< Níveis: Binário=1; Cinza/RGB [1,255] */
    int bytesperline;        /**< Bytes por linha = largura * canais */
} IVC;

/**
 * @brief Estrutura para armazenar informações sobre um blob (objeto) numa imagem
 * 
 * Esta estrutura armazena informações sobre objetos detetados na imagem,
 * como posição, dimensões, área, centro de massa e perímetro.
 */
typedef struct {
    int x, y, width, height; /**< Caixa delimitadora (bounding box) */
    int area;                /**< Área do objeto em pixels */
    int xc, yc;              /**< Centro de massa (centróide) */
    int perimeter;           /**< Perímetro do objeto em pixels */
    int label;               /**< Etiqueta do objeto */
} OVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    FUNÇÕES PRINCIPAIS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Manipulação de imagens
/**
 * @brief Cria uma nova imagem com as dimensões e características especificadas
 * @param width Largura da imagem em pixels
 * @param height Altura da imagem em pixels
 * @param channels Número de canais (1 para cinza, 3 para RGB)
 * @param levels Número de níveis de intensidade (255 para normal, 1 para binário)
 * @return Ponteiro para a imagem criada ou NULL em caso de erro
 */
IVC *createImage(int width, int height, int channels, int levels);

/**
 * @brief Liberta a memória alocada para uma imagem
 * @param image Ponteiro para a imagem a ser libertada
 * @return NULL após a libertação bem-sucedida
 */
IVC *freeImage(IVC *image);

/**
 * @brief Lê uma imagem de um ficheiro
 * @param filename Caminho do ficheiro a ser lido
 * @return Ponteiro para a imagem carregada ou NULL em caso de erro
 */
IVC *readImage(char *filename);

/**
 * @brief Guarda uma imagem num ficheiro
 * @param filename Caminho onde guardar a imagem
 * @param image Imagem a ser guardada
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int writeImage(char *filename, IVC *image);

// Conversões de cores
/**
 * @brief Converte uma imagem do formato BGR para RGB
 * @param src Imagem de origem no formato BGR
 * @param dst Imagem de destino no formato RGB
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int bgr2rgb(IVC *src, IVC *dst);

/**
 * @brief Converte uma imagem RGB para escala de cinzento
 * @param src Imagem de origem no formato RGB
 * @param dst Imagem de destino em escala de cinzento
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int rgb2gray(IVC *src, IVC *dst);

/**
 * @brief Converte uma imagem RGB para o espaço de cores HSV e segmenta com base no tipo
 * @param srcdst Imagem a ser convertida e segmentada (altera a original)
 * @param segmentType Tipo de segmentação: 0=moedas douradas, 1=moedas cobre, 2=moedas euro
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int rgb2hsv(IVC *srcdst, int segmentType);

// Operações binárias
/**
 * @brief Converte uma imagem em escala de cinzento para binária usando um limiar
 * @param src Imagem de origem em escala de cinzento
 * @param dst Imagem de destino binária
 * @param threshold Valor limiar para a conversão (0-255)
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int gray2binary(IVC *src, IVC *dst, int threshold);

/**
 * @brief Aplica a operação morfológica de abertura (erosão seguida de dilatação)
 * @param src Imagem de origem binária
 * @param dst Imagem de destino binária
 * @param kernel Tamanho do kernel (matriz estruturante)
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int binaryOpen(IVC *src, IVC *dst, int kernel);

/**
 * @brief Aplica a operação morfológica de fechamento (dilatação seguida de erosão)
 * @param src Imagem de origem binária
 * @param dst Imagem de destino binária
 * @param kernel Tamanho do kernel (matriz estruturante)
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int binaryClose(IVC *src, IVC *dst, int kernel);

// Deteção de bordas
/**
 * @brief Deteta bordas numa imagem em escala de cinzento
 * @param src Imagem de origem em escala de cinzento
 * @param dst Imagem de destino com as bordas detetadas
 * @param threshold Limiar para determinar o que é considerado uma borda
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int detectEdges(IVC *src, IVC *dst, float threshold);

// Análise de blobs
/**
 * @brief Etiqueta objetos (blobs) numa imagem binária
 * @param src Imagem de origem binária
 * @param dst Imagem de destino onde cada objeto terá uma etiqueta única
 * @param nlabels Ponteiro para armazenar o número de objetos encontrados
 * @return Array de estruturas OVC com as informações dos objetos detetados
 */
OVC *blobLabel(IVC *src, IVC *dst, int *nlabels);

/**
 * @brief Calcula propriedades dos objetos etiquetados
 * @param src Imagem com os objetos etiquetados
 * @param blobs Array de estruturas OVC para armazenar as informações
 * @param nblobs Número de objetos a processar
 * @return 1 em caso de sucesso, 0 em caso de erro
 */
int blobInfo(IVC *src, OVC *blobs, int nblobs);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    FUNÇÕES PARA MOEDAS
