cmake_minimum_required(VERSION 3.5)
project(CommonChat)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target) 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/target/debug) 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/target/release) 

if(MSVC)
    add_compile_options("/utf-8")
endif()

if(USE_DESKTOP_MODE)
message("Desktop mode: ON")
else()
message("Desktop mode: OFF")
endif()

find_package(libuv CONFIG REQUIRED)
find_package(uvw CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
if(USE_DESKTOP_MODE)
find_package(glfw3 CONFIG REQUIRED)
endif()

# Client
file(GLOB_RECURSE CLI_SRC client/*.cpp)
add_executable(CommonChat ${CLI_SRC})

target_link_libraries(CommonChat PRIVATE uvw::uvw)
if(USE_DESKTOP_MODE)
target_link_libraries(CommonChat PRIVATE glfw)
target_compile_definitions(CommonChat PRIVATE USE_DESKTOP_MODE)
target_include_directories(CommonChat PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(CommonChat PRIVATE ${Vulkan_LIBRARIES})
endif()

add_custom_command(
    TARGET CommonChat
    # COMMAND mkdir ${PROJECT_BINARY_DIR}/assets
    COMMAND glslc ${CMAKE_SOURCE_DIR}/client/shaders/shader.vert -o ${PROJECT_BINARY_DIR}/shader.vert.spv
    COMMAND glslc ${CMAKE_SOURCE_DIR}/client/shaders/shader.frag -o ${PROJECT_BINARY_DIR}/shader.frag.spv
)

# Server
file(GLOB_RECURSE SRV_SRC server/*.cpp)
add_executable(CommonChatSrv ${SRV_SRC})

target_link_libraries(CommonChatSrv PRIVATE uvw::uvw)
