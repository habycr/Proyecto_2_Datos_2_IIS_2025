# 🔧 Configura VCPKG como toolchain (antes del project)
set(CMAKE_TOOLCHAIN_FILE "C:/Users/dylan/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


cmake_minimum_required(VERSION 3.15)
project(CodeCoachDBTest)

# 🧠 Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 📦 MongoDB (C++ driver)
find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)


# 📦 Crow (framework HTTP) – solo para los includes
find_package(crow CONFIG REQUIRED)

# ============================================================
# Ejecutable 1: Prueba de BD (main_db_test)
# ============================================================
add_executable(CodeCoachDBTest
        Gestor/main_db_test.cpp
        Gestor/src/ProblemRepository.cpp
)

target_include_directories(CodeCoachDBTest
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Gestor/include
)

target_link_libraries(CodeCoachDBTest
        PRIVATE
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
)

# ============================================================
# Ejecutable 2: Servidor REST (problem_manager_api)
# ============================================================
add_executable(problem_manager_api
        Gestor/GestorREST.cpp
        Gestor/src/ProblemRepository.cpp
)

target_include_directories(problem_manager_api
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Gestor/include
)

# 👇 SOLO MongoDB; Crow NO se enlaza porque es header-only
target_link_libraries(problem_manager_api
        PRIVATE
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
)
