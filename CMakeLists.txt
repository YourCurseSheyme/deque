cmake_minimum_required(VERSION 3.26)
#project(deque)

set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_C_STANDARD 17)

# For sanitizers
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,undefined -fno-sanitize-recover=all -g -lm")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined,leak -fno-sanitize-recover=all -g -lm")

# For Valgrind
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lm")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lm")

add_executable(deque main.cpp)