cmake_minimum_required(VERSION 3.10)
project(linggo)

include_directories(include)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
add_executable(linggo src/main.c src/server.c src/user.c src/utils.c src/voc.c src/email.c)
target_link_libraries(linggo Threads::Threads curl)
