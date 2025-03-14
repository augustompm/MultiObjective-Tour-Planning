cmake_minimum_required(VERSION 3.15)
project(tourist_route VERSION 1.0)

# Requer C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Opções de compilação
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Adiciona diretório de headers
include_directories(${PROJECT_SOURCE_DIR}/include)

# Lista todos os source files para a biblioteca original
set(SOURCES
    src/models.cpp
    src/utils.cpp
    src/hypervolume.cpp
    src/nsga2-base.cpp  
)

# Lista de arquivos fonte do MOVNS
set(MOVNS_SOURCES
    src/movns-solution.cpp
    src/movns-utils.cpp
    src/movns-neighborhood.cpp
    src/movns-algorithm.cpp
    src/movns-metrics.cpp
)

# Cria biblioteca estática
add_library(tourist_lib STATIC ${SOURCES})

# Cria biblioteca separada para MOVNS
add_library(movns_lib STATIC ${MOVNS_SOURCES})
target_link_libraries(movns_lib PRIVATE tourist_lib)

# Cria executável para o algoritmo original
add_executable(tourist_route src/main.cpp)
target_link_libraries(tourist_route PRIVATE tourist_lib)

# Cria executável para o MOVNS
add_executable(movns_main src/movns-main.cpp)
target_link_libraries(movns_main PRIVATE movns_lib tourist_lib)

# Copia arquivos de dados para o diretório de build
file(COPY ${PROJECT_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})

# Configuração do diretório de build
set_target_properties(tourist_route PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(movns_main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Mensagem informativa
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")