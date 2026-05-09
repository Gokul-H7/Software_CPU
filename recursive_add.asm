# recursive sum: recursive_add(6) = 6 + 5 + 4 + 3 + 2 + 1 = 21

addi x2, x0, 2000      # stack pointer
addi x10, x0, 6        # argument n = 6

jal x1, add_func       # call add_func

addi x6, x0, 1024      # MMIO_PRINT address = 0x400
sw x10, 0(x6)          # print final result through MMIO

beq x0, x0, end        # stop after printing

add_func:
    beq x10, x0, base_case

    addi x2, x2, -8
    sw x1, 4(x2)       # save return address
    sw x10, 0(x2)      # save n

    addi x10, x10, -1
    jal x1, add_func

    lw x5, 0(x2)       # restore saved n
    add x10, x10, x5   # result += n

    lw x1, 4(x2)       # restore return address
    addi x2, x2, 8
    jalr x0, 0(x1)     # return

base_case:
    addi x10, x0, 0
    jalr x0, 0(x1)

end: