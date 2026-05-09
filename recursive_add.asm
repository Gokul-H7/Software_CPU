addi x5, x0, 5      # recursive add
addi x6, x0, 0      # sum
addi x7, x0, 1
addi x2, x0, 16384  # stack pointer address 0x4000
addi x10, x0, 32768 # MMIO Address 0x8000

return:
    jal recurse
    sw x11, 0(x10)
    beq x0, x0, end
recurse:
    addi x2, x2, -8
    sw x5, 4(x2)    # store current value on stack
    sw x1, 0(x2)    # store return address on stack
    beq x5, x7, calc
    addi x11, x0, 0
    jal x1

calc:
    addi x5, x5, -1
    jal recurse
    lw x1, 0(x2)
    lw x5, 4(x2)
    addi x2, x2, 8
    add x11, x5, x11
    jalr x0, x1, 0
end: