#!/bin/bash
# Executes the encoder part of the compiled C++ program.
# Usage: ./run1.sh <basename>
# It will read <basename>.city and produce <basename>.satinput

if [ $# -lt 1 ]; then
    echo "Usage: $0 <basename (without .city)>"
    exit 1
fi
./metro_planner run1 "$1"