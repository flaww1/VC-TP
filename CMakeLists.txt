cmake_minimum_required(VERSION 3.21)  # Utiliza a versão 3.21 ou superior

project(VisaoComputador LANGUAGES C CXX)

# Explicitly specify only the components we need
set(OpenCV_FIND_COMPONENTS core imgproc highgui imgcodecs videoio)

# Explicitly exclude problematic components
set(OpenCV_FIND_EXCLUDE_COMPONENTS viz hdf)

# Add this option to prevent auto-linking of all components
set(OpenCV_LIBS_EXCLUDES "opencv_viz;opencv_hdf")

add_subdirectory(lib)

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

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(VisaoComputador main.cpp)

target_include_directories(VisaoComputador PUBLIC
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(VisaoComputador PUBLIC
    vclib
    ${FILTERED_OPENCV_LIBS}
)

# Copy video files to the build directory
set(VIDEO_FILES
    "${CMAKE_SOURCE_DIR}/video1.mp4"
    "${CMAKE_SOURCE_DIR}/video2.mp4"
)

foreach(VIDEO_FILE ${VIDEO_FILES})
    get_filename_component(FILENAME ${VIDEO_FILE} NAME)
    add_custom_command(
        TARGET VisaoComputador POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VIDEO_FILE}" "$<TARGET_FILE_DIR:VisaoComputador>"
        COMMENT "A copiar ficheiro de vídeo para o diretório de saída"
    )
endforeach()