
# if (A < B) {
#   Result = A + B
# } else {
#   Result = A - B
# }
# using add, sub, slt, lw, sw, beq, and j.

.data
    varA:   .word 50      # A (at address 0x0)
    varB:   .word 30      # B (at address 0x4)
    result: .word 0       # result (at address 0x8)

.text
main:
    lw $s1, 0($zero)  
    lw $s2, 4($zero)   

    
    slt $t0, $s1, $s2   # Set $t0 to 1 if $s1 < $s2. Otherwise, set to 0.

    # Branch to the ELSE block if the condition (A < B) is false.
    beq $t0, $zero, ELSE

    # if A < B was true.
    add $t1, $s1, $s2   
    j   END_IF

# if A >= B
ELSE:
    sub $t1, $s1, $s2

END_IF:
    
    sw $t1, 8($zero)   # Store 'result'

HALT:
    j HALT  
