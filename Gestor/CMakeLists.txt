# Este archivo NO genera ejecutables.
# Solo define la librería "gestor" para que la raíz pueda usarla.

cmake_minimum_required(VERSION 3.15)

# Crear una librería con el código del gestor
add_library(gestor_lib
        src/ProblemRepository.cpp
)

# Incluir los headers dentro de Gestor/include
target_include_directories(gestor_lib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Enlazar mongo C++ driver a esta librería
target_link_libraries(gestor_lib
        PUBLIC
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
)
