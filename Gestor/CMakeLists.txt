# ============================================================================
# Este CMake no crea ejecutables.
# Define una librería llamada "gestor_lib" que contiene la capa de acceso
# a MongoDB (ProblemRepository).
# ============================================================================

cmake_minimum_required(VERSION 3.15)

# Compilar la librería con el código del repositorio
add_library(gestor_lib
        src/ProblemRepository.cpp   # implementación del repositorio
)

# Incluir headers desde Gestor/include para que otros targets los vean
target_include_directories(gestor_lib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Vincular la librería con los drivers de MongoDB
target_link_libraries(gestor_lib
        PUBLIC
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
)
