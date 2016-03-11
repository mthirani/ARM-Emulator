/*
 * Insertion Sort
 */

.global isort
.func isort

isort:
	mov r4,#1
	mov r8,r1	
	mov r2,r0	
	ldr r2,[r2]
OuterLoop:
	cmp r4,r2
	beq ExitOuterLoop
	mov r6,r4
InnerLoop:
	cmp r6,#0
	beq ExitInnerLoop
	mov r1,r6,LSL #2
	ldr r5,[r8,+r1]
	sub r1,r1,#4
	ldr r7,[r8,+r1]
	cmp r5,r7
	bgt ExitInnerLoop
	mov r1,r6,LSL #2
	str r7,[r8,+r1]
	sub r1,r1,#4
	str r5,[r8,+r1]
	sub r6,r6,#1
	b InnerLoop
ExitInnerLoop:
	add r4,r4,#1
	b OuterLoop
ExitOuterLoop:
	bx lr
