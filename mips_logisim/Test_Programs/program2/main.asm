# if (A < B) {
#   Result = A + B
# } else {
#   Result = A - B
# }

.data
    result: .word 0       # result (at address 0x0)

.text
main:
    addi $s1, $zero, 50 # A
    addi $s2, $zero, 40 # B

    
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
    
    sw $t1, result   # Store 'result'

HALT:
    j HALT  
