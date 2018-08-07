# cmake needs this line
cmake_minimum_required(VERSION 2.8)

# Define project name
project( ConsoleVideo )

# Use c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

ADD_SUBDIRECTORY("./src")
