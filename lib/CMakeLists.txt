# Adiciona a biblioteca com todos os ficheiros fonte necessários
add_library(vclib 
    vc_blob.cpp
    vc_coin.cpp
    vc_core.cpp
    vc_utils.cpp
    vc_coin_detection.cpp
    vc_frame_processor.cpp
)

# Procura e configura o OpenCV
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui imgcodecs videoio)

# Filter out problematic libraries
set(FILTERED_OPENCV_LIBS "")
foreach(lib ${OpenCV_LIBS})
    if(NOT ${lib} MATCHES "opencv_viz|opencv_hdf")
        list(APPEND FILTERED_OPENCV_LIBS ${lib})
    endif()
endforeach()

# Adiciona os diretórios de inclusão do OpenCV
include_directories(${OpenCV_INCLUDE_DIRS})

# Liga a biblioteca apenas às bibliotecas do OpenCV necessárias
target_link_libraries(vclib ${FILTERED_OPENCV_LIBS})