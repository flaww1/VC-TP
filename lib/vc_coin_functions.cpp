//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @file vc_coin_functions.cpp
 * @brief Funções específicas para deteção e classificação de moedas.
 *
 * Este ficheiro contém funções dedicadas ao processamento de imagens
 * para identificar e classificar moedas por valor.
 *
 * @author Grupo 7 ( Daniel - 26432 / Maria - 26438 / Bruno - 26014 / Flávio - 21110)
 * @date 2024/2025
 */

// Include all headers before any extern "C" blocks
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/types_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include "vc.h"

// Debug macro - uncomment to enable debug messages
// #define VC_DEBUG_COINS

#ifdef VC_DEBUG_COINS
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) 
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    // Add function prototypes to avoid issues with functions used before definition
    float CalculateCircularity(OVC *blob);
    int DrawBoundingBoxes(IVC *src, OVC *blobs, int nBlobs, int coinType);

    /**
     * @brief Desenha círculos e centros de massa nas moedas detetadas com performance otimizada.
     */
    int DrawBoundingBoxes(IVC *src, OVC *blobs, int nBlobs, int coinType)
    {
        if (!src || !blobs || nBlobs <= 0 || src->data == NULL)
            return 0;

        unsigned char *datasrc = (unsigned char *)src->data;
        const int bytesperline_src = src->width * src->channels;
        const int channels_src = src->channels;
        const int width = src->width;
        const int height = src->height;
        
        // Define colors for gold and copper coins with more contrast (BGR format)
        const unsigned char colorCopper[3] = {0, 80, 255};     // Copper/bronze coins - orange (BGR)
        const unsigned char colorGold[3] = {0, 215, 255};      // Gold coins - yellow gold (BGR)
        const unsigned char colorCenter[3] = {255, 255, 255};  // White for centers
        const unsigned char colorText[3] = {255, 255, 255};    // White for text labels
        
        // Choose fixed color based on coin type (only gold or copper)
        const unsigned char *baseColor = (coinType == 0) ? colorCopper : colorGold;
        
        // Process each blob
        for (int i = 0; i < nBlobs; i++)
        {
            // Skip invalid blobs
            if ((blobs[i].label == 0 && blobs[i].area < 14000) || 
                (blobs[i].area <= 7000) || 
                (blobs[i].height > 220))
                continue;
            
            // Get blob properties
            const float circularDiameter = CalculateCircularDiameter(&blobs[i]);
            const int radius = (int)(circularDiameter / 2.0f);
            
            // Skip coins that are partially off-screen
            // Check if the coin is fully within the frame with a small margin (5 pixels)
            const int margin = 5;
            if (blobs[i].xc - radius < margin || 
                blobs[i].xc + radius >= width - margin || 
                blobs[i].yc - radius < margin || 
                blobs[i].yc + radius >= height - margin) {
                continue;  // Skip this coin as it's partially outside the frame
            }
            
            // Determine coin type based on diameter for tracking and labeling
            int actualCoinType = 0;
            const char* coinLabel = "";
            
            // Classify coins by size
            if (coinType == 0) { // Copper coins
                if (circularDiameter < 130) {
                    actualCoinType = 1;       // 1c
                    coinLabel = "1c";
                }
                else if (circularDiameter < 145) {
                    actualCoinType = 2;       // 2c
                    coinLabel = "2c";
                }
                else {
                    actualCoinType = 3;       // 5c
                    coinLabel = "5c";
                }
            } else if (coinType == 1) { // Gold coins
                if (circularDiameter < 150) {
                    actualCoinType = 4;       // 10c
                    coinLabel = "10c";
                }
                else if (circularDiameter < 170) {
                    actualCoinType = 5;       // 20c
                    coinLabel = "20c";
                }
                else {
                    actualCoinType = 6;       // 50c
                    coinLabel = "50c";
                }
            }
            
            // Check if this coin has been detected before (for tracking only)
            if (actualCoinType > 0) {
                IsCoinAlreadyDetected(blobs[i].xc, blobs[i].yc, actualCoinType);
            }
            
            // Always use the proper color based on coin type
            const unsigned char *coinColor = baseColor;
            
            // Use blob center directly
            int centerX = blobs[i].xc;
            int centerY = blobs[i].yc;
            
            // Draw circle with improved efficiency
            const int CIRCLE_THICKNESS = 3;
            for (int a = 0; a < 360; a++) {
                for (int t = 0; t < CIRCLE_THICKNESS; t++) {
                    int drawRadius = radius - t;
                    if (drawRadius <= 0) continue;
                    
                    int x = (int)(centerX + drawRadius * cosf(a * (M_PI / 180.0f)));
                    int y = (int)(centerY + drawRadius * sinf(a * (M_PI / 180.0f)));
                    
                    // Check boundaries once
                    if (x >= 0 && x < width && y >= 0 && y < height) {
                        int pos = y * bytesperline_src + x * channels_src;
                        datasrc[pos] = coinColor[0];
                        datasrc[pos + 1] = coinColor[1];
                        datasrc[pos + 2] = coinColor[2];
                    }
                }
            }
            
            // Draw enhanced center marker (if within bounds)
            if (centerX >= 0 && centerX < width && centerY >= 0 && centerY < height) {
                // Draw coin value label - a simple way to draw text by just plotting pixels
                // Determine text placement position relative to center
                int textX = centerX;
                int textY = centerY;
                
                // Function to set a pixel for text drawing
                auto setTextPixel = [&](int x, int y) {
                    if (x >= 0 && x < width && y >= 0 && y < height) {
                        int pos = y * bytesperline_src + x * channels_src;
                        datasrc[pos] = colorText[0];
                        datasrc[pos + 1] = colorText[1];
                        datasrc[pos + 2] = colorText[2];
                    }
                };
                
                // IMPROVED TEXT RENDERING - Larger characters and better background
                
                // Draw a larger, more visible background box
                const int textHeight = 30; 
                const int textWidth = 35;
                
                // Draw a more opaque background rectangle for better visibility
                for (int y = textY - textHeight/2; y <= textY + textHeight/2; y++) {
                    for (int x = textX - textWidth/2; x <= textX + textWidth/2; x++) {
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            int pos = y * bytesperline_src + x * channels_src;
                            // Solid black background
                            datasrc[pos] = 0;
                            datasrc[pos + 1] = 0;
                            datasrc[pos + 2] = 0;
                        }
                    }
                }
                
                // Draw the coin label with LARGER characters
                int scale = 2; // Scale factor for larger text
                
                // Draw character using simple pixel patterns - but larger than before
                switch(actualCoinType) {
                    case 1: // 1 cent
                        // Draw "1c" larger
                        for (int y = -10*scale; y <= 10*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX - 5*scale + x, textY + y);
                        
                        // Draw 'c' larger
                        for (int y = -5*scale; y <= 5*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX + 5*scale + x, textY + y);
                        for (int x = -5*scale; x <= 5*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + 10*scale + x, textY - 5*scale + y);
                                setTextPixel(textX + 10*scale + x, textY + 5*scale + y);
                            }
                        }
                        break;
                        
                    case 2: // 2 cents
                        // Draw "2c" larger
                        for (int x = -5*scale; x <= 5*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX - 5*scale + x, textY - 10*scale + y);
                                setTextPixel(textX - 5*scale + x, textY + y);
                                setTextPixel(textX - 5*scale + x, textY + 10*scale + y);
                            }
                        }
                        for (int y = -10*scale; y <= 0; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX + x, textY + y);
                        for (int y = 0; y <= 10*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX - 10*scale + x, textY + y);
                                
                        // Draw 'c'
                        for (int y = -5*scale; y <= 5*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX + 5*scale + x, textY + y);
                        for (int x = -5*scale; x <= 5*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + 10*scale + x, textY - 5*scale + y);
                                setTextPixel(textX + 10*scale + x, textY + 5*scale + y);
                            }
                        }
                        break;
                        
                    case 3: // 5 cents
                        // Draw "5c" larger
                        for (int x = -5*scale; x <= 5*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX - 5*scale + x, textY - 10*scale + y);
                                setTextPixel(textX - 5*scale + x, textY + y);
                                setTextPixel(textX - 5*scale + x, textY + 10*scale + y);
                            }
                        }
                        for (int y = -10*scale; y <= 0; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX - 10*scale + x, textY + y);
                        for (int y = 0; y <= 10*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX + x, textY + y);
                                
                        // Draw 'c'
                        for (int y = -5*scale; y <= 5*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX + 5*scale + x, textY + y);
                        for (int x = -5*scale; x <= 5*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + 10*scale + x, textY - 5*scale + y);
                                setTextPixel(textX + 10*scale + x, textY + 5*scale + y);
                            }
                        }
                        break;
                    
                    case 4: // 10 cents - draw "10"
                        // Draw "1"
                        for (int y = -10*scale; y <= 10*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX - 10*scale + x, textY + y);
                        // Draw "0"
                        for (int y = -10*scale; y <= 10*scale; y += scale) {
                            for (int x = 0; x <= scale; x++) {
                                setTextPixel(textX - 2*scale + x, textY + y);
                                setTextPixel(textX + 8*scale + x, textY + y);
                            }
                        }
                        for (int x = -2*scale; x <= 8*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + x, textY - 10*scale + y);
                                setTextPixel(textX + x, textY + 10*scale + y);
                            }
                        }
                        break;
                        
                    case 5: // 20 cents - draw "20"
                        // Draw "2"
                        for (int x = -10*scale; x <= 0; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + x, textY - 10*scale + y);
                                setTextPixel(textX + x, textY + y);
                                setTextPixel(textX + x, textY + 10*scale + y);
                            }
                        }
                        for (int y = -10*scale; y <= 0; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX + x, textY + y);
                        for (int y = 0; y <= 10*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX - 10*scale + x, textY + y);
                        // Draw "0"
                        for (int y = -10*scale; y <= 10*scale; y += scale) {
                            for (int x = 0; x <= scale; x++) {
                                setTextPixel(textX + 5*scale + x, textY + y);
                                setTextPixel(textX + 15*scale + x, textY + y);
                            }
                        }
                        for (int x = 5*scale; x <= 15*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + x, textY - 10*scale + y);
                                setTextPixel(textX + x, textY + 10*scale + y);
                            }
                        }
                        break;
                        
                    case 6: // 50 cents - draw "50"
                        // Draw "5"
                        for (int x = -10*scale; x <= 0; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + x, textY - 10*scale + y);
                                setTextPixel(textX + x, textY + y);
                                setTextPixel(textX + x, textY + 10*scale + y);
                            }
                        }
                        for (int y = -10*scale; y <= 0; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX - 10*scale + x, textY + y);
                        for (int y = 0; y <= 10*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX + x, textY + y);
                        // Draw "0"
                        for (int y = -10*scale; y <= 10*scale; y += scale) {
                            for (int x = 0; x <= scale; x++) {
                                setTextPixel(textX + 5*scale + x, textY + y);
                                setTextPixel(textX + 15*scale + x, textY + y);
                            }
                        }
                        for (int x = 5*scale; x <= 15*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + x, textY - 10*scale + y);
                                setTextPixel(textX + x, textY + 10*scale + y);
                            }
                        }
                        break;
                }
                
                // Draw center dot (white)
                int pos = centerY * bytesperline_src + centerX * channels_src;
                datasrc[pos] = colorCenter[0];
                datasrc[pos + 1] = colorCenter[1];
                datasrc[pos + 2] = colorCenter[2];
                
                // Draw crosshair
                const int crossSize = 8;
                for (int offset = 1; offset <= crossSize; offset++) {
                    // Define the four directions from center
                    const int directions[4][2] = {
                        {offset, 0},    // Right
                        {-offset, 0},   // Left
                        {0, offset},    // Down
                        {0, -offset}    // Up
                    };
                    
                    // Draw in all four directions
                    for (int dir = 0; dir < 4; dir++) {
                        int x = centerX + directions[dir][0];
                        int y = centerY + directions[dir][1];
                        
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            pos = y * bytesperline_src + x * channels_src;
                            datasrc[pos] = colorCenter[0];
                            datasrc[pos + 1] = colorCenter[1];
                            datasrc[pos + 2] = colorCenter[2];
                        }
                    }
                }
            }
        }
        
        return 1;
    }

    /**
     * @brief Processa os blobs detetados na imagem para identificar moedas
     */
    void ProcessImage(IVC *src, OVC *blobs, OVC *blobs2, OVC *blobs3,
                    int nlabels, int nlabels2, int nlabels3,
                    int *excludeList, int *coinCounts)
    {
        // Constants
        const int DISTANCE_THRESHOLD_SQ = 30 * 30;
        const int AREA_MIN_THRESHOLD = 9000;
        
        // Temporary frame coin count
        int frameCoins[8] = {0};

        // Use a bitmap for faster collision detection (1 bit per 4 pixels)
        const int MAP_WIDTH = 320;
        const int MAP_HEIGHT = 240;
        const int MAP_SIZE = MAP_WIDTH * MAP_HEIGHT;
        unsigned char *detectionMap = (unsigned char*)calloc(MAP_SIZE, sizeof(unsigned char));
        
        if (!detectionMap) {
            fprintf(stderr, "Memory allocation failed in ProcessImage\n");
            return;
        }

        // Process main blobs
        for (int i = 0; i < nlabels; i++) {
            OVC *blob = &blobs[i];
            
            // Quick reject based on area and position
            if (blob->area <= AREA_MIN_THRESHOLD || blob->area >= 30000 ||
                blob->yc >= 650 || blob->yc <= 550 || blob->width > 220) {
                    
                // Check if this belongs to the upper region that should be excluded
                if (blob->yc >= 400 && blob->yc <= 550) {
                    ExcludeCoin(excludeList, blob->xc, blob->yc, 1);
                }
                continue;
            }
            
            // Calculate map position (downsized by factor of 2)
            int mapX = blob->xc / 2;
            int mapY = blob->yc / 2;
            
            // Skip if outside bounds
            if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
                continue;
            }
            
            // Get position in detection map
            int mapPos = mapY * MAP_WIDTH + mapX;
            
            // Skip if already detected in this frame
            if (detectionMap[mapPos]) {
                continue;
            }
            
            // Mark as detected
            detectionMap[mapPos] = 1;
            
            // Check exclusion list
            bool isExcluded = false;
            for (int j = 0; j < MAX_COINS; j++) {
                if (excludeList[j * 2] == 0 && excludeList[j * 2 + 1] == 0)
                    continue;
                    
                int dx = excludeList[j * 2] - blob->xc;
                int dy = excludeList[j * 2 + 1] - blob->yc;
                int distance_squared = dx * dx + dy * dy;
                
                if (distance_squared <= DISTANCE_THRESHOLD_SQ) {
                    isExcluded = true;
                    break;
                }
            }
            
            if (isExcluded)
                continue;
            
            // Try to detect coins - first gold coins, then copper if no gold found
            bool coinFound = DetectGoldCoins(blob, blobs2, nlabels2, excludeList, frameCoins, DISTANCE_THRESHOLD_SQ);
            
            if (!coinFound) {
                coinFound = DetectBronzeCoins(blob, blobs3, nlabels3, excludeList, frameCoins, DISTANCE_THRESHOLD_SQ);
            }
        }

        // Update all coin counts (indices 0-7: 1c through 2€)
        for (int i = 0; i < 8; i++) {
            coinCounts[i] += frameCoins[i];
        }

        // Free the detection map
        free(detectionMap);
    }

    /**
     * @brief Processa os frames do vídeo para detetar moedas.
     */
    void ProcessFrame(IVC *frame, IVC *frame2, int *excludeList, int *coinCounts)
    {
        // Increment frame counter for coin tracking
        IncrementFrameCounter();
        
        // Return early if invalid parameters
        if (!frame || !frame2 || !excludeList || !coinCounts) {
            fprintf(stderr, "Invalid parameters in ProcessFrame\n");
            return;
        }

        // Create all pointers as NULL initially for safer cleanup
        IVC *rgbImage = NULL, *hsvImage = NULL, *hsvImage2 = NULL, *hsvSilver = NULL;
        IVC *grayImage = NULL, *grayImage2 = NULL, *grayImage3 = NULL;
        IVC *binaryImage = NULL, *binaryImage2 = NULL, *binaryImage3 = NULL;
        OVC *blobs = NULL, *blobs2 = NULL, *blobs3 = NULL;
        int nlabels = 0, nlabels2 = 0, nlabels3 = 0;
        
        // Allocate all required images in a single block to improve error handling
        rgbImage = vc_image_new(frame->width, frame->height, frame->channels, 255);
        hsvImage = vc_image_new(frame->width, frame->height, 3, 255);
        hsvImage2 = vc_image_new(frame->width, frame->height, 3, 255);
        hsvSilver = vc_image_new(frame->width, frame->height, 3, 255);
        
        grayImage = vc_image_new(frame->width, frame->height, 1, 255);
        grayImage2 = vc_image_new(frame->width, frame->height, 1, 255);
        grayImage3 = vc_image_new(frame->width, frame->height, 1, 255);
        
        binaryImage = vc_image_new(frame->width, frame->height, 1, 255);
        binaryImage2 = vc_image_new(frame->width, frame->height, 1, 255);
        binaryImage3 = vc_image_new(frame->width, frame->height, 1, 255);

        // Check if any allocation failed
        if (!rgbImage || !hsvImage || !hsvImage2 || !hsvSilver ||
            !grayImage || !grayImage2 || !grayImage3 ||
            !binaryImage || !binaryImage2 || !binaryImage3) {
            fprintf(stderr, "Memory allocation failed in ProcessFrame\n");
            goto cleanup;
        }

        // Basic segmentation for all coins
        vc_bgr_to_rgb(frame, rgbImage);
        memcpy(hsvImage->data, rgbImage->data, frame->width * frame->height * 3);
        // No need to process silver areas
        
        vc_rgb_to_gray(rgbImage, grayImage);
        vc_gray_negative(grayImage);
        vc_gray_to_binary(grayImage, binaryImage, 150);

        // IMPROVED: Gold coin segmentation with better HSV thresholds
        vc_rgb_to_hsv(hsvImage, 0); // 0 = gold coin segmentation (adjusted for better gold detection)
        vc_rgb_to_gray(hsvImage, grayImage2);
        vc_gray_to_binary(grayImage2, binaryImage2, 120); // Adjusted threshold
        vc_binary_open(binaryImage2, grayImage2, 7);

        // IMPROVED: Copper coin segmentation with better HSV thresholds
        vc_bgr_to_rgb(frame2, rgbImage);
        memcpy(hsvImage2->data, rgbImage->data, frame->width * frame->height * 3);
        vc_rgb_to_hsv(hsvImage2, 1); // 1 = copper coin segmentation (adjusted for better copper detection)
        vc_rgb_to_gray(hsvImage2, grayImage3);
        vc_gray_to_binary(grayImage3, binaryImage3, 85); // Adjusted threshold
        vc_binary_open(binaryImage3, grayImage3, 3);

        // Blob detection for all coins
        blobs = vc_binary_blob_labelling(binaryImage, binaryImage, &nlabels);
        if (blobs && nlabels > 0) {
            vc_binary_blob_info(binaryImage, blobs, nlabels);
        }

        // Blob detection for gold coins - with improved filtering
        blobs2 = vc_binary_blob_labelling(grayImage2, grayImage2, &nlabels2);
        if (blobs2 && nlabels2 > 0) {
            vc_binary_blob_info(grayImage2, blobs2, nlabels2);
            FilterGoldCoinBlobs(blobs2, nlabels2);
        }

        // Blob detection for copper coins - with improved filtering
        blobs3 = vc_binary_blob_labelling(grayImage3, grayImage3, &nlabels3);
        if (blobs3 && nlabels3 > 0) {
            vc_binary_blob_info(grayImage3, blobs3, nlabels3);
            FilterCopperCoinBlobs(blobs3, nlabels3);
        }

        // Process detected objects for standard coin detection
        ProcessImage(frame, blobs, blobs2, blobs3, nlabels, nlabels2, nlabels3, excludeList, coinCounts);
        
        // Draw visualizations - only pass gold and copper coins (NULL for silver)
        DrawCoins(frame, blobs2, blobs3, nlabels2, nlabels3, NULL, 0);

    cleanup:
        // Free all allocated resources
        if (blobs3) free(blobs3);
        if (blobs2) free(blobs2);
        if (blobs) free(blobs);
        
        if (binaryImage3) vc_image_free(binaryImage3);
        if (binaryImage2) vc_image_free(binaryImage2);
        if (binaryImage) vc_image_free(binaryImage);
        
        if (grayImage3) vc_image_free(grayImage3);
        if (grayImage2) vc_image_free(grayImage2);
        if (grayImage) vc_image_free(grayImage);
        
        if (hsvSilver) vc_image_free(hsvSilver);
        if (hsvImage2) vc_image_free(hsvImage2);
        if (hsvImage) vc_image_free(hsvImage);
        if (rgbImage) vc_image_free(rgbImage);
    }

#ifdef __cplusplus
}
#endif