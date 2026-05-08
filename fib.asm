addi x5, x0, 0
addi x6, x0, 1      # check values in x5 and x6 to verigy
addi x7, x0, 10     # Loop counter
addi x10, x0, 32768 # MMIO Address 0x8000

loop:
    add x8, x5, x6 
    sw x8, 0(x10)   # Output sum 
    add x5, x6, x0  # Shift: x5 = x6
    add x6, x8, x0  # Shift: x6 = x8
    addi x7, x7, -1
    beq x7, x0, end
    jal x1, loop

end:
