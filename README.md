g++ -o assembler assembler.cpp

g++ -o cpu cpu.cpp

./assembler program.asm

./cpu output.bin [-c] [-v]

[-c/-v] are optional mode for character and verbose
-c prints MMIO output encoded as ASCII characters (use with hello_world.asm)
-v prints out the fetch/decode/execute cycles for each instruction

All CPU registers will be printed to terminal to verify
