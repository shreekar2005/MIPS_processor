.data
var1:   .word 15    # var1 = 15
var2:   .word 10    # var2 = 10
result: .word 0

.text

main:
    lw      $t0, var1       # $t0 = 15
    lw      $t1, var2       # $t1 = 10

    move    $s0, $t0        # $s0 = 15
    subi    $s1, $t1, 5     # $s1 = 5

    addi    $s0, $s0, 10    # $s0 = 25

    add     $s2, $s0, $s1   # $s2 = 30
    sub     $s3, $s0, $s1   # $s3 = 20
    and     $s4, $s0, $s1   # $s4 = 1
    or      $s5, $s0, $s1   # $s5 = 29
    slt     $s6, $s1, $s0   # $s6 = 1

    sw      $s2, result     # result = 30

    beq     $s1, $t1, skip_jump # Branch not taken (5 != 10)

    j       end_program

skip_jump:
    addi    $s7, $zero, 99  # $s7 = 99

end_program:
    # End of program
