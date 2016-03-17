# ARM-Emulator
C based project which emulates the ARM assembly instructions. Below are the high level details of the project:

1.  ARM assembly instructions such as LDR, STR, MOV, ADD, SUB, MUL, CMP, BX, B, BL were emulated
2.  Provides the representation of the register state (r0-r15, CPSR)
3.  Provides the representation of memory (stack)
4.  Dynamic analysis of the function execution:
      Number of instructions executed
      Register usage counts (for each register)
          Register reads
          Register writes
      Instruction counts
          Computation
          Memory
          Branches
5.  Performance measurements comparing native execution time versus emulated execution time. Use the Linux times() library function.
6.  Analysis of all the data has been represented in tabular format
7.  ARM assembly functions such as Insertion Sort, Factorial of a number (Iterative and Recursive way), Sum of Elements in Array (Recursively) were emulated successfully through this emulator; Examples of such functions were also provided
