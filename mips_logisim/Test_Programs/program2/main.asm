# main.asm
# A test program for a MIPS processor with a limited instruction set.
# This program implements the following logic:
# if (A < B) {
#   Result = A + B
# } else {
#   Result = A - B
# }
# It only uses add, sub, slt, lw, sw, beq, and j.

.data
    varA:   .word 40      # An initial value for variable A (at address 0x0)
    varB:   .word 30      # An initial value for variable B (at address 0x4)
    result: .word 0       # A memory location to store the result (at address 0x8)

.text
main:
    # 1. Load initial values from data memory into registers.
    # We use $zero as the base address since 'la' is not supported.
    lw  $s1, 0($zero)   # Load varA (from address 0) into register $s1.
    lw  $s2, 4($zero)   # Load varB (from address 4) into register $s2.

    # 2. Compare the two values to implement the 'if (A < B)' condition.
    slt $t0, $s1, $s2   # Set $t0 to 1 if $s1 < $s2. Otherwise, set to 0.

    # 3. Branch to the ELSE block if the condition (A < B) is false.
    # beq branches if $t0 is equal to $zero (meaning A was NOT less than B).
    beq $t0, $zero, ELSE

# IF block: This code runs if A < B was true.
    add $t1, $s1, $s2   # Calculate Result = A + B, and store it in $t1.
    j   END_IF          # Jump past the ELSE block to the end.

# ELSE block: This code runs if A < B was false (i.e., A >= B).
ELSE:
    sub $t1, $s1, $s2   # Calculate Result = A - B, and store it in $t1.

# End of the conditional logic.
END_IF:
    # 4. Store the final result from register $t1 back into data memory.
    sw  $t1, 8($zero)   # Store the value from $t1 into the 'result' memory location (address 8).

# 5. Halt the processor.
# A simple infinite loop is the standard way to stop execution in a simulation.
HALT:
    j   HALT            # Jump back to this same instruction indefinitely.

