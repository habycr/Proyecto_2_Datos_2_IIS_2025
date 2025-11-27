# Subproyecto del motor de evaluación
find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(engine_core
        src/SubmissionFilesystem.cpp
        src/DockerRunner.cpp
        src/OutputComparer.cpp
        src/EvaluationService.cpp
)

target_include_directories(engine_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(engine_demo
        src/demo_main.cpp
)

target_link_libraries(engine_demo PRIVATE engine_core)

target_include_directories(engine_demo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(evaluation_server
        src/server_main.cpp
)

target_link_libraries(evaluation_server PRIVATE
        engine_core
        Crow::Crow
        nlohmann_json::nlohmann_json
)

target_include_directories(evaluation_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
