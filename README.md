g++ -o X assembler.cpp
g++ -o Y cpu.cpp
./X Z.asm
./Y output.bin

CPU registers will be printed to terminal to verify
