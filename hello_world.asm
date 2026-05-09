# run with -c flag to show characters on cpu output
addi x10, x0, 1024     # MMIO_PRINT address

addi x5, x0, 72        # H
sw x5, 0(x10)

addi x5, x0, 101       # e
sw x5, 0(x10)

addi x5, x0, 108       # l
sw x5, 0(x10)

addi x5, x0, 108       # l
sw x5, 0(x10)

addi x5, x0, 111       # o
sw x5, 0(x10)

addi x5, x0, 44        # ,
sw x5, 0(x10)

addi x5, x0, 32        # space
sw x5, 0(x10)

addi x5, x0, 87        # W
sw x5, 0(x10)

addi x5, x0, 111       # o
sw x5, 0(x10)

addi x5, x0, 114       # r
sw x5, 0(x10)

addi x5, x0, 108       # l
sw x5, 0(x10)

addi x5, x0, 100       # d
sw x5, 0(x10)

addi x5, x0, 33        # !
sw x5, 0(x10)

addi x5, x0, 10        # newline
sw x5, 0(x10)

end: