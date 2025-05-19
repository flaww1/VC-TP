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
        
        // Define colors for gold, copper, and euro coins with more contrast (BGR format)
        const unsigned char colorCopper[3] = {0, 80, 255};     // Copper/bronze coins - orange (BGR)
        const unsigned char colorGold[3] = {0, 215, 255};      // Gold coins - yellow gold (BGR)
        const unsigned char colorEuro[3] = {255, 130, 0};      // Euro coins - blue (BGR)
        const unsigned char colorCenter[3] = {255, 255, 255};  // White for centers
        const unsigned char colorText[3] = {255, 255, 255};    // White for text labels
        
        // Choose fixed color based on coin type
        const unsigned char *baseColor;
        if (coinType == 0) baseColor = colorCopper;       // Copper
        else if (coinType == 1) baseColor = colorGold;    // Gold
        else baseColor = colorEuro;                       // Euro (new)
        
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
            } else if (coinType == 2) { // Euro coins (new)
                // Check if this is likely a 2 Euro coin based on multiple factors
                bool likely2Euro = (circularDiameter >= 140.0f || // Larger diameter
                                   blobs[i].area >= 14500 ||     // Larger area
                                   blobs[i].width >= 142 ||      // Wider
                                   blobs[i].height >= 142);      // Taller
                                   
                // Force 2€ for all larger parts - this is the fix for the video
                if (likely2Euro) {
                    actualCoinType = 8;       // 2€
                    coinLabel = "2€";
                } else {
                    actualCoinType = 7;       // 1€
                    coinLabel = "1€";
                }
            }
            
            // Check if this coin has been detected before (for tracking only - don't count)
            if (actualCoinType > 0) {
                // Pass 0 as last parameter to just track without counting
                IsCoinAlreadyDetected(blobs[i].xc, blobs[i].yc, actualCoinType, 0);
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

                    case 7: // 1 Euro (new)
                        // Draw "1€" larger
                        // Draw "1"
                        for (int y = -10*scale; y <= 10*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX - 5*scale + x, textY + y);
                        
                        // Draw '€' symbol
                        for (int x = -2*scale; x <= 5*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + 5*scale + x, textY - 5*scale + y); // Top horizontal
                                setTextPixel(textX + 5*scale + x, textY + y);           // Middle horizontal
                            }
                        }
                        for (int y = -5*scale; y <= 5*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX + 5*scale + x, textY + y);           // Vertical line
                        break;
                        
                    case 8: // 2 Euro (new)
                        // Draw "2€" larger
                        // Draw "2"
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
                        
                        // Draw '€' symbol
                        for (int x = -2*scale; x <= 5*scale; x += scale) {
                            for (int y = 0; y <= scale; y++) {
                                setTextPixel(textX + 5*scale + x, textY - 5*scale + y); // Top horizontal
                                setTextPixel(textX + 5*scale + x, textY + y);           // Middle horizontal
                            }
                        }
                        for (int y = -5*scale; y <= 5*scale; y += scale) 
                            for (int x = 0; x <= scale; x++) 
                                setTextPixel(textX + 5*scale + x, textY + y);           // Vertical line
                        break;

                    default:
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
    void ProcessImage(IVC *src, OVC *blobs, OVC *blobs2, OVC *blobs3, OVC *blobs4,
                    int nlabels, int nlabels2, int nlabels3, int nlabels4,
                    int *excludeList, int *coinCounts)
    {
        // Constants
        const int DISTANCE_THRESHOLD_SQ = 30 * 30;
        const int AREA_MIN_THRESHOLD = 9000;
        
        // Frame dimensions for coin detection
        const int FRAME_WIDTH = src->width;
        const int FRAME_HEIGHT = src->height;
        
        // Margin to ensure full coin visibility
        const int FULL_VISIBILITY_MARGIN = 20;
        
        // Use a bitmap for faster collision detection (1 bit per 4 pixels)
        const int MAP_WIDTH = FRAME_WIDTH / 2;
        const int MAP_HEIGHT = FRAME_HEIGHT / 2;
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
                blob->width > 220) {
                continue;
            }
            
            // Calculate blob radius from area for checking full visibility
            float diameter = CalculateCircularDiameter(blob);
            float radius = diameter / 2.0f;
            
            // Check if the coin is fully visible with margin
            if (blob->xc - radius < FULL_VISIBILITY_MARGIN || 
                blob->xc + radius >= FRAME_WIDTH - FULL_VISIBILITY_MARGIN || 
                blob->yc - radius < FULL_VISIBILITY_MARGIN || 
                blob->yc + radius >= FRAME_HEIGHT - FULL_VISIBILITY_MARGIN) {
                continue; // Skip partially visible coins
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
            
            // Create temporary counters for this frame only
            int frameCoins[8] = {0}; // Updated to 8 to include Euro coins
            
            // Try to detect coins using improved algorithms - only checking for fully visible coins
            bool coinFound = DetectGoldCoinsImproved(blob, blobs2, nlabels2, excludeList, frameCoins, DISTANCE_THRESHOLD_SQ);
            
            if (!coinFound) {
                coinFound = DetectBronzeCoinsImproved(blob, blobs3, nlabels3, excludeList, frameCoins, DISTANCE_THRESHOLD_SQ);
            }

            // NEW: Try to detect Euro coins
            if (!coinFound && blobs4 && nlabels4 > 0) {
                coinFound = DetectEuroCoinsImproved(blob, blobs4, nlabels4, excludeList, frameCoins, DISTANCE_THRESHOLD_SQ);
            }
            
            // Add frame counts to total counts
            for (int j = 0; j < 8; j++) { // Updated to 8 to include Euro coins
                coinCounts[j] += frameCoins[j];
            }
        }

        // Free the detection map
        free(detectionMap);
    }

    /**
     * @brief Processa os frames do vídeo para detetar moedas.
     */
    void ProcessFrame(IVC *frame, IVC *frame2, int *excludeList, int *coinCounts)
    {
        // Initialize only once, then store static variables
        static bool initialized = false;
        static IVC *rgbImage = NULL, *hsvImage = NULL, *hsvImage2 = NULL, *hsvImage3 = NULL;
        static IVC *grayImage = NULL, *grayImage2 = NULL, *grayImage3 = NULL, *grayImage4 = NULL;
        static IVC *binaryImage = NULL, *binaryImage2 = NULL, *binaryImage3 = NULL, *binaryImage4 = NULL;
        static IVC *edgeImage = NULL;
        
        // First-time initialization
        if (!initialized) {
            memset(coinCounts, 0, 8 * sizeof(int));
            initialized = true;
        }
        
        // Increment frame counter
        IncrementFrameCounter();
        
        // Basic parameter validation
        if (!frame || !frame2 || !excludeList || !coinCounts) 
            return;

        // Get frame dimensions
        const int width = frame->width;
        const int height = frame->height;
        const int channels = frame->channels;
        const int size = width * height * channels;
        
        // Allocate image buffers only once
        if (!rgbImage) {
            rgbImage = vc_image_new(width, height, channels, 255);
            hsvImage = vc_image_new(width, height, channels, 255);
            hsvImage2 = vc_image_new(width, height, channels, 255);
            hsvImage3 = vc_image_new(width, height, channels, 255);
            
            grayImage = vc_image_new(width, height, 1, 255);
            grayImage2 = vc_image_new(width, height, 1, 255);
            grayImage3 = vc_image_new(width, height, 1, 255);
            grayImage4 = vc_image_new(width, height, 1, 255);
            
            binaryImage = vc_image_new(width, height, 1, 255);
            binaryImage2 = vc_image_new(width, height, 1, 255);
            binaryImage3 = vc_image_new(width, height, 1, 255);
            binaryImage4 = vc_image_new(width, height, 1, 255);
            
            edgeImage = vc_image_new(width, height, 1, 255);
            
            // Handle allocation failure
            if (!rgbImage || !hsvImage || !hsvImage2 || !hsvImage3 ||
                !grayImage || !grayImage2 || !grayImage3 || !grayImage4 ||
                !binaryImage || !binaryImage2 || !binaryImage3 || !binaryImage4 ||
                !edgeImage) {
                return;
            }
        }

        // Convert BGR to RGB efficiently
        vc_bgr_to_rgb(frame, rgbImage);
        
        // Process gold coins
        memcpy(hsvImage->data, rgbImage->data, size);
        vc_rgb_to_hsv(hsvImage, 0);  
        vc_rgb_to_gray(hsvImage, grayImage2);
        vc_gray_to_binary(grayImage2, binaryImage2, 110);
        vc_binary_open(binaryImage2, grayImage2, 7);

        // Process copper coins
        vc_bgr_to_rgb(frame2, rgbImage);
        memcpy(hsvImage2->data, rgbImage->data, size);
        vc_rgb_to_hsv(hsvImage2, 1);
        vc_rgb_to_gray(hsvImage2, grayImage3);
        vc_gray_to_binary(grayImage3, binaryImage3, 80);
        vc_binary_open(binaryImage3, grayImage3, 3);

        // Process Euro coins
        vc_bgr_to_rgb(frame, rgbImage);
        memcpy(hsvImage3->data, rgbImage->data, size);
        vc_rgb_to_hsv(hsvImage3, 2);
        vc_rgb_to_gray(hsvImage3, grayImage4);
        vc_gray_to_binary(grayImage4, binaryImage4, 90);
        vc_binary_open(binaryImage4, grayImage4, 3);
        
        // Extract gray image for general blob detection
        vc_rgb_to_gray(rgbImage, grayImage);
        vc_gray_negative(grayImage);
        vc_gray_to_binary(grayImage, binaryImage, 150);
        vc_binary_open(binaryImage, binaryImage, 3);
        vc_binary_close(binaryImage, binaryImage, 5);

        // Blob detection and processing
        int nlabels = 0, nlabels2 = 0, nlabels3 = 0, nlabels4 = 0;
        OVC *blobs = NULL, *blobs2 = NULL, *blobs3 = NULL, *blobs4 = NULL;
        
        // Only proceed if we can extract main blobs
        blobs = vc_binary_blob_labelling(binaryImage, binaryImage, &nlabels);
        if (blobs && nlabels > 0) {
            vc_binary_blob_info(binaryImage, blobs, nlabels);
            
            // Process gold coins
            blobs2 = vc_binary_blob_labelling(grayImage2, grayImage2, &nlabels2);
            if (blobs2 && nlabels2 > 0) {
                vc_binary_blob_info(grayImage2, blobs2, nlabels2);
                // No filtering needed here as it's done in ProcessImage
            }
            
            // Process copper coins
            blobs3 = vc_binary_blob_labelling(grayImage3, grayImage3, &nlabels3);
            if (blobs3 && nlabels3 > 0) {
                vc_binary_blob_info(grayImage3, blobs3, nlabels3);
                // No filtering needed here as it's done in ProcessImage
            }
            
            // Process Euro coins
            blobs4 = vc_binary_blob_labelling(grayImage4, grayImage4, &nlabels4);
            if (blobs4 && nlabels4 > 0) {
                vc_binary_blob_info(grayImage4, blobs4, nlabels4);
                // Force all reasonable blobs to be valid
                for (int i = 0; i < nlabels4; i++) {
                    if (blobs4[i].area > 5000 && blobs4[i].label == 0) {
                        blobs4[i].label = 999;
                    }
                }
            }
            
            // Process detected objects
            ProcessImage(frame, blobs, blobs2, blobs3, blobs4, 
                       nlabels, nlabels2, nlabels3, nlabels4, 
                       excludeList, coinCounts);
            
            // Draw coins for visualization
            DrawCoins(frame, blobs2, blobs3, blobs4, nlabels2, nlabels3, nlabels4);
        }

        // Show summary of current counts every 30 frames
        if (GetFrameCounter() % 30 == 0) {
            float total = coinCounts[0] * 0.01f + coinCounts[1] * 0.02f + 
                         coinCounts[2] * 0.05f + coinCounts[3] * 0.10f + 
                         coinCounts[4] * 0.20f + coinCounts[5] * 0.50f +
                         coinCounts[6] * 1.00f + coinCounts[7] * 2.00f; // Add Euro coins
            
            printf("\n[COIN SUMMARY] Frame %d\n", GetFrameCounter());
            printf("1c: %d (%.2f€), 2c: %d (%.2f€), 5c: %d (%.2f€)\n", 
                   coinCounts[0], coinCounts[0] * 0.01f,
                   coinCounts[1], coinCounts[1] * 0.02f,
                   coinCounts[2], coinCounts[2] * 0.05f);
            printf("10c: %d (%.2f€), 20c: %d (%.2f€), 50c: %d (%.2f€)\n", 
                   coinCounts[3], coinCounts[3] * 0.10f,
                   coinCounts[4], coinCounts[4] * 0.20f,
                   coinCounts[5], coinCounts[5] * 0.50f);
            printf("1€: %d (%.2f€), 2€: %d (%.2f€)\n", 
                   coinCounts[6], coinCounts[6] * 1.00f,
                   coinCounts[7], coinCounts[7] * 2.00f);
            printf("Total coins: %d | Total value: %.2f EUR\n", 
                   coinCounts[0] + coinCounts[1] + coinCounts[2] + 
                   coinCounts[3] + coinCounts[4] + coinCounts[5] +
                   coinCounts[6] + coinCounts[7], total);
        }

        // Cleanup
        if (blobs) free(blobs);
        if (blobs2) free(blobs2);
        if (blobs3) free(blobs3);
        if (blobs4) free(blobs4);

        // Note: We don't free image buffers as they're reused between frames
    }

    /**
     * @brief Desenha visualizações de moedas com suporte para Euro coins
     */
    void DrawCoins(IVC *frame, OVC *goldBlobs, OVC *copperBlobs, OVC *euroBlobs,
                  int nGoldBlobs, int nCopperBlobs, int nEuroBlobs) {
        // Draw Euro coins first (they have priority)
        if (euroBlobs && nEuroBlobs > 0) {
            // Find best Euro blob in one pass instead of allocating memory
            OVC bestEuro = {0};
            bool foundValidEuro = false;
            
            float bestCircularity = 0.0f;
            int bestArea = 0;
            
            // Constants
            const int MAX_VALID_AREA = 100000;
            const int MIN_VALID_AREA = 12000;
            
            // Find complete Euro coins first
            for (int i = 0; i < nEuroBlobs; i++) {
                if (euroBlobs[i].area > MAX_VALID_AREA)
                    continue;
                    
                const float diameter = CalculateCircularDiameter(&euroBlobs[i]);
                const float circularity = CalculateCircularity(&euroBlobs[i]);
                
                // Complete Euro detection
                if (diameter >= 175.0f && diameter <= 210.0f && circularity >= 0.75f) {
                    // Skip if this is a gold coin position
                    const int lastType = GetLastCoinTypeAtLocation(euroBlobs[i].xc, euroBlobs[i].yc);
                    if (lastType >= 4 && lastType <= 6)
                        continue;
                        
                    memcpy(&bestEuro, &euroBlobs[i], sizeof(OVC));
                    bestEuro.label = 999;
                    foundValidEuro = true;
                    break; // First match is sufficient
                }
            }
            
            // If no complete coins found, look for valid parts
            if (!foundValidEuro) {
                for (int i = 0; i < nEuroBlobs; i++) {
                    if (euroBlobs[i].area < MIN_VALID_AREA || euroBlobs[i].area > MAX_VALID_AREA)
                        continue;
                        
                    const float circularity = CalculateCircularity(&euroBlobs[i]);
                    
                    if (circularity >= 0.65f && 
                        euroBlobs[i].width >= 130 && euroBlobs[i].height >= 130 &&
                        euroBlobs[i].area > bestArea) {
                        
                        // Skip if this is a gold coin position
                        const int lastType = GetLastCoinTypeAtLocation(euroBlobs[i].xc, euroBlobs[i].yc);
                        if (lastType >= 4 && lastType <= 6)
                            continue;
                            
                        memcpy(&bestEuro, &euroBlobs[i], sizeof(OVC));
                        bestEuro.label = 999;
                        bestArea = euroBlobs[i].area;
                        foundValidEuro = true;
                    }
                }
                
                // Expand bounding box for 2€ coins
                if (foundValidEuro && bestEuro.area >= 14000) {
                    bestEuro.width = 220;
                    bestEuro.height = 220;
                    bestEuro.x = bestEuro.xc - bestEuro.width/2;
                    bestEuro.y = bestEuro.yc - bestEuro.height/2;
                    bestEuro.area = 35000;
                }
            }
            
            // Draw the best Euro coin if found
            if (foundValidEuro) {
                DrawBoundingBoxes(frame, &bestEuro, 1, 2);
            }
        }
        
        // Draw copper and gold coins
        if (copperBlobs && nCopperBlobs > 0) {
            DrawBoundingBoxes(frame, copperBlobs, nCopperBlobs, 0);
        }
        
        if (goldBlobs && nGoldBlobs > 0) {
            DrawBoundingBoxes(frame, goldBlobs, nGoldBlobs, 1);
        }
    }

#ifdef __cplusplus
}
#endif