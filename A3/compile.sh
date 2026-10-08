#!/bin/bash
# Compiles the C++ source code into an executable named 'metro_planner'.
# The g++ version 4.8.1 specified in the assignment supports C++11.
echo "Compiling metro_planner.cpp..."
g++ -o metro_planner metro_planner.cpp
echo "Compilation finished."