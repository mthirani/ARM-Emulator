/*
 * Factorial - Recursive Approach
 */

.global fact_recursive
.func fact_recursive

fact_recursive:
	str lr,[sp,#-4]!
	str r0,[sp,#-4]!
	cmp r0,#0
	bne is_nonzero
	mov r0,#1
	b end
is_nonzero:
	sub r0,r0,#1
	bl fact_recursive
	ldr r1,[sp]
	mul r0,r1,r0
end:
	add sp,sp,#4
	ldr lr,[sp]
	add sp,sp,#4
	bx lr
