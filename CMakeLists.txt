cmake_minimum_required(VERSION 3.10)
project(linggo)

include_directories(include thr)

find_package(libhv REQUIRED)

add_executable(linggo
        src/main.c
        src/server.c
        src/utils.c
        src/voc.c
        src/ai.c
        src/user.c
        src/utils.c
        src/error.c
        thr/json-parser/json.c
        thr/json-builder/json-builder.c
        include/linggo/user.h)
target_link_libraries(linggo hv)
