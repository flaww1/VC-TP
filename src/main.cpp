/**
 * @file main.cpp
 * @brief Programa principal para deteção e classificação de moedas em vídeo.
 *
 * Este programa implementa um sistema de deteção e classificação automática
 * de moedas de Euro em sequências de vídeo. Utiliza processamento de imagem
 * e algoritmos de visão computacional para identificar, contar e classificar
 * diferentes tipos de moedas.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <opencv2/opencv.hpp>

extern "C" {
#include "../lib/vc.h"
}

// Estrutura para armazenar estatísticas das moedas
typedef struct {
    int count;          // Número de moedas detetadas
    long totalArea;     // Soma total das áreas
    long totalPerimeter; // Soma total dos perímetros
    float avgArea;      // Área média
    float avgPerimeter; // Perímetro médio
} CoinStats;

// Função temporizador para medir o tempo de processamento
void timer(void) {
    static bool running = false;
    static std::chrono::steady_clock::time_point prevTime = std::chrono::steady_clock::now();

    if (!running) {
        running = true;
    } else {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - prevTime);
        double seconds = elapsed.count();

        std::cout << "Tempo decorrido: " << seconds << " segundos" << std::endl;
        std::cout << "Prima Enter para continuar...\n";
        std::cin.get();
    }
}

int main(void) {
    // Lista de exclusão para moedas que não devem ser contadas
    int excludeList[MAX_COINS * 2] = {0};
    // Contadores de moedas: 1c, 2c, 5c, 10c, 20c, 50c, 1€, 2€
    int coinCounts[8] = {0};
    // Estatísticas para cada tipo de moeda
    CoinStats coinStats[8] = {0};
    
    // Carregamento do vídeo
    cv::VideoCapture capture;
    int key = 0;
    
    // Abre o ficheiro de vídeo
    capture.open("../video/moedas.avi");
    
    // Verifica se o vídeo foi aberto com sucesso
    if (!capture.isOpened()) {
        std::cerr << "Erro: VideoCapture não foi aberto!\n";
        return -1;
    }
    
    // Obtém as propriedades do vídeo
    int width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    int fps = (int)capture.get(cv::CAP_PROP_FPS);
    int totalFrames = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    
    std::cout << "Propriedades do vídeo:\n";
    std::cout << "  - Largura: " << width << "\n";
    std::cout << "  - Altura: " << height << "\n";
    std::cout << "  - FPS: " << fps << "\n";
    std::cout << "  - Total de frames: " << totalFrames << "\n\n";
    
    // Cria uma janela para visualização
    cv::namedWindow("Contador de Moedas", cv::WINDOW_NORMAL);
    
    // Cria contentores de imagem OpenCV
    cv::Mat frame, frame2;
    
    // Cria contentores de imagem IVC
    IVC *ivc_frame = createImage(width, height, 3, 255);
    IVC *ivc_frame2 = createImage(width, height, 3, 255);
    
    // Verifica se as imagens foram criadas com sucesso
    if (!ivc_frame || !ivc_frame2) {
        std::cerr << "Erro: Imagens IVC não criadas!\n";
        return -1;
    }
    
    // Configura para rastrear estatísticas dos blobs para médias
    int frameCount = 0;
    
    // Processa os frames do vídeo
    while (key != 'q') {
        // Obtém o próximo frame
        if (!capture.read(frame)) break;
        
        // Processa a cada dois frames para melhorar o desempenho
        if (frameCount % 2 == 0) {
            frame.copyTo(frame2);
        }
        frameCount++;
        
        // Converte o formato Mat do OpenCV para o formato IVC
        memcpy(ivc_frame->data, frame.data, width * height * 3);
        memcpy(ivc_frame2->data, frame2.data, width * height * 3);
        
        // Processa o frame com as nossas funções personalizadas
        processFrame(ivc_frame, ivc_frame2, excludeList, coinCounts);
        
        // Exibe a imagem
        cv::imshow("Contador de Moedas", frame);
        
        // Aguarda tecla (10ms)
        key = cv::waitKey(10);
    }
    
    // Calcula estatísticas finais
    const char* coinNames[8] = {"1¢", "2¢", "5¢", "10¢", "20¢", "50¢", "1€", "2€"};
    const float coinValues[8] = {0.01, 0.02, 0.05, 0.10, 0.20, 0.50, 1.00, 2.00};
    float totalValue = 0.0f;
    int totalCoins = 0;
    
    // Recolhe estatísticas de moedas do array de rastreamento
    // Este array externo contém informações sobre todas as moedas detetadas
    extern int detectedCoins[150][5]; // Do sistema de rastreamento de moedas
    extern int MAX_TRACKED_COINS; 
    
    // Reinicia os arrays de estatísticas de moedas
    for (int i = 0; i < 8; i++) {
        coinStats[i].count = 0;
        coinStats[i].totalArea = 0;
        coinStats[i].totalPerimeter = 0;
    }
    
    // Processa os dados de moedas armazenados para calcular estatísticas
    for (int i = 0; i < MAX_TRACKED_COINS; i++) {
        // Ignora registos vazios
        if (detectedCoins[i][0] == 0 && detectedCoins[i][1] == 0) continue;
        
        // Obtém o tipo de moeda (índice baseado em 1, precisa subtrair 1 para índice do array)
        int coinType = detectedCoins[i][2];
        if (coinType < 1 || coinType > 8) continue;
        
        // Contabiliza apenas moedas que foram realmente contadas (flag definida como 1)
        if (detectedCoins[i][4] == 1) {
            int coinIndex = coinType - 1;
            
            // Atualiza contagem apenas (área e perímetro não são mais necessários)
            coinStats[coinIndex].count++;
        }
    }
    
    // Calcula estatísticas finais
    for (int i = 0; i < 8; i++) {
        totalCoins += coinCounts[i];
        totalValue += coinCounts[i] * coinValues[i];
    }
    
    // Imprime resultados finais com formato simplificado (sem área e perímetro)
    std::cout << "\n\n";
    std::cout << "=====================================================\n";
    std::cout << "                 RESULTADOS FINAIS                   \n";
    std::cout << "=====================================================\n";
    std::cout << "Tipo Moeda | Quantidade | Valor (€)\n";
    std::cout << "-----------|-----------|---------\n";
    
    for (int i = 0; i < 8; i++) {
        if (coinCounts[i] > 0) {
            std::cout << std::left << std::setw(11) << coinNames[i] << " | " 
                      << std::right << std::setw(9) << coinCounts[i] << " | " 
                      << std::fixed << std::setprecision(2) << std::setw(7) << coinCounts[i] * coinValues[i] << " €\n";
        }
    }
    
    std::cout << "-----------|-----------|---------\n";
    std::cout << std::left << std::setw(11) << "TOTAL" << " | " 
              << std::right << std::setw(9) << totalCoins << " | "
              << std::fixed << std::setprecision(2) << std::setw(7) << totalValue << " €\n";
    std::cout << "=====================================================\n";
    
    // Liberta recursos
    if (ivc_frame) freeImage(ivc_frame);
    if (ivc_frame2) freeImage(ivc_frame2);
    
    // Fecha janelas e liberta o vídeo
    cv::destroyAllWindows();
    capture.release();
    
    return 0;
}
