# run with -v flag to show timer countdown execution cycles on cpu output
addi x10, x0, 1024     # MMIO_PRINT address
addi x5, x0, 5         # timer counter starts at 5

loop:
    sw x5, 0(x10)      # Store cycle: output current timer value

    addi x5, x5, -1    # Compute cycle: counter--
    beq x5, x0, end    # Stop when counter reaches 0

    jal x1, loop       # Fetch continues from loop label

end: