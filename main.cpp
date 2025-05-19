/**
 * @file main.cpp
 * @brief Programa principal para deteção e classificação de moedas em vídeo.
 *
 * Este programa permite a análise de vídeos para detetar moedas, classificá-las
 * pelo seu valor e calcular o montante total.
 *
 * @author
 * Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>  // For formatted output
#include <sstream>  // For string streams
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

// Inclusão dos cabeçalhos C necessários para o vc.h
extern "C"
{
#include "lib/vc.h"
}

// Timer function to measure processing time
void vc_timer(void) {
    static bool running = false;
    static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

    if (!running) {
        running = true;
    }
    else {
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

        // Tempo em segundos.
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
        double nseconds = time_span.count();

        std::cout << "Tempo decorrido: " << nseconds << " segundos" << std::endl;
        std::cout << "Pressione Enter para continuar...\n";
        std::cin.get();
    }
}

// Structure to store coin detection statistics
typedef struct {
    int frame;
    int totalCoins;
    float totalValue;
    int coinCounts[6]; // 1c, 2c, 5c, 10c, 20c, 50c
} DetectionStats;

int main(void)
{
    // Video file selection
    std::string videoFile = "video1.mp4";
    int choice = 0;

    // Simplified video selection
    std::cout << "\n=== Improved Coin Detector ===\n";
    std::cout << "Choose video to process:\n";
    std::cout << "1 - Video 1\n";
    std::cout << "2 - Video 2\n";
    std::cout << "Option: ";
    
    std::cin >> choice;
    if (choice != 1 && choice != 2) {
        std::cout << "Invalid option. Defaulting to video1.mp4\n";
        choice = 1;
    }
    
    videoFile = (choice == 1) ? "video1.mp4" : "video2.mp4";
    std::cout << "Processing " << videoFile << "...\n";

    // Time measurement
    vc_timer();

    // Open the video
    cv::VideoCapture capture;
    capture.open(videoFile);

    // Check if the video file was opened successfully
    if (!capture.isOpened())
    {
        std::cerr << "Error opening video: " << videoFile << std::endl;
        return 1;
    }

    // Video properties
    int width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    int fps = (int)capture.get(cv::CAP_PROP_FPS);
    int totalFrames = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);

    // Create a window for displaying the video - make it resizable
    cv::namedWindow("Coin Detector", cv::WINDOW_NORMAL);
    
    // Set window size to be larger (1.5x original size)
    cv::resizeWindow("Coin Detector", width * 1.5, height * 1.5);

    // Allocate memory for coin tracking
    int *excludeList = (int *)calloc(MAX_COINS * 2, sizeof(int));
    int *coinCounts = (int *)calloc(9, sizeof(int));

    if (!excludeList || !coinCounts) {
        std::cerr << "Memory allocation failed!" << std::endl;
        if (excludeList) free(excludeList);
        if (coinCounts) free(coinCounts);
        capture.release();
        return 1;
    }

    // Create video writer for output (using compatible codec for MP4)
    cv::VideoWriter outputVideo;
    std::string outputFilename = "output_" + videoFile;
    
    // Use MP4V codec which is compatible with MP4 container
    int codec = cv::VideoWriter::fourcc('M', 'P', '4', 'V');
    outputVideo.open(outputFilename, codec, fps, cv::Size(width, height));

    if (!outputVideo.isOpened()) {
        std::cerr << "Could not create output video file! Will continue without saving output." << std::endl;
    }

    // Ask user if they want fullscreen mode
    std::cout << "Show in fullscreen mode? (y/n): ";
    char fullscreen;
    std::cin >> fullscreen;
    if (fullscreen == 'y' || fullscreen == 'Y') {
        cv::setWindowProperty("Coin Detector", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    }

    // Processing variables
    int key = 0;
    bool paused = false;
    int frameNumber = 0;
    DetectionStats currentStats;
    std::vector<DetectionStats> statistics;

    std::cout << "Processing... Press 'q' to stop, 'p' to pause, 's' to save screenshot, 'f' to toggle fullscreen\n";

    // Processing loop
    cv::Mat frame;
    bool isFullscreen = (fullscreen == 'y' || fullscreen == 'Y');
    
    while (key != 'q') {
        if (!paused) {
            // Read a frame from the video
            capture.read(frame);

            // Check if we reached the end of the video
            if (frame.empty()) break;

            // Current frame number
            frameNumber = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

            // Create IVC image from the frame
            IVC *image = vc_image_new(width, height, 3, 255);
            if (!image) {
                std::cerr << "IVC image allocation failed" << std::endl;
                break;
            }

            // Copy data from cv::Mat to IVC
            memcpy(image->data, frame.data, width * height * 3);

            // Process the frame with our improved coin detection functions
            ProcessFrame(image, image, excludeList, coinCounts);

            // Copy processed data back to cv::Mat
            memcpy(frame.data, image->data, width * height * 3);
            vc_image_free(image);

            // Update statistics
            currentStats.frame = frameNumber;
            currentStats.totalCoins = 0;
            currentStats.totalValue = 0.0f;
            
            for (int i = 0; i < 6; i++) {
                currentStats.coinCounts[i] = coinCounts[i];
                currentStats.totalCoins += coinCounts[i];
                
                // Add value in euros
                float coinValue = (i == 0) ? 0.01f :  // 1 cent
                                 (i == 1) ? 0.02f :  // 2 cents
                                 (i == 2) ? 0.05f :  // 5 cents
                                 (i == 3) ? 0.10f :  // 10 cents
                                 (i == 4) ? 0.20f :  // 20 cents
                                 0.50f;              // 50 cents
                                 
                currentStats.totalValue += coinCounts[i] * coinValue;
            }
            
            // Store statistics
            statistics.push_back(currentStats);
            
            // Display progress bar on console
            double progress = (double)frameNumber / totalFrames;
            int barWidth = 50;
            std::cout << "[";
            int pos = barWidth * progress;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << "% | Frame " << frameNumber << "/" << totalFrames 
                      << " | Coins: " << currentStats.totalCoins << " | Value: " << std::fixed << std::setprecision(2) 
                      << currentStats.totalValue << " EUR\r";
            std::cout.flush();
            
            // Display information on the frame using OpenCV's putText
            std::stringstream ss;
            ss << "Total: " << currentStats.totalCoins << " coins = " << std::fixed << std::setprecision(2) << currentStats.totalValue << " EUR";
            cv::putText(frame, ss.str(), cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 2);
            cv::putText(frame, ss.str(), cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
            
            ss.str("");
            ss << "1c:" << currentStats.coinCounts[0] << "  2c:" << currentStats.coinCounts[1] << "  5c:" << currentStats.coinCounts[2]
               << "  10c:" << currentStats.coinCounts[3] << "  20c:" << currentStats.coinCounts[4] << "  50c:" << currentStats.coinCounts[5];
            cv::putText(frame, ss.str(), cv::Point(15, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 2);
            cv::putText(frame, ss.str(), cv::Point(15, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 1);
            
            // Write to output video
            if (outputVideo.isOpened()) {
                outputVideo.write(frame);
            }

            // Display the frame
            cv::imshow("Coin Detector", frame);
        }

        // Check for user input
        key = cv::waitKey(paused ? 0 : 1000 / fps);
        
        // Handle special keys
        if (key == 'p') {
            paused = !paused;
            std::cout << "\n" << (paused ? "Paused" : "Resumed") << std::endl;
        }
        else if (key == 's') {
            // Save screenshot using OpenCV's imwrite (one of our allowed 3 functions)
            std::string screenshotName = "screenshot_" + std::to_string(frameNumber) + ".jpg";
            cv::imwrite(screenshotName, frame);
            std::cout << "\nSaved screenshot to " << screenshotName << std::endl;
        }
        else if (key == 'f') {
            // Toggle fullscreen mode
            isFullscreen = !isFullscreen;
            cv::setWindowProperty("Coin Detector", cv::WND_PROP_FULLSCREEN, 
                                 isFullscreen ? cv::WINDOW_FULLSCREEN : cv::WINDOW_NORMAL);
            std::cout << "\n" << (isFullscreen ? "Fullscreen mode" : "Window mode") << std::endl;
        }
    }

    std::cout << "\n"; // Move to next line after progress bar

    // Calculate and display final results
    int totalCoins = 0;
    for (int i = 0; i < 6; i++) {
        totalCoins += coinCounts[i];
    }

    float totalValue = coinCounts[0] * 0.01f + coinCounts[1] * 0.02f +
                      coinCounts[2] * 0.05f + coinCounts[3] * 0.10f +
                      coinCounts[4] * 0.20f + coinCounts[5] * 0.50f;

    std::cout << "\n===== FINAL RESULTS =====\n";
    std::cout << "Total coins: " << totalCoins << std::endl;
    std::cout << "Coins by type:" << std::endl;
    std::cout << "  1 cent:  " << std::setw(3) << coinCounts[0] << " (" << std::fixed << std::setprecision(2) << coinCounts[0] * 0.01f << " €)" << std::endl;
    std::cout << "  2 cents: " << std::setw(3) << coinCounts[1] << " (" << std::fixed << std::setprecision(2) << coinCounts[1] * 0.02f << " €)" << std::endl;
    std::cout << "  5 cents: " << std::setw(3) << coinCounts[2] << " (" << std::fixed << std::setprecision(2) << coinCounts[2] * 0.05f << " €)" << std::endl;
    std::cout << " 10 cents: " << std::setw(3) << coinCounts[3] << " (" << std::fixed << std::setprecision(2) << coinCounts[3] * 0.10f << " €)" << std::endl;
    std::cout << " 20 cents: " << std::setw(3) << coinCounts[4] << " (" << std::fixed << std::setprecision(2) << coinCounts[4] * 0.20f << " €)" << std::endl;
    std::cout << " 50 cents: " << std::setw(3) << coinCounts[5] << " (" << std::fixed << std::setprecision(2) << coinCounts[5] * 0.50f << " €)" << std::endl;
    std::cout << "Total value: " << std::fixed << std::setprecision(2) << totalValue << " euros" << std::endl;
    std::cout << "==================\n";

    // Save results to CSV file
    FILE* resultsFile = fopen("coin_detection_results.csv", "w");
    if (resultsFile) {
        fprintf(resultsFile, "Frame,TotalCoins,TotalValue,1c,2c,5c,10c,20c,50c\n");
        
        for (const auto& stat : statistics) {
            fprintf(resultsFile, "%d,%d,%.2f,%d,%d,%d,%d,%d,%d\n",
                   stat.frame, stat.totalCoins, stat.totalValue,
                   stat.coinCounts[0], stat.coinCounts[1], stat.coinCounts[2],
                   stat.coinCounts[3], stat.coinCounts[4], stat.coinCounts[5]);
        }
        
        fclose(resultsFile);
        std::cout << "Results saved to coin_detection_results.csv\n";
    }

    // Stop the timer and show elapsed time
    vc_timer();

    // Cleanup
    cv::destroyAllWindows();
    capture.release();
    outputVideo.release();
    free(excludeList);
    free(coinCounts);

    return 0;
}
