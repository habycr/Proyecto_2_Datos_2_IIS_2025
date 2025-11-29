# 🔧 Configura VCPKG como toolchain (antes del project)
set(CMAKE_TOOLCHAIN_FILE "C:/Users/Javier/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain")

cmake_minimum_required(VERSION 3.15)
project(CodeCoachDBTest)

# 🧠 Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 📦 MongoDB (C++ driver)
find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)

# 📦 Crow (framework HTTP) – solo para los includes (es header-only)
find_package(crow CONFIG REQUIRED)

# 📦 Asio (requerido por Crow)
find_package(asio CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)


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
        cpr::cpr
)
add_executable(analyzer_server
        Analizador/main.cpp
        Analizador/Analyzer/analizador.h
        Analizador/Analyzer/analizador.cpp
        Analizador/LLM/gemini_client.cpp
        Analizador/LLM/gemini_client.h
        Analizador/Env/env_loader.cpp
        Analizador/Env/env_loader.h
)

target_include_directories(analyzer_server
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Analizador
)

# IMPORTANTE: Linkear con Crow y Asio
target_link_libraries(analyzer_server
        PRIVATE
        Crow::Crow
        asio::asio
)

# Configuración específica para Windows y MSVC
if(MSVC)
    target_compile_options(analyzer_server PRIVATE
            /utf-8
            /W3
            /permissive-
    )
    target_compile_definitions(analyzer_server PRIVATE
            _CRT_SECURE_NO_WARNINGS
            _WIN32_WINNT=0x0A00
            NOMINMAX
            WIN32_LEAN_AND_MEAN
    )

    # Linkear bibliotecas de red de Windows
    target_link_libraries(analyzer_server PRIVATE
            ws2_32
            wsock32
            mswsock
            Crow::Crow
            CURL::libcurl
            nlohmann_json::nlohmann_json
    )
endif()

configure_file(${CMAKE_SOURCE_DIR}/.env
        ${CMAKE_BINARY_DIR}/.env COPYONLY)

# Para Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(analyzer_server PRIVATE /O2)
    endif()
endif()

# con eso CLion verá el submódulo y te creará la configuración engine_demo
add_subdirectory(evaluation_engine)
