project("vulkan_model_loader")

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

set(SOURCES
    "src/file_ops.cpp"
    "src/game_engine.cpp"
    "src/main.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    ${PROJECT_NAME}
    glfw
    Vulkan::Vulkan
)

configure_file(${PROJECT_SOURCE_DIR}/shaders/vert.spv shaders/vert.spv COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/shaders/frag.spv shaders/frag.spv COPYONLY)
