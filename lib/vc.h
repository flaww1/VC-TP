//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @file vc.h
 * @brief Biblioteca de funções para processamento de imagem e visão computacional.
 *
 * Este ficheiro contém as definições de estruturas e protótipos de funções
 * para processamento de imagem e visão computacional implementadas na cadeira
 * de Visão por Computador.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 */

#ifdef __cplusplus
extern "C"
{
#endif

#define VC_DEBUG

// Define o número máximo de moedas que podem ser detectadas
#define MAX_COINS 50

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                           MACROS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @brief Retorna o valor máximo entre dois valores
 */
#define VC_MAX(a, b) (a > b ? a : b)

/**
 * @brief Retorna o valor mínimo entre dois valores
 */
#define VC_MIN(a, b) (a < b ? a : b)

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//                   ESTRUTURA DE UMA IMAGEM
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Estrutura para armazenar uma imagem
	 */
	typedef struct
	{
		unsigned char *data; /**< Ponteiro para os dados da imagem */
		int width, height;	 /**< Largura e altura da imagem */
		int channels;		 /**< Número de canais: Binário/Cinzentos=1; RGB=3 */
		int levels;			 /**< Níveis: Binário=1; Cinzentos [1,255]; RGB [1,255] */
		int bytesperline;	 /**< Bytes por linha = width * channels */
	} IVC;

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//                   ESTRUTURA DE UM BLOB (OBJETO)
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Estrutura para armazenar informação sobre um blob (objeto) numa imagem
	 */
	typedef struct
	{
		int x, y, width, height; /**< Caixa Delimitadora (Bounding Box) */
		int area;				 /**< Área */
		int xc, yc;				 /**< Centro-de-massa */
		int perimeter;			 /**< Perímetro */
		int label;				 /**< Etiqueta */
	} OVC;

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//                    PROTÓTIPOS DE FUNÇÕES
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Alocar memória para uma nova imagem
	 * @param width Largura da imagem
	 * @param height Altura da imagem
	 * @param channels Número de canais
	 * @param levels Níveis de cinzento
	 * @return Ponteiro para a estrutura IVC criada ou NULL em caso de erro
	 */
	IVC *vc_image_new(int width, int height, int channels, int levels);

	/**
	 * @brief Libertar memória de uma imagem
	 * @param image Ponteiro para a imagem a libertar
	 * @return NULL em caso de sucesso
	 */
	IVC *vc_image_free(IVC *image);

	/**
	 * @brief Ler uma imagem de ficheiro (PBM, PGM ou PPM)
	 * @param filename Caminho para o ficheiro
	 * @return Ponteiro para a estrutura IVC criada ou NULL em caso de erro
	 */
	IVC *vc_read_image(char *filename);

	/**
	 * @brief Gravar uma imagem em ficheiro (PBM, PGM ou PPM)
	 * @param filename Caminho para o ficheiro
	 * @param image Ponteiro para a imagem a gravar
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_write_image(char *filename, IVC *image);

	/**
	 * @brief Aplica negativo a uma imagem de nível de cinzento
	 * @param srcdst Imagem de origem e destino
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_gray_negative(IVC *srcdst);

	/**
	 * @brief Aplica negativo a uma imagem RGB
	 * @param srcdst Imagem de origem e destino
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_rgb_negative(IVC *srcdst);

	/**
	 * @brief Converte uma imagem RGB para nível de cinzento
	 * @param src Imagem de origem (RGB)
	 * @param dst Imagem de destino (nível de cinzento)
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_rgb_to_gray(IVC *src, IVC *dst);

	/**
	 * @brief Converte uma imagem RGB para HSV
	 * @param srcdst Imagem de origem e destino
	 * @param tipo Tipo de conversão
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_rgb_to_hsv(IVC *srcdst, int tipo);

	/**
	 * @brief Converte uma imagem BGR para RGB
	 * @param src Imagem de origem (BGR)
	 * @param dst Imagem de destino (RGB)
	 * @return 0
	 */
	int vc_bgr_to_rgb(IVC *src, IVC *dst);

	/**
	 * @brief Aplica uma escala de cores de cinzento para RGB
	 * @param src Imagem de origem (nível de cinzento)
	 * @param dst Imagem de destino (RGB)
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_scale_gray_to_rgb(IVC *src, IVC *dst);

	/**
	 * @brief Binariza uma imagem em nível de cinzento
	 * @param src Imagem de origem (nível de cinzento)
	 * @param dst Imagem de destino (binária)
	 * @param threshold Valor de limiar para binarização
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_gray_to_binary(IVC *src, IVC *dst, int threshold);

	/**
	 * @brief Binariza uma imagem em nível de cinzento usando a média global como limiar
	 * @param src Imagem de origem (nível de cinzento)
	 * @param dst Imagem de destino (binária)
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_gray_to_binary_global_mean(IVC *src, IVC *dst);

	/**
	 * @brief Binariza uma imagem em nível de cinzento usando o midpoint de uma vizinhança
	 * @param src Imagem de origem (nível de cinzento)
	 * @param dst Imagem de destino (binária)
	 * @param kernel Tamanho do kernel para calcular o midpoint
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel);

	/**
	 * @brief Aplica operação de dilatação em imagem binária
	 * @param src Imagem de origem (binária)
	 * @param dst Imagem de destino (binária)
	 * @param kernel Tamanho do kernel de dilatação
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_binary_dilate(IVC *src, IVC *dst, int kernel);

	/**
	 * @brief Aplica operação de erosão em imagem binária
	 * @param src Imagem de origem (binária)
	 * @param dst Imagem de destino (binária)
	 * @param kernel Tamanho do kernel de erosão
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_binary_erode(IVC *src, IVC *dst, int kernel);

	/**
	 * @brief Aplica operação de abertura em imagem binária (erosão seguida de dilatação)
	 * @param src Imagem de origem (binária)
	 * @param dst Imagem de destino (binária)
	 * @param kernel Tamanho do kernel
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_binary_open(IVC *src, IVC *dst, int kernel);

	/**
	 * @brief Aplica operação de fecho em imagem binária (dilatação seguida de erosão)
	 * @param src Imagem de origem (binária)
	 * @param dst Imagem de destino (binária)
	 * @param kernel Tamanho do kernel
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_binary_close(IVC *src, IVC *dst, int kernel);

	/**
	 * @brief Realiza a etiquetagem de objetos/blobs em imagem binária
	 * @param src Imagem de origem (binária)
	 * @param dst Imagem de destino (etiquetada)
	 * @param nlabels Ponteiro para armazenar o número de etiquetas encontradas
	 * @return Array de estruturas OVC ou NULL em caso de erro
	 */
	OVC *vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels);

	/**
	 * @brief Extrai informações sobre os blobs etiquetados
	 * @param src Imagem de origem (etiquetada)
	 * @param blobs Array de estruturas OVC
	 * @param nblobs Número de blobs
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs);

	/**
	 * @brief Deteta bordas usando operador de Prewitt
	 * @param src Imagem de origem (nível de cinzento)
	 * @param dst Imagem de destino (bordas)
	 * @param th Limiar
	 * @return 1 em caso de sucesso, 0 em caso de erro
	 */
	int vc_gray_edge_prewitt(IVC *src, IVC *dst, float th);

	/**
	 * @brief Gere a lista de moedas a excluir da contagem
	 * @param excludeList Array para guardar coordenadas das moedas
	 * @param xc Coordenada x do centro da moeda
	 * @param yc Coordenada y do centro da moeda
	 * @param option 0 para adicionar, 1 para remover
	 * @return 0
	 */
	int ExcludeCoin(int *excludeList, int xc, int yc, int option);

	/**
	 * @brief Processa os frames do vídeo para detetar moedas
	 * @param frame Frame atual
	 * @param frame2 Frame seguinte
	 * @param excludeList Array para guardar coordenadas de moedas a excluir
	 * @param coinCounts Array para contar ocorrências de cada tipo de moeda
	 */
	void ProcessFrame(IVC *frame, IVC *frame2, int *excludeList, int *coinCounts);

	/**
	 * @brief Processa a imagem para detetar e classificar moedas
	 * @param src Imagem original para análise
	 * @param blobs Blobs da primeira segmentação
	 * @param blobs2 Blobs da segunda segmentação (HSV tipo 0)
	 * @param blobs3 Blobs da terceira segmentação (HSV tipo 1)
	 * @param nlabels Número de blobs em blobs
	 * @param nlabels2 Número de blobs em blobs2
	 * @param nlabels3 Número de blobs em blobs3
	 * @param excludeList Array para guardar coordenadas de moedas a excluir
	 * @param coinCounts Array para contar ocorrências de cada tipo de moeda
	 */
	void ProcessImage(IVC *src, OVC *blobs, OVC *blobs2, OVC *blobs3, int nlabels, int nlabels2, int nlabels3, int *excludeList, int *coinCounts);

	/**
	 * @brief Identifica o tipo de moeda com base no diâmetro e análise de cor
	 * @param src Imagem original para análise
	 * @param diameter Diâmetro da moeda em pixels
	 * @param segmentType Tipo de segmentação utilizada (0 ou 1)
	 * @param xc Coordenada X do centro da moeda
	 * @param yc Coordenada Y do centro da moeda
	 * @param coinCounts Array para contar ocorrências de cada tipo de moeda
	 * @return Valor da moeda identificada
	 */
	int CoinType(IVC *src, int diameter, int segmentType, int xc, int yc, int *coinCounts);

	/**
	 * @brief Detecta se uma moeda é bimetálica (1€ ou 2€)
	 * 
	 * @param src Imagem original para análise
	 * @param xc Coordenada X do centro da moeda
	 * @param yc Coordenada Y do centro da moeda
	 * @param radius Raio da moeda
	 * @param silverBlobs Array de blobs de áreas prateadas
	 * @param nSilverBlobs Número de blobs prateados
	 * @return 1 para 1€, 2 para 2€, 0 caso não seja bimetálica
	 */
	int DetectBimetalCoin(IVC *src, int xc, int yc, int radius, OVC *silverBlobs, int nSilverBlobs);

	/**
	 * @brief Verifica se um blob tem uma forma consistente de moeda
	 * 
	 * @param blob Estrutura OVC contendo informações do blob
	 * @return true se o blob for uma moeda válida, false caso contrário
	 */
	bool IsValidCoinShape(OVC *blob);

	/**
	 * @brief Determina se as dimensões físicas de um blob correspondem a uma moeda específica
	 *
	 * @param blob Ponteiro para a estrutura do blob
	 * @param targetDiameter O diâmetro esperado para este tipo de moeda
	 * @param tolerance Porcentagem de desvio aceitável do alvo (ex.: 0.08 = 8%)
	 * @return true se o blob corresponder às dimensões da moeda, false caso contrário
	 */
	bool MatchesCoinSize(OVC *blob, float targetDiameter, float tolerance);

	/**
	 * @brief Determina a circularidade de um blob para validação de moedas
	 * 
	 * @param blob Estrutura OVC contendo informações do blob
	 * @return Valor de circularidade (1.0 = perfeitamente circular)
	 */
	float CalculateCircularity(OVC *blob);

	/**
	 * @brief Calcular diâmetro de um blob circular com base na área
	 * @param blob Ponteiro para o blob a ser avaliado
	 * @return Diâmetro estimado do blob
	 */
	float CalculateCircularDiameter(OVC *blob);

	/**
	 * @brief Filtra blobs de moedas douradas
	 * @param blobs Array de blobs a filtrar
	 * @param nlabels Número de blobs
	 */
	void FilterGoldCoinBlobs(OVC *blobs, int nlabels);

	/**
	 * @brief Filtra blobs de moedas de cobre
	 * @param blobs Array de blobs a filtrar
	 * @param nlabels Número de blobs
	 */
	void FilterCopperCoinBlobs(OVC *blobs, int nlabels);

	/**
	 * @brief Incrementa o contador de frames para rastreamento de moedas
	 */
	void IncrementFrameCounter();

	/**
	 * @brief Verifica se uma moeda já foi detectada anteriormente
	 * @param x Coordenada X da moeda
	 * @param y Coordenada Y da moeda
	 * @param coinType Tipo de moeda (1-8)
	 * @return 1 se já detectada, 0 caso contrário
	 */
	int IsCoinAlreadyDetected(int x, int y, int coinType);

	/**
	 * @brief Detecta moedas douradas
	 * @param blob Blob principal
	 * @param goldBlobs Array de blobs dourados
	 * @param ngoldBlobs Número de blobs dourados
	 * @param excludeList Lista de exclusão
	 * @param counters Contadores de moedas
	 * @param distThresholdSq Limiar de distância ao quadrado
	 * @return true se encontrou moeda dourada, false caso contrário
	 */
	bool DetectGoldCoins(OVC *blob, OVC *goldBlobs, int ngoldBlobs, int *excludeList, int *counters, int distThresholdSq);

	/**
	 * @brief Detecta moedas de bronze/cobre
	 * @param blob Blob principal
	 * @param copperBlobs Array de blobs de cobre
	 * @param ncopperBlobs Número de blobs de cobre
	 * @param excludeList Lista de exclusão
	 * @param counters Contadores de moedas
	 * @param distThresholdSq Limiar de distância ao quadrado
	 * @return true se encontrou moeda de cobre, false caso contrário
	 */
	bool DetectBronzeCoins(OVC *blob, OVC *copperBlobs, int ncopperBlobs, int *excludeList, int *counters, int distThresholdSq);

	/**
	 * @brief Segmenta áreas prateadas na imagem
	 * @param hsvSilver Imagem HSV onde segmentar
	 */
	void SegmentSilverAreas(IVC *hsvSilver);
	
	/**
	 * @brief Filtra blobs de áreas prateadas
	 * @param blobs Array de blobs a filtrar
	 * @param nlabels Número de blobs
	 */
	void FilterSilverBlobs(OVC *blobs, int nlabels);
	
	/**
	 * @brief Desenha visualizações de moedas
	 * @param frame Frame onde desenhar
	 * @param goldBlobs Blobs de moedas douradas
	 * @param copperBlobs Blobs de moedas de cobre
	 * @param nGoldBlobs Número de blobs dourados
	 * @param nCopperBlobs Número de blobs de cobre
	 * @param silverBlobs Blobs de moedas prateadas (pode ser NULL)
	 * @param nSilverBlobs Número de blobs prateados
	 */
	void DrawCoins(IVC *frame, OVC *goldBlobs, OVC *copperBlobs, 
                int nGoldBlobs, int nCopperBlobs, 
                OVC *silverBlobs, int nSilverBlobs);

	/**
	 * @brief Desenha caixas delimitadoras e centros de massa nas moedas detetadas
	 * @param src Imagem onde desenhar as caixas
	 * @param blobs Array de blobs
	 * @param nBlobs Número de blobs
	 * @param coinType Tipo de moeda (0: 1-5 cêntimos, 1: 10-2 euros)
	 * @return 1 em caso de sucesso
	 */
	int DrawBoundingBoxes(IVC *src, OVC *blobs, int nBlobs, int coinType);

	/**
	 * @brief Checks if an area has a different colored center than its outer part
	 * 
	 * @param frame Original color frame
	 * @param centerX Center X coordinate
	 * @param centerY Center Y coordinate
	 * @param sampleRadius Radius to sample pixels
	 * @param expectSilverCenter true if expecting silver center, false if expecting gold center
	 * @return 1 if different colored center detected, 0 otherwise
	 */
	int CheckForDifferentColoredCenter(IVC *frame, int centerX, int centerY, float sampleRadius, bool expectSilverCenter);

	/**
	 * @brief Gets the most recent coin type detected at a specific location
	 * This helps maintain coin type consistency when coins are partially visible
	 * @param x X coordinate
	 * @param y Y coordinate
	 * @return Coin type (1-8) or 0 if no coin was detected at that location
	 */
	int GetLastCoinTypeAtLocation(int x, int y);

	/**
	 * @brief Calculate adaptive tolerance based on coin position and frame edge proximity
	 * 
	 * @param xc X-coordinate of coin center
	 * @param yc Y-coordinate of coin center
	 * @param frameWidth Width of the frame
	 * @param frameHeight Height of the frame
	 * @return Adjusted tolerance value
	 */
	float CalculateAdaptiveTolerance(int xc, int yc, int frameWidth, int frameHeight);

	/**
	 * @brief Calibrate the pixel-to-mm conversion factor using a reference coin
	 * 
	 * @param referenceBlob Blob of a reference coin (any coin with known diameter)
	 * @param referenceCoinType Type of the reference coin (1-6 for euro cents)
	 * @return True if calibration successful, false otherwise
	 */
	bool CalibrateWithReferenceCoin(OVC *referenceBlob, int referenceCoinType);

	/**
	 * @brief Get expected pixel diameter based on coin type and current calibration
	 * 
	 * @param coinType Type of coin (1-6 for euro cents)
	 * @return Expected diameter in pixels
	 */
	float GetExpectedDiameter(int coinType);

	/**
	 * @brief Improved copper coin detection with adaptive tolerance
	 */
	bool DetectBronzeCoinsImproved(OVC *blob, OVC *copperBlobs, int ncopperBlobs, 
                             int *excludeList, int *counters, int distThresholdSq);

	/**
	 * @brief Improved gold coin detection with adaptive tolerance and better edge handling
	 */
	bool DetectGoldCoinsImproved(OVC *blob, OVC *goldBlobs, int ngoldBlobs, 
                           int *excludeList, int *counters, int distThresholdSq);

	/**
	 * @brief Draw improved visualization with annotations
	 */
	void DrawCoinsWithInfo(IVC *frame, OVC *coinBlobs, int nBlobs, int *coinCounts);

	/**
	 * @brief Draw a rectangle in the image
	 */
	void DrawRectangle(IVC *image, int x, int y, int width, int height, 
                  unsigned char r, unsigned char g, unsigned char b);

	/**
	 * @brief Draw a circle in the image
	 */
	void DrawCircle(IVC *image, int x, int y, int radius,
               unsigned char r, unsigned char g, unsigned char b);

	/**
	 * @brief Draw text on the image
	 */
	void DrawText(IVC *image, const char* text, int x, int y,
             unsigned char r, unsigned char g, unsigned char b, int fontSize);

	/**
	 * @brief Draw a thick circle around a coin
	 */
	void DrawThickCircle(IVC *image, int centerX, int centerY, int radius,
                    unsigned char r, unsigned char g, unsigned char b, int thickness);

	/**
	 * @brief Draw digits directly as pixels for coin values
	 */
	void DrawDigitPixels(IVC *image, int value, int centerX, int centerY, 
                    unsigned char r, unsigned char g, unsigned char b);
                    
	/**
	 * @brief Draw simple text for status display
	 */
	void DrawSimpleText(IVC *image, const char* text, int x, int y,
                  unsigned char r, unsigned char g, unsigned char b);

#ifdef __cplusplus
}
#endif
