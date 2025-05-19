cmake_minimum_required(VERSION 3.21)  # Utiliza a versão 3.21 ou superior

project(CoinDetector LANGUAGES C CXX)

# Set C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Explicitly specify only the components we need
set(OpenCV_FIND_COMPONENTS core imgproc highgui imgcodecs videoio)

# Explicitly exclude problematic components
set(OpenCV_FIND_EXCLUDE_COMPONENTS viz hdf)

# Add this option to prevent auto-linking of all components
set(OpenCV_LIBS_EXCLUDES "opencv_viz;opencv_hdf")

add_subdirectory(lib)

# Find required packages
find_package(OpenCV REQUIRED COMPONENTS ${OpenCV_FIND_COMPONENTS})

if(OpenCV_FOUND)
    message(STATUS "OpenCV encontrado na versão: ${OpenCV_VERSION}")
    message(STATUS "Diretórios de inclusão do OpenCV: ${OpenCV_INCLUDE_DIRS}")
    
    # Filter out problematic libraries
    set(FILTERED_OPENCV_LIBS "")
    foreach(lib ${OpenCV_LIBS})
        if(NOT ${lib} MATCHES "opencv_viz|opencv_hdf")
            list(APPEND FILTERED_OPENCV_LIBS ${lib})
        endif()
    endforeach()
    
    message(STATUS "Bibliotecas do OpenCV filtradas: ${FILTERED_OPENCV_LIBS}")
endif()

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Create a library with all our VC functions
file(GLOB LIB_SOURCES 
    "${CMAKE_SOURCE_DIR}/lib/*.cpp"
)

add_library(vc STATIC ${LIB_SOURCES})

# Create the main executable
add_executable(coin_detector ${CMAKE_SOURCE_DIR}/src/main.cpp)

# Link libraries
target_link_libraries(coin_detector 
    vc
    ${FILTERED_OPENCV_LIBS}
)

# Add a README file
file(WRITE ${CMAKE_SOURCE_DIR}/README.md "# Coin Detector

This project detects and classifies Euro coins in videos.

## Authors
- Daniel - 26432
- Maria - 26438
- Bruno - 26014
- Flávio - 21110

## Features
- Detects 1, 2, 5, 10, 20, 50 cents, 1€ and 2€ coins
- Tracks coins across multiple frames
- Calculates the total monetary value
- Displays coin labels and statistics

## Usage
```
./coin_detector
```
")

# Create install rules
install(TARGETS coin_detector DESTINATION bin)

# Copy video files to the build directory
set(VIDEO_FILES
    "${CMAKE_SOURCE_DIR}/video1.mp4"
    "${CMAKE_SOURCE_DIR}/video2.mp4"
)

foreach(VIDEO_FILE ${VIDEO_FILES})
    get_filename_component(FILENAME ${VIDEO_FILE} NAME)
    add_custom_command(
        TARGET coin_detector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VIDEO_FILE}" "$<TARGET_FILE_DIR:coin_detector>"
        COMMENT "A copiar ficheiro de vídeo para o diretório de saída"
    )
endforeach()