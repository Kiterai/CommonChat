cmake_minimum_required(VERSION 3.5)
project(CommonChat)

find_package(libuv CONFIG REQUIRED)
find_package(uvw CONFIG REQUIRED)

# Client
file(GLOB_RECURSE CLI_SRC client/*.cpp)
add_executable(CommonChat ${CLI_SRC})

target_link_libraries(CommonChat PRIVATE $<IF:$<TARGET_EXISTS:uv_a>,uv_a,uv>)
target_link_libraries(CommonChat PRIVATE uvw::uvw)

# Server
file(GLOB_RECURSE SRV_SRC server/*.cpp)
add_executable(CommonChatSrv ${SRV_SRC})

target_link_libraries(CommonChatSrv PRIVATE $<IF:$<TARGET_EXISTS:uv_a>,uv_a,uv>)
target_link_libraries(CommonChatSrv PRIVATE uvw::uvw)
