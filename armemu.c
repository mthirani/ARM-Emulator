#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* ARM Assembly Functions to emulate */
int fact_recursive(int);
int fact_iterative(int);
int isort(int *);
int rsum(int *);

/* ARM Machine State */
#define ARM_STACK_SIZE 16384

struct arm_state {
    unsigned regs[16];
    unsigned cpsr;
    unsigned char stack[ARM_STACK_SIZE];
};

/* Initialize the arm_state struct */
void arm_state_init(struct arm_state *state)
{
    int i;
    for (i = 0; i < 16; i++) {
	state->regs[i] = 0;
    }
    state->cpsr = 0;
    for (i = 0; i < ARM_STACK_SIZE; i++) {
	state->stack[i] = 0;
    }
}

/* Print the arm_state struct */
void arm_state_print(struct arm_state *state)
{
    int i;
    unsigned *ptr;
    for (i = 0; i < 16; i++) {
	printf("regs[%d] = %X\n", i, state->regs[i]);
    }
    ptr = (unsigned *)state->regs[13];
    printf("Stack Value: %X\n", *ptr);
    printf("cpsr = %X\n", state->cpsr);
}

/* Determine if the iw corresponds to Data Processing */
bool is_dp_iw(unsigned iw)
{
    iw = iw >> 26;
    iw = iw & 0b00;
    return (iw == 0b00);
}

/* Execute a data processing instruction word */
void execute_dp_iw(struct arm_state *state, unsigned iw)
{
    unsigned cond;
    unsigned opcode;
    unsigned rn;
    unsigned rm;
    unsigned rd;
    unsigned rotate;
    unsigned imm;
    unsigned immBit;
    unsigned setBit;
    unsigned shiftCode;
    unsigned shiftType;
    unsigned shiftAmount;
    unsigned shiftRegister;
    unsigned valueRmReg;
    int compare;

    cond = iw >> 28;
    opcode = (iw >> 21) & 0b1111;
    rn = (iw >> 16) & 0b1111;
    rd = (iw >> 12) & 0b1111;
    rm = iw & 0b1111;
    rotate = (iw >> 8) & 0b1111;
    imm = iw & 0b11111111;
    immBit = (iw >> 25) & 0b1;
    setBit = (iw >> 20) & 0b1;
    
    if(immBit == 0) {
	valueRmReg = state->regs[rm];
	shiftCode = (iw >> 4) & 0b1;
	shiftType = (iw >> 5) & 0b11;
	if(shiftCode == 1) {
		shiftAmount = (iw >> 8) & 0b1111;
		shiftAmount = state->regs[shiftAmount];
	}
	else {
		shiftAmount = (iw >> 7) & 0b11111;	
	}
	if((shiftType == 0b00) && (shiftAmount != 0)) {
       		 valueRmReg = valueRmReg * shiftAmount * 2;
        } else if((shiftType == 0b10) && (shiftAmount != 0)) {
                 valueRmReg = valueRmReg / (shiftAmount * 2);
        }
    }
    if (opcode == 0b1101) {	//MOV Instruction
	if(immBit == 1)
		state->regs[rd] = imm;
	else
		state->regs[rd] = valueRmReg;
    } else if (opcode == 0b0100) {	//ADD Instruction
	if(immBit == 1)
		state->regs[rd] = state->regs[rn] + imm;
	else
		state->regs[rd] = state->regs[rn] + valueRmReg;
    } else if (opcode == 0b0010) {      //SUB Instruction
        if(immBit == 1)
                state->regs[rd] = state->regs[rn] - imm;
        else
                state->regs[rd] = state->regs[rn] - valueRmReg;
    } else if (opcode == 0b1010) {	//CMP Instruction
	if(immBit == 1)
                compare = state->regs[rn] - imm;
        else
		compare = state->regs[rn] - valueRmReg;
	if(compare == 0)			//EQ
		state->cpsr = 0b0000;
	else if (compare > 0)			//GT
		state->cpsr = 0b1100;
	else					//LT
		state->cpsr = 0b1011;
    }
    state->regs[15] = state->regs[15] + 4;
}

/* Determine if the iw corresponds to Multiply Instruction */
bool is_mul_iw(unsigned iw)
{   
    unsigned iw1, iw2;
    iw1 = iw >> 22;
    iw1 = iw1 & 0b111111;
    iw2 = iw >> 4;
    iw2 = iw2 & 0b1111;

    return ((iw1 == 0b000000) &&(iw2 == 0b1001));
}

/* Execute a data processing instruction word */
void execute_mul_iw(struct arm_state *state, unsigned iw)
{
    unsigned cond;
    unsigned rn;
    unsigned rm;
    unsigned rd;
    unsigned rs;
    unsigned setBit;
    int mul;

    cond = iw >> 28;
    rn = (iw >> 12) & 0b1111;
    rd = (iw >> 16) & 0b1111;
    rs = (iw >> 8) & 0b1111;
    rm = iw & 0b1111;
    setBit = (iw >> 20) & 0b1;

    mul = state->regs[rm] * state->regs[rs];
    state->regs[rd] = mul;
    if(setBit == 0b1){
	if(mul < 0){
		state->cpsr = 0b0100;
	}
    }

    state->regs[15] = state->regs[15] + 4;
}

/* Determine if iw is a branch and exchange instruction */
bool is_bx_iw(unsigned iw)
{
    iw = iw >> 4;
    iw = iw & 0x00FFFFFF;
    return (iw == 0b000100101111111111110001);
}

/* Execute a bx instruction */
void execute_bx_iw(struct arm_state *state, unsigned iw)
{
    iw = iw & 0b1111;
    state->regs[15] = state->regs[iw];
}

/* Determine if iw is a single data transfer instruction */
bool is_dt_iw(unsigned iw)
{
    iw = iw >> 26;
    iw = iw & 0b11;
    return (iw == 0b01);
}

/* Execute a bx instruction */
void execute_dt_iw(struct arm_state *state, unsigned iw)
{
    unsigned loadOrStore;
    unsigned rd;
    unsigned rn;
    unsigned postOrPre;
    unsigned rm;
    unsigned upDown;
    unsigned imm;
    unsigned immBit;
    unsigned shiftCode;
    unsigned shiftAmount;
    unsigned shiftType;
    unsigned writeBack;
    unsigned *ptr;
    int valueOffset;

    loadOrStore = (iw >> 20) & 0b1;
    rd = (iw >> 12) & 0b1111;
    rn = (iw >> 16) & 0b1111;
    immBit = (iw >> 25) & 0b1;
    postOrPre = (iw >> 24) & 0b1;
    upDown = (iw >> 23) & 0b1;
    writeBack = (iw >> 21) & 0b1;
    rm = iw & 0b1111;
    imm = iw & 0b111111111111; 
    valueOffset = imm;

    if(immBit == 1) {
        valueOffset = state->regs[rm];
        shiftCode = (iw >> 4) & 0b1;
        shiftType = (iw >> 5) & 0b11;
        if(shiftCode == 1) {
                shiftAmount = (iw >> 8) & 0b1111;
                shiftAmount = state->regs[shiftAmount];
        }
        else {
                shiftAmount = (iw >> 7) & 0b11111;
        }
        if((shiftType == 0b00) && (shiftAmount != 0)) {
                 valueOffset = valueOffset * shiftAmount * 2;
        } else if((shiftType == 0b10) && (shiftAmount != 0)) {
                 valueOffset = valueOffset / (shiftAmount * 2);
        }
    }
    if(upDown == 0) 
	valueOffset = -valueOffset;
    if(postOrPre == 1)
	state->regs[rn] = state->regs[rn] + valueOffset;
    ptr = (unsigned *) state->regs[rn];
    if(loadOrStore == 1)
	state->regs[rd] = *ptr;
    else
	*ptr = state->regs[rd];       
    if(postOrPre == 0)
	state->regs[rn] = state->regs[rn] + valueOffset;
    if(writeBack == 0)
	 state->regs[rn] = state->regs[rn] - valueOffset;
    state->regs[15] = state->regs[15] + 4;
}

/* Determine if iw is a branch and link instruction */
bool is_b_iw(unsigned iw)
{
    iw = iw >> 25;
    iw = iw & 0b101;
    return (iw == 0b101);
}

/* Execute a branch and link instruction word */
void execute_b_iw(struct arm_state *state, unsigned iw)
{
    unsigned cond;
    unsigned link;
    unsigned offset;
    unsigned offsetCheck;
    int newOffset;

    cond = iw >> 28;
    link = (iw >> 24) & 0b1;
    offset = iw & 0xFFFFFF;
    offsetCheck = (offset >> 23) & 0b1;
    
    if(offsetCheck == 1){
	offset = offset + 0xFF000000;
        offset = ~offset;
        offset = (offset + 1) << 2;
	newOffset = -offset;
    }
    else{
	offset = offset << 2;
	newOffset = offset;
    }
    if(link == 0b1){
        state->regs[14] = state->regs[15] + 4;
    	state->regs[15] = state->regs[15] + 8 + newOffset;
    } else if(cond == 0b0001){
	if(state->cpsr != 0){
		state->regs[15] = state->regs[15] + 8 + newOffset;
	}
	else{
		state->regs[15] = state->regs[15] + 4;
	}
    } else if (cond == state->cpsr){
	state->regs[15] = state->regs[15] + 8 + newOffset;
    } else if (cond != 0b1110){
	state->regs[15] = state->regs[15] + 4;
    } else{
	state->regs[15] = state->regs[15] + 8 + newOffset;
    }
}

