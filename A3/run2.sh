#!/bin/bash
# Executes the decoder part of the compiled C++ program.
# Usage: ./run2.sh <basename>
# It will read <basename>.city and <basename>.satoutput,
# and produce <basename>.metromap

if [ $# -lt 1 ]; then
    echo "Usage: $0 <basename (without .city)>"
    exit 1
fi
./metro_planner run2 "$1"