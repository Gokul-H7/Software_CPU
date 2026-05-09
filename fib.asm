# iterative fibonacci
# computes 10 iterations
# final x8 should contain the latest fib value

addi x5, x0, 0       # fib prev = 0
addi x6, x0, 1       # fib curr = 1
addi x7, x0, 10      # loop counter
addi x10, x0, 1024   # memory output pointer

loop:
    add x8, x5, x6   # next = prev + curr
    sw x8, 0(x10)    # store next fib value

    add x5, x6, x0   # prev = curr
    add x6, x8, x0   # curr = next

    addi x7, x7, -1
    beq x7, x0, end
    jal x1, loop

end: