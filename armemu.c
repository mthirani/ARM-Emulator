#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>
#include <time.h>

/* ARM Assembly Functions to emulate */
int fact_recursive(int);
int fact_iterative(int);
int isort(int, int);
int rsum(int, int, int, int);

/* ARM Machine State */
#define ARM_STACK_SIZE 16384

struct arm_state {
    unsigned regs[16];
    unsigned cpsr;
    unsigned char stack[ARM_STACK_SIZE];
    unsigned regReads[16];
    unsigned regWrites[16];
    unsigned cpsrReads;
    unsigned cpsrWrites;
    unsigned memoryInstr;
    unsigned computeInstr;
    unsigned branchInstr;
};

/* Initialize the arm_state struct */
void arm_state_init(struct arm_state *state)
{
    int i;
    for (i = 0; i < 16; i++) {
	state->regs[i] = 0;
	state->regReads[i] = 0;
	state->regWrites[i] = 0;
    }
    state->cpsr = 0;
    state->cpsrReads = 0;
    state->cpsrWrites = 0;
    state->memoryInstr = 0;
    state->computeInstr = 0;
    state->branchInstr = 0;
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
    valueRmReg = imm;

    if(immBit == 0) {			//Operand2 is a register
	valueRmReg = state->regs[rm];
	state->regReads[rm] = state->regReads[rm] + 1;
	shiftCode = (iw >> 4) & 0b1;
	shiftType = (iw >> 5) & 0b11;
	if(shiftCode == 1) {
		shiftAmount = (iw >> 8) & 0b1111;
		shiftAmount = state->regs[shiftAmount];
		state->regReads[shiftAmount] =  state->regReads[shiftAmount] + 1;
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
	state->regs[rd] = valueRmReg;
	state->regWrites[rd] = state->regWrites[rd] + 1;
    } else if (opcode == 0b0100) {	//ADD Instruction
	state->regs[rd] = state->regs[rn] + valueRmReg;
	state->regReads[rn] = state->regReads[rn] + 1;
	state->regWrites[rd] = state->regWrites[rd] + 1;
    } else if (opcode == 0b0010) {      //SUB Instruction
        state->regs[rd] = state->regs[rn] - valueRmReg;
	state->regReads[rn] = state->regReads[rn] + 1;
	state->regWrites[rd] = state->regWrites[rd] + 1;
    } else if (opcode == 0b1010) {	//CMP Instruction
	compare = state->regs[rn] - valueRmReg;
        state->regReads[rn] = state->regReads[rn] + 1;	
	if(compare == 0)			//EQ
		state->cpsr = 0b0000;
	else if (compare > 0)			//GT
		state->cpsr = 0b1100;
	else					//LT
		state->cpsr = 0b1011;
	state->cpsrWrites = state->cpsrWrites + 1;
    }

    state->regs[15] = state->regs[15] + 4;
    state->regReads[15] = state->regReads[15] + 1;
    state->regWrites[15] = state->regWrites[15] + 1;
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
		state->cpsrWrites = state->cpsrWrites + 1;
	}
    }
    state->regReads[rm] = state->regReads[rm] + 1;
    state->regReads[rs] = state->regReads[rs] + 1;
    state->regWrites[rd] = state->regWrites[rd] + 1;

    state->regs[15] = state->regs[15] + 4;
    state->regReads[15] = state->regReads[15] + 1;
    state->regWrites[15] = state->regWrites[15] + 1;
}

/* Determine if iw is a branch and exchange instruction */
bool is_bx_iw(unsigned iw)
{
    iw = iw >> 4;
    iw = iw & 0x00FFFFFF;
    return (iw == 0b000100101111111111110001);
}

/* Execute a branch and exchange instruction word */
void execute_bx_iw(struct arm_state *state, unsigned iw)
{
    iw = iw & 0b1111;

    state->regs[15] = state->regs[iw];
    state->regReads[iw] = state->regReads[iw] + 1;
    state->regWrites[15] = state->regWrites[15] + 1;
}

/* Determine if iw is a single data transfer instruction */
bool is_dt_iw(unsigned iw)
{
    iw = iw >> 26;
    iw = iw & 0b11;
    return (iw == 0b01);
}

/* Execute a Load or Store instruction word */
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

    if(immBit == 1) {			//Offset is a register
        valueOffset = state->regs[rm];
	state->regReads[rm] = state->regReads[rm] + 1;
        shiftCode = (iw >> 4) & 0b1;
        shiftType = (iw >> 5) & 0b11;
        if(shiftCode == 1) {
                shiftAmount = (iw >> 8) & 0b1111;
                shiftAmount = state->regs[shiftAmount];
		state->regReads[shiftAmount] = state->regReads[shiftAmount] + 1;
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
    state->regReads[rn] = state->regReads[rn] + 1;
    if(loadOrStore == 1){		//LDR Instruction
	state->regs[rd] = *ptr;
	state->regWrites[rd] = state->regWrites[rd] + 1;
    }
    else{				//STR Instruction
	*ptr = state->regs[rd];
	state->regReads[rd] = state->regReads[rd] + 1;
    }       
    if(postOrPre == 0)
	state->regs[rn] = state->regs[rn] + valueOffset;
    if(writeBack == 0)
	 state->regs[rn] = state->regs[rn] - valueOffset;
    else{
	state->regWrites[rn] = state->regWrites[rn] + 1;
    }

    state->regs[15] = state->regs[15] + 4;
    state->regReads[15] = state->regReads[15] + 1;
    state->regWrites[15] = state->regWrites[15] + 1;
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
    if(link == 0b1){						//BL Instruction
        state->regs[14] = state->regs[15] + 4;
    	state->regs[15] = state->regs[15] + 8 + newOffset;
	state->regReads[15] = state->regReads[15] + 1;
	state->regWrites[14] = state->regWrites[14] + 1;
    } else if(cond == 0b0001){					//BNE Instruction
	if(state->cpsr != 0)
		state->regs[15] = state->regs[15] + 8 + newOffset;
	else
		state->regs[15] = state->regs[15] + 4;
	state->cpsrReads = state->cpsrReads + 1;
    } else if (cond == state->cpsr){				//B<Cond> Instruction
	state->regs[15] = state->regs[15] + 8 + newOffset;
	state->cpsrReads = state->cpsrReads + 1;
    } else if (cond != 0b1110){					//B<Cond> Instruction
	state->regs[15] = state->regs[15] + 4;
	state->cpsrReads = state->cpsrReads + 1;
    } else{							//B Instruction
	state->regs[15] = state->regs[15] + 8 + newOffset;
    }
    
    state->regReads[15] = state->regReads[15] + 1;
    state->regWrites[15] = state->regWrites[15] + 1;
}

/* Determine the correct iw instruction and execute it */
void emu_instruction(struct arm_state *state)
{
    unsigned iw;

    iw = *((unsigned *) state->regs[15]);
    
    if(is_b_iw(iw)) {
	execute_b_iw(state, iw);
	state->branchInstr = state->branchInstr + 1;
    } else if (is_dt_iw(iw)) {
        execute_dt_iw(state, iw);
	state->memoryInstr = state->memoryInstr + 1;
    } else if (is_bx_iw(iw)) {
	execute_bx_iw(state, iw);
	state->branchInstr = state->branchInstr + 1;
    } else if(is_mul_iw(iw)) {
	execute_mul_iw(state, iw);
	state->computeInstr = state->computeInstr + 1;
    } else if (is_dp_iw(iw)) {
	execute_dp_iw(state, iw);
	state->computeInstr = state->computeInstr + 1;
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

/* Counting the Total Register Usage(Both R/W)  */
int registersUsage(struct arm_state *state)
{
    int i;
    int count = 0;
    for(i=0; i<16; i++) {
	count = count + state->regReads[i] + state->regWrites[i];
    }
    count = count + state->cpsrReads + state->cpsrWrites;

    return count;
}

/* Register Read Analysis */
void regReadAnalysis(struct arm_state *state, int count, char *str)
{
    int i;
    float perReads;
    printf("[Register Read Analysis @ %s] ::: \n", str);
    for(i=0; i<16; i++) {
	perReads = ((float) state->regReads[i] / count) * 100;
	printf("r%d has been read %d times (%.2f%) of total register usage counts(%d)\n", i, state->regReads[i], perReads, count);
   }
   perReads = ((float) state->cpsrReads / count) * 100;
   printf("cpsr has been read %d times (%.2f%) of total register usage counts(%d)\n\n", state->cpsrReads, perReads, count);
}

/* Register Write Analysis */
void regWriteAnalysis(struct arm_state *state, int count, char *str)
{
    int i;
    float perWrites;
    printf("[Register Write Analysis @ %s] ::: \n", str);
    for(i=0; i<16; i++) {
        perWrites = ((float) state->regWrites[i] / count) * 100;
        printf("r%d has been written %d times (%.2f%) of total register usage counts(%d)\n", i, state->regWrites[i], perWrites, count);
   }
   perWrites = ((float) state->cpsrWrites / count) * 100;
   printf("cpsr has been written %d times (%.2f%) of total register usage counts(%d)\n\n", state->cpsrWrites, perWrites, count);
}

/* Instrucctions Analysis */
void instructionAnalysis(struct arm_state *state, char *str)
{
    int totalInstructions = state->memoryInstr + state->computeInstr + state->branchInstr;
    float perInstructions;
    printf("[Instructions  Analysis @ %s] ::: \n", str);
    printf("Number of Instructions executed := %d\n", totalInstructions);
    perInstructions = ((float) state->memoryInstr / totalInstructions) * 100;
    printf("Memory Instruction(s) has been called %d times (%.2f%) of total Instruction counts(%d)\n", state->memoryInstr, perInstructions, totalInstructions);
    perInstructions = ((float) state->computeInstr / totalInstructions) * 100;
    printf("Compute Instruction(s) has been called %d times (%.2f%) of total Instruction counts(%d)\n", state->computeInstr, perInstructions, totalInstructions);
    perInstructions = ((float) state->branchInstr / totalInstructions) * 100;
    printf("Branch Instruction(s) has been called %d times (%.2f%) of total Instruction counts(%d)\n\n", state->branchInstr, perInstructions, totalInstructions);
}

/* Main */
int main(int argc, char **argv)
{
    clock_t ct1, ct2;
    unsigned rv;
    int factNumber;
    int *insSortArray;
    int lengthArray;
    int i;
    unsigned insSort[2];
    struct arm_state state;
    int *rsumArray;
    int index = 0;
    int sum = 0;
    unsigned recurSum[4];
    int totalRegCounts;
    
    /* Recursive Sum: Recursively Compute the numbers of an array */
    printf("\n/**************** Result and Dynamic Analysis for \"Recursive Sum\" ***************/\n\n");
    printf("[Input/ Output @ %s] :::\n", "Recursive Sum");
    printf("<-------------- Input -------------->\n");
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
	printf("As the length of array is > 10 so Input array generated dynamically for you.\n");
    }
    recurSum[0] = sum;
    recurSum[1] = lengthArray;
    recurSum[2] = index;
    recurSum[3] = (unsigned) &rsumArray[0];
    ct1 = clock();
    rv = emu(&state, (void *) rsum, 4, (unsigned *) recurSum);
    ct2 = clock();
    printf("<-------------- Output -------------->\n");
    printf("sum = %d\n\n", rv);
    totalRegCounts = registersUsage(&state);
    regReadAnalysis(&state, totalRegCounts, "Recursive Sum");
    regWriteAnalysis(&state, totalRegCounts, "Recursive Sum");
    instructionAnalysis(&state, "Recursive Sum");
    printf("[Performance Analysis @ %s] :::\n", "Recursive Sum");
    printf("<-------------- ARM Emulator -------------->\n");
    printf ("CLOCK_TICKS_PER_SEC = %ld\n", CLOCKS_PER_SEC);
    printf ("CPU TimeUtilization = %f seconds\n\n", ((double)(ct2 - ct1))/ CLOCKS_PER_SEC);
    ct1 = clock();
    rv = rsum(recurSum[0], recurSum[1], recurSum[2], recurSum[3]);
    ct2 = clock();
    printf("<---------- Native Assembly Code ---------->\n");
    printf ("CPU TimeUtilization = %f seconds\n\n", ((double)(ct2 - ct1))/ CLOCKS_PER_SEC);

    /* Factorial Recursive: Input Number and Passing it to emu function for executing ARM instructions */
    printf("/**************** Result and Dynamic Analysis for \"Factorial Recursive\" ***************/\n\n");
    printf("[Input/ Output @ %s] :::\n", "Factorial Recursive");
    printf("<---------------- Input ---------------->\n");
    while(true){
        printf("Enter the number to get the factorial: ");
        scanf("%d", &factNumber);
        if(factNumber < 0)
                printf("Number cannot be negative. Please input a positive number. \n");
        else
                break;
    }
    ct1 = clock();
    rv = emu(&state, (void *) fact_recursive, 1, &factNumber);
    ct2 = clock();
    printf("<---------------- Output ---------------->\n");
    printf("fact_recursive(%d) = %d\n\n", factNumber, rv);
    totalRegCounts = registersUsage(&state);
    regReadAnalysis(&state, totalRegCounts, "Factorial Recursive Way");
    regWriteAnalysis(&state, totalRegCounts, "Factorial Recursive Way");
    instructionAnalysis(&state, "Factorial Recursive Way");
    printf("[Performance Analysis @ %s] :::\n", "Factorial Recursive Way");
    printf("<------------------ ARM Emulator ------------------>\n");
    printf ("CLOCK_TICKS_PER_SEC = %ld\n", CLOCKS_PER_SEC);
    printf ("CPU TimeUtilization = %f seconds\n\n", ((double)(ct2 - ct1))/ CLOCKS_PER_SEC);
    ct1 = clock();
    rv = fact_recursive(factNumber);
    ct2 = clock();
    printf("<-------------- Native Assembly Code -------------->\n");
    printf ("CPU TimeUtilization = %f seconds\n\n", ((double)(ct2 - ct1))/ CLOCKS_PER_SEC);
    
    /* Factorial Iterative: Input Number and Passing it to emu function for executing ARM instructions */
    printf("/**************** Result and Dynamic Analysis for \"Factorial Iterative\" ***************/\n\n");
    printf("[Input/ Output @ %s] :::\n", "Factorial Iterative");
    printf("<---------------- Input ---------------->\n");
    while(true){
    	printf("Enter the number to get the factorial: ");
    	scanf("%d", &factNumber);
	if(factNumber < 0)
		printf("Number cannot be negative. Please input a positive number. \n");
	else
		break;
    }
    ct1 = clock();
    rv = emu(&state, (void *) fact_iterative, 1, &factNumber);
    ct2 = clock();
    printf("<---------------- Output ---------------->\n");
    printf("fact_iterative(%d) = %d\n\n", factNumber, rv);
    totalRegCounts = registersUsage(&state);
    regReadAnalysis(&state, totalRegCounts, "Factorial Iterative Way");
    regWriteAnalysis(&state, totalRegCounts, "Factorial Iterative Way");
    instructionAnalysis(&state, "Factorial Iterative Way");   
    printf("[Performance Analysis @ %s] :::\n", "Factorial Iterative Way");
    printf("<------------------ ARM Emulator ------------------>\n");
    printf ("CLOCK_TICKS_PER_SEC = %ld\n", CLOCKS_PER_SEC);
    printf ("CPU TimeUtilization = %f seconds\n\n", ((double)(ct2 - ct1))/ CLOCKS_PER_SEC);
    ct1 = clock();
    rv = fact_iterative(factNumber);
    ct2 = clock();
    printf("<-------------- Native Assembly Code -------------->\n");
    printf ("CPU TimeUtilization = %f seconds\n\n", ((double)(ct2 - ct1))/ CLOCKS_PER_SEC);

    /* InsertionSort: Input Numbers and Passing it to emu function for executing ARM instructions */
    printf("/**************** Result and Dynamic Analysis for \"Insertion Sort\" ***************/\n\n");
    printf("[Input/ Output @ %s] :::\n", "Insertion Sort");
    printf("<-------------- Input -------------->\n");
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
	printf("As the length of array is > 10 so Input array generated dynamically for you.\n");
    }
    insSort[0] = (unsigned) &lengthArray;
    insSort[1] = (unsigned) &insSortArray[0];
    ct1 = clock();
    rv = emu(&state, (void *) isort, 2, (unsigned *) insSort);
    ct2 = clock();
    insSortArray = (int *)rv;
    printf("<-------------- Output -------------->\n");
    printf("Below is the sorted Array.\n");
    for(i=0; i<lengthArray; i++) {
        printf("%d: %d\n", (i+1), insSortArray[i]);
    }
    printf("\n");
    totalRegCounts = registersUsage(&state);
    regReadAnalysis(&state, totalRegCounts, "Insertion Sort");
    regWriteAnalysis(&state, totalRegCounts, "Insertion Sort");
    instructionAnalysis(&state, "Insertion Sort"); 
    printf("[Performance Analysis @ %s] :::\n", "Insertion Sort");
    printf("<-------------- ARM Emulator -------------->\n");
    printf ("CLOCK_TICKS_PER_SEC = %ld\n", CLOCKS_PER_SEC);
    printf ("CPU TimeUtilization = %f seconds\n\n", ((double)(ct2 - ct1))/ CLOCKS_PER_SEC);
    ct1 = clock();
    rv = isort(insSort[0], insSort[1]);
    ct2 = clock();
    printf("<---------- Native Assembly Code ---------->\n");
    printf ("CPU TimeUtilization = %f seconds\n\n", ((double)(ct2 - ct1))/ CLOCKS_PER_SEC);

    return 0;
}

