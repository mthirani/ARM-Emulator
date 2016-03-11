/*
 * Recursion - Sum
 */
.global rsum
.func rsum

rsum:
	str lr,[sp,#-4]!
	str r2,[sp,#-4]!
	cmp r2,r1
	beq EndFunction
	add r2,r2,#1
	bl rsum
	ldr r4,[sp]
	mov r5,r3
	ldr r6,[r5,+r4,LSL #2]
	add r0,r0,r6
EndFunction:
	add sp,sp,#4
	ldr lr,[sp]
	add sp,sp,#4
	bx lr
