# Allocate 100 bites of space on the stack for input buffer
ADDI $sp, $sp, -100

# Read from STDIN
ADDI $v0, $zero, 8   # System call number for read
ADDI $a0, $sp, 0     # Address to store input
ADDI $a1, $zero, 100 # Maximum length to read
syscall

# Print the input string back to stdout
ADDI $v0, $zero, 4   # System call number for print
ADDI $a0, $sp, 0     # Address of input string
syscall

# Exit program
ADDI $v0, $zero, 10  # System call number for exit
syscall