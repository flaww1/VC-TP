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

#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string>

// Inclusão dos cabeçalhos C necessários para o vc.h
#include <opencv2/core/core_c.h>

extern "C"
{
#include "lib/vc.h"
}

int main(void)
{
    std::string videoFile;
    int choice = 0;

    // Simplified video selection
    printf("\n=== Coin Detector ===\n");
    printf("Choose video to process:\n");
    printf("1 - Video 1\n");
    printf("2 - Video 2\n");
    printf("Option: ");
    
    if (scanf("%d", &choice) != 1 || (choice != 1 && choice != 2)) {
        printf("Invalid option. Defaulting to video1.mp4\n");
        choice = 1;
    }
    
    videoFile = (choice == 1) ? "video1.mp4" : "video2.mp4";
    printf("Processing %s...\n", videoFile.c_str());

    // Open video
    cv::VideoCapture capture(videoFile);
    if (!capture.isOpened()) {
        fprintf(stderr, "Error opening video: %s\n", videoFile.c_str());
        return 1;
    }

    // Video properties
    int width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    int fps = (int)capture.get(cv::CAP_PROP_FPS);

    // Create window
    cv::namedWindow("Coin Detector", cv::WINDOW_NORMAL);
    cv::resizeWindow("Coin Detector", width/2, height/2);

    // Allocate memory for coin tracking
    int *excludeList = (int *)calloc(100, sizeof(int));
    int *coinCounts = (int *)calloc(9, sizeof(int));

    if (!excludeList || !coinCounts) {
        fprintf(stderr, "Memory allocation error!\n");
        if (excludeList) free(excludeList);
        if (coinCounts) free(coinCounts);
        capture.release();
        return 1;
    }

    int key = 0;
    bool paused = false;

    printf("Processing... Press 'q' to stop, 'p' to pause.\n");

    // Processing loop
    while (key != 'q' && key != 27) // 27 = ESC
    {
        if (!paused) {
            // Read frame
            cv::Mat frame;
            if (!capture.read(frame))
                break;

            if (frame.empty())
                break;

            // Create IVC image
            IVC *image = vc_image_new(width, height, 3, 255);
            if (!image) {
                fprintf(stderr, "IVC image allocation failed\n");
                break;
            }

            // Copy data to IVC
            memcpy(image->data, frame.data, width * height * 3);

            // Process frame
            ProcessFrame(image, image, excludeList, coinCounts);

            // Copy processed data back
            memcpy(frame.data, image->data, width * height * 3);
            vc_image_free(image);

            // Show frame
            cv::imshow("Coin Detector", frame);
        }

        // Handle key input
        key = cv::waitKey(paused ? 0 : 1000 / fps);
        if (key == 'p')
            paused = !paused;
    }

    // Calculate results
    coinCounts[8] = 0; // Total coins
    for (int i = 0; i < 8; i++) {
        coinCounts[8] += coinCounts[i];
    }

    float totalValue = coinCounts[0] * 0.01f + coinCounts[1] * 0.02f +
                       coinCounts[2] * 0.05f + coinCounts[3] * 0.10f +
                       coinCounts[4] * 0.20f + coinCounts[5] * 0.50f +
                       coinCounts[6] * 1.00f + coinCounts[7] * 2.00f;

    // Show results
    printf("\n===== RESULTS =====\n");
    printf("Total coins: %d\n", coinCounts[8]);
    printf("Coins by type:\n");
    printf("  1 cent:  %3d (%.2f €)\n", coinCounts[0], coinCounts[0] * 0.01f);
    printf("  2 cents: %3d (%.2f €)\n", coinCounts[1], coinCounts[1] * 0.02f);
    printf("  5 cents: %3d (%.2f €)\n", coinCounts[2], coinCounts[2] * 0.05f);
    printf(" 10 cents: %3d (%.2f €)\n", coinCounts[3], coinCounts[3] * 0.10f);
    printf(" 20 cents: %3d (%.2f €)\n", coinCounts[4], coinCounts[4] * 0.20f);
    printf(" 50 cents: %3d (%.2f €)\n", coinCounts[5], coinCounts[5] * 0.50f);
    printf("   1 euro: %3d (%.2f €)\n", coinCounts[6], coinCounts[6] * 1.00f);
    printf("   2 euro: %3d (%.2f €)\n", coinCounts[7], coinCounts[7] * 2.00f);
    printf("Total value: %.2f euros\n", totalValue);
    printf("==================\n");

    // Cleanup
    cv::destroyAllWindows();
    capture.release();
    free(excludeList);
    free(coinCounts);

    printf("Press Enter to exit...");
    getchar(); // Consume any pending newline
    getchar(); // Wait for Enter

    return 0;
}
