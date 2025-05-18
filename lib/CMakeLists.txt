# Adiciona a biblioteca com todos os ficheiros fonte necessários
add_library(vclib 
    vc_core_functions.cpp               # Funções básicas de manipulação de imagens
    vc_conversion_functions.cpp         # Funções de conversão entre espaços de cor
    vc_binary_functions.cpp             # Funções para processamento de imagens binárias
    vc_filter_functions.cpp             # Funções de filtragem de imagens
    vc_analysis_functions.cpp           # Funções para análise de imagens
    vc_coin_functions.cpp               # Funções principais para moedas
    vc_coin_utils.cpp                   # Funções utilitárias para moedas
    vc_coin_detection.cpp               # Funções para detecção de moedas
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