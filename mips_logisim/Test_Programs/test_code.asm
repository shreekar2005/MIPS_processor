.data
    message: .asciiz "The sum is: " 
    num1: .word 5                 
    num2: .word 3                 

.text
main:

    lw $t0, num1          
    lw $t1, num2          
    add $t2, $t0, $t1     
    addi $t2, $t2, 10

    li $v0, 4             
    la $a0, message       
    syscall               

    li $v0, 1             
    move $a0, $t2       
    syscall               

    li $v0, 10            
    syscall  

// following code is just for testing some commands
label:
    beq $t2, $t3, label // just for showing purpose      
    lw $s0, 16($sp) // just for showing purpose
    lui $at, 0x1001 // just for showing purpose mostly used for lw pseudo instruction 
    ori $s0, $s0, 0x00FF // just for showing purpose mostly used for lw pseudo instruction 
    j label
    jal label