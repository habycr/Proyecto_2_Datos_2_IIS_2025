# 🔧 Configura VCPKG como toolchain (antes del project)
set(CMAKE_TOOLCHAIN_FILE "C:/Users/Javier/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain")

cmake_minimum_required(VERSION 3.15)
project(CodeCoachDBTest)

# 🧠 Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 📦 Busca los paquetes de MongoDB (C++ driver)
find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)

# 🎯 Ejecutable principal
add_executable(CodeCoachDBTest main.cpp)

# 🔗 Enlaza con las bibliotecas de MongoDB
target_link_libraries(CodeCoachDBTest
        PRIVATE
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
)
