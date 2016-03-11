/*
 * Factorial
 */

.global fact_iterative
.func fact_iterative

fact_iterative:
	  mov r3,r0	
	  mov r0,#1
	  mov r4,#1
Loop:	 
	  cmp r4,r3
	  bgt ExitLoop
 	  mul r0,r4,r0
	  add r4,r4,#1
	  b Loop
ExitLoop: 
	  bx lr
