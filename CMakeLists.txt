cmake_minimum_required(VERSION 3.20)
project(OS_Ex2)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(OS_Ex2
#        demo_itimer.c
#        demo_jmp.c
#        demo_singInt_handler.c
        thread.h
        uthreads.cpp
        uthreads.h
        main.cpp)
