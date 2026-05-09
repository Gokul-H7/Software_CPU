run these instructions within the same directory

Usage instructions:

1. g++ -o assembler assembler.cpp

2. g++ -o cpu cpu.cpp

3. ./assembler program.asm -> generates output.bin

4. ./cpu output.bin [-c] [-v]




[-c/-v] are optional mode for character and verbose

-c prints MMIO output encoded as ASCII characters (use with hello_world.asm)

-v prints out the fetch/decode/execute cycles for each instruction (use with timer to see execution cycles)

All CPU registers will be printed to terminal to verify

Videos for fib and recursive add tests are available within the repository.
recursive_add.asm was manually translated from recursive_add.c