/* Determine the correct iw instruction and execute it */
void emu_instruction(struct arm_state *state)
{
    unsigned iw;

    iw = *((unsigned *) state->regs[15]);
    
    if(is_b_iw(iw)) {
	execute_b_iw(state, iw);
    } else if (is_dt_iw(iw)) {
        execute_dt_iw(state, iw);
    } else if (is_bx_iw(iw)) {
	execute_bx_iw(state, iw);
    } else if(is_mul_iw(iw)) {
	execute_mul_iw(state, iw);
    } else if (is_dp_iw(iw)) {
	execute_dp_iw(state, iw);
    } else {
	printf("emu_instruction: unrecognized instruction\n");
	exit(-1);
    }
}

/* Function call starts here */
unsigned emu(struct arm_state *state, void *func, int argc, unsigned *args)
{
    int i;
    
    arm_state_init(state);

    if (argc < 0 || argc > 4) {
	printf("Too many args passed to emu.\n");
	exit(-1);
    }

    /* Assign args to registers */
    for (i = 0; i < argc; i++) {
	state->regs[i] = args[i];
    }

    /* Assign pc */
    state->regs[15] = (unsigned) func;

    /* Assign lr */
    state->regs[14] = 0;

    /* Assign sp */
    state->regs[13] = (unsigned) &state->stack[ARM_STACK_SIZE];
    
    /* Emulate ARM function */
    while(state->regs[15] != 0) {
	emu_instruction(state);
    }

    return state->regs[0];
}


int main(int argc, char **argv)
{
    unsigned rv;
    int factNumber;
    int *insSortArray;
    int lengthArray;
    int i;
    unsigned *insSort[2];
    struct arm_state state;
    int *rsumArray;
    int index = 0;
    int sum = 0;
    unsigned recurSum[4];

    /* Recursive Sum: Recursively Compute the numbers of an array */
    while(true){
        printf("Enter the length of Array: ");
        scanf("%d", &lengthArray);
        if(lengthArray < 0)
                printf("Length cannot be negative. Please input a positive number. \n");
        else
                break;
    }
    rsumArray = (int *)malloc(lengthArray*sizeof(int));
    if(lengthArray <= 10){
        printf("Enter the numbers in Array.\n");
        for(i=0; i<lengthArray; i++) {
                printf("%d: ", (i+1));
                scanf("%d", (rsumArray+i));
        }
    }
    else{
        for(i=0; i<lengthArray; i++) {
                rsumArray[i] = (rand() % 50) + 1;
        }
    }
    recurSum[0] = sum;
    recurSum[1] = lengthArray;
    recurSum[2] = index;
    recurSum[3] = (unsigned )&rsumArray[0];
    rv = emu(&state, (void *) rsum, 4, (unsigned *) recurSum);
    printf("sum = %d\n", rv);   
    
     /* Factorial Recursive: Input Number and Passing it to emu function for executing ARM instructions */
    while(true){
        printf("Enter the number to get the factorial: ");
        scanf("%d", &factNumber);
        if(factNumber < 0)
                printf("Number cannot be negative. Please input a positive number. \n");
        else
                break;
    }
    rv = emu(&state, (void *) fact_recursive, 1, &factNumber);
    printf("fact_recursive(%d) = %d\n", factNumber, rv);

    /* Factorial Iterative: Input Number and Passing it to emu function for executing ARM instructions */
    while(true){
    	printf("Enter the number to get the factorial: ");
    	scanf("%d", &factNumber);
	if(factNumber < 0)
		printf("Number cannot be negative. Please input a positive number. \n");
	else
		break;
    }
    rv = emu(&state, (void *) fact_iterative, 1, &factNumber);
    printf("fact_iterative(%d) = %d\n", factNumber, rv);   
    
    /* InsertionSort: Input Numbers and Passing it to emu function for executing ARM instructions */ 
    while(true){
        printf("Enter the length of Array: ");
        scanf("%d", &lengthArray);
        if(lengthArray < 0)
                printf("Length cannot be negative. Please input a positive number. \n");
        else
                break;
    }
    insSortArray = (int *)malloc(lengthArray*sizeof(int));
    if(lengthArray <= 10){
    	printf("Enter the numbers in Array.\n");
    	for(i=0; i<lengthArray; i++) {
		printf("%d: ", (i+1));
		scanf("%d", (insSortArray+i));
    	}
    }
    else{
	for(i=0; i<lengthArray; i++) {
		insSortArray[i] = (rand() % 100) + 1;
    	}
    }
    insSort[0] = &lengthArray;
    insSort[1] = &insSortArray[0];
    rv = emu(&state, (void *) isort, 2, (unsigned *) insSort);
    printf("Below is the sorted Array.\n");
    for(i=0; i<lengthArray; i++) {
        printf("%d: %d\n", (i+1), insSortArray[i]);
    }
  
    return 0;
}

