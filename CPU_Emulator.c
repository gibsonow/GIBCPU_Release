#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define MAX_VALUES 256
#define BINARY_STRING_LENGTH 8

#define AND     0
#define OR      16
#define XOR     32
#define ADD     48
#define SUB     64
#define NOTB    80
#define SHIFTB  96
#define LSHIFTB 112
#define LOAD    128
#define WRT     144
#define JMPZ    160
#define JMP     176
#define LOADL   192
#define WRTL    208
#define HALT    240

int loopCounter = 0;
int posEdgeCounter = 0;

uint8_t currentStateFirst = 161;    // binary address of #currentStateFirst
uint8_t nextStateFirst = 203;       // binary address of #nextStateFirst
uint8_t progCounterPrintAddr = 6;   // once progCounter hits this, the currentState grid will be printed

uint8_t ram[MAX_VALUES] = {0};

uint8_t reg0 = 0;
uint8_t reg1 = 0;
uint8_t reg2 = 0;
uint8_t reg3 = 0;

uint8_t regA = 0;
uint8_t regB = 0;

uint8_t ALUout = 0;

uint8_t count = 0;
uint8_t programHalt = 0;

uint8_t ramDataOut = 0;

uint8_t command = 0;
uint8_t memCtrlRAM = 0;
uint8_t setRAM = 0;
uint8_t RAMSet = 0;
uint8_t memCtrlCount = 0;
uint8_t setCount = 0;
uint8_t countSet = 0;
uint8_t incrementCount = 0;
uint8_t countIncremented = 0;
uint8_t memCtrlReg = 0;
uint8_t setReg = 0;
uint8_t regSet = 0;
uint8_t state = 0;
uint8_t Mdata = 0;
uint8_t bookmark = 0;

/* FILE IO FUNCTIONS */

//  Function to print the "CurrentState" grid to the terminal
void printGrid(uint8_t startIndex) {
    // Ensure the start index and the next 36 addresses are within the array bounds
    if (startIndex + 35 >= MAX_VALUES) {
        printf("Start index is out of bounds.\n");
        return;
    }

    printf("-----------------------------\n");
    for (int i = 0; i < 36; i++) {
        if (ram[startIndex + i] == 1) {
            printf("\u2593");       // Dark shaded block
        } else {
            printf("\u2591");       // Light shaded block
        }
        
        if ((i + 1) % 6 == 0) {     // New line after every 6 characters
            printf("\n");
        }
    }
}

// Function to convert a binary string to a uint8_t value
uint8_t binaryStringToUint8(const char *binaryString) {
    uint8_t value = 0;
    for (int i = 0; i < BINARY_STRING_LENGTH; i++) {
        value <<= 1;
        if (binaryString[i] == '1') {
            value |= 1;
        }
    }
    return value;
}

// Function to load up to 256 8-bit binary values from "RAM.txt" into an array
size_t loadBinaryValues(const char *filename, uint8_t *array, size_t max_values) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return 0;
    }

    char line[BINARY_STRING_LENGTH + 2]; // 8 bits + newline + null terminator
    size_t count = 0;
    while (count < max_values && fgets(line, sizeof(line), file)) {
        // Remove newline character if present
        line[strcspn(line, "\r\n")] = 0;

        // Check if the line contains exactly 8 binary digits
        if (strlen(line) == BINARY_STRING_LENGTH) {
            array[count++] = binaryStringToUint8(line);
        }
    }

    fclose(file);
    return count; // Return the number of values loaded into the array
}

/* CPU MODULE FUNCTIONS */

// "Arithmetic Logic Unit", Module that performs Arithmetic Operations on RegA and RegB
void ALU(){
    switch((command & 0b11110000)){
        case AND:
            ALUout = regB & regA;
            break;
        case OR:
            ALUout = regB | regA;
            break;
        case XOR:
            ALUout = regB ^ regA;
            break;
        case ADD:
            ALUout = regB + regA;
            break;
        case SUB:
            ALUout = regB - regA;
            break;
        case NOTB:
            ALUout = ~regB;
            break;
        case SHIFTB:
            ALUout = regB >> 1;
            break;
        case LSHIFTB:
            ALUout = regB << 1;
            break;
        default:
            ALUout = 0;
            break;
    }
}

// reg = destination reg (0 = A, 1 = B), num = source reg (reg0,reg1,reg2,reg3)
void twoRegPathCase(uint8_t reg, uint8_t num){     
    switch(reg){
        case 0:
            switch(num){
                case 0:
                    regA = reg0;
                    break;
                case 1:
                    regA = reg1;
                    break;
                case 2:
                    regA = reg2;
                    break;
                case 3:
                    regA = reg3;
                    break;
                default:
                    break;
            }
            break;
        case 1:
            switch(num){
                case 0:
                    regB = reg0;
                    break;
                case 1:
                    regB = reg1;
                    break;
                case 2:
                    regB = reg2;
                    break;
                case 3:
                    regB = reg3;
                    break;
                default:
                    break;
            }
        default:
            break;
    }
}

// Module that loads RegA and RegB with correct registers based on the Command
void regPathSet(){
    uint8_t instruction = ((command & 0b11110000) >> 4);
    if(instruction < 5 || instruction == 12 || instruction == 13){  //if two-variable op
        twoRegPathCase(0, (command & 0b1100) >> 2);                 //set reg a
        twoRegPathCase(1, (command & 0b11));                        //set reg b
    }
    else if(instruction >= 5 && instruction < 11){                  //if one-variable op
        twoRegPathCase(1, (command & 0b11));                        //set reg b
    }
}

// Module that loads the registers with a given module source based on the Command
void regBank(){
    if(setReg && !regSet){
        if((command & 0b10000000) >> 7){        // if not an ALU op, pull from memCtrlReg
            switch (command & 0b11){
                case 0:
                    reg0 = memCtrlReg;          // reg0 is now writable
                    break;
                case 1:
                    reg1 = memCtrlReg;
                    break;
                case 2:
                    reg2 = memCtrlReg;
                    break;
                case 3:
                    reg3 = memCtrlReg;
                    break;
                default:
                    break;
            }
        }
        else{                                   // if ALU op, pull from ALU
            switch (command & 0b11){
                case 0:
                    reg0 = ALUout;              // reg0 is now writable
                    break;
                case 1:
                    reg1 = ALUout;
                    break;
                case 2:
                    reg2 = ALUout;
                    break;
                case 3:
                    reg3 = ALUout;
                    break;
                default:
                    break;
            }
        }
        regSet = 1;
    }
    if(!setReg && regSet){
        regSet = 0;
    }
}

// Module that reads/writes specific memory locations in the RAM
void ramModule(){
    if(setRAM && !RAMSet){
        ram[count] = memCtrlRAM;
        RAMSet = 1;
    }
    if(!setRAM && RAMSet){
        RAMSet = 0;
    }
    ramDataOut = ram[count];
}

// Module that increments or sets the Program Counter
void progCounter(){
    if(setCount && !countSet){
        count = memCtrlCount;
        countSet = 1;
    }
    if(!setCount && countSet){
        countSet = 0;
    }
    if(incrementCount && !countIncremented){
        count++;
        countIncremented = 1;
        // DEBUG
        if(count == progCounterPrintAddr){
            printGrid(currentStateFirst);
            loopCounter++;
        }
    }
    if(!incrementCount && countIncremented){
        countIncremented = 0;
    }
}

// Module that controls all other modules.
void memCtrl(){
    switch(state){
        case 0:                     // get next ram data
            command = ramDataOut;
            if(command & 0b10000000){
                state = 2;
            }
            else{
                state = 1;
            }
            break;
        case 1:                     // ALU op
            setReg = 1;
            state = 19;
            break;
        case 2:
            bookmark = count;
            if(((command & 0b11110000) == LOADL) || ((command & 0b11110000) == WRTL)){   //if LOADL or WRTL
                state = 3;
            }
            else{
                state = 4;
            }
            break;
        case 3:
            memCtrlCount = regA;
            state = 6;
            break;
        case 4:
            incrementCount = 1;
            state = 5;
            break;
        case 5:
            if(countIncremented){
                incrementCount = 0;
                memCtrlCount = ramDataOut;
                state = 6;
            }
            break;
        case 6:
            setCount = 1;
            state = 7;
            break;
        case 7:
            if(countSet){
                setCount = 0;
                Mdata = ramDataOut;
                switch(command & 0b11110000){
                    case LOAD:
                    case LOADL:
                        state = 8;
                        break;
                    case WRT:
                    case WRTL:
                        state = 9;
                        break;
                    case JMPZ:
                        if(regB == 0){
                            state = 15;
                        }
                        else{
                            state = 11;
                        }
                        break;
                    case JMP:
                        state = 15;
                        break;
                    case HALT:
                        state = 18;
                        break;
                    default:
                        break;
                }
            }
            break;
        case 8:                     // LOAD or LOADL
            memCtrlReg = Mdata;
            setReg = 1;
            state = 20;
            break;
        case 9:                     // WRT or WRTL
            memCtrlRAM = regB;
            setRAM = 1;
            state = 10;
            break;
        case 10:
            if(RAMSet){
                setRAM = 0;
                state = 11;
            }
            break;
        case 11:                    // return to bookmark
            memCtrlCount = bookmark;
            setCount = 1;
            state = 12;
            break;
        case 12:
            if(countSet){
                setCount = 0;
                incrementCount = 1;
                if(!((command & 0b11110000) == LOADL) && !((command & 0b11110000) == WRTL)){    // if a command that has a second byte
                    state = 13;     // increment the program counter again
                }
                else{
                    state = 17;
                }
            }
            break;
        case 13:
            if(countIncremented){
                incrementCount = 0;
                state = 14;
            }
            break;
        case 14:
            incrementCount = 1;
            state = 17;
            break;
        case 15:                     // JMP or Passing JMPZ
            memCtrlCount = Mdata;
            setCount = 1;
            state = 16;
            break;
        case 16:
            if(countSet){
                setCount = 0;
                state = 0;
            }
            break;
        case 17:
            if(countIncremented){
                incrementCount = 0;
                state = 0;
            }
            break;
        case 18:
            programHalt = 1;
            break;
        case 19:
            if(regSet){
                setReg = 0;
                incrementCount = 1;
                state = 17;
            }
            break;
        case 20:
            if(regSet){
                setReg = 0;
                state = 11;
            }
            break;
        default:
            break;
    }
}

int main(){

    // load RAM with the binary file
    size_t loadedValues = loadBinaryValues("RAM.txt", ram, MAX_VALUES);

    clock_t t;
    t = clock();

    while(!programHalt){
        posEdgeCounter++;
        regBank();          // ALUOut OR memCtrlReg -> reg0-3
        regPathSet();       // reg0-3 -> regA-B
        ALU();              // regA-B -> ALUOut
        progCounter();      // count++ OR memCtrlCount -> count
        ramModule();        // memCtrlRAM -> ram[count] AND ram[count] -> ramDataOut
        memCtrl();          // CPU Control State Machine
    }

    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;

    printf("\nPROGRAM HALTED\n");
    printf("\nProgram iterated %d times over %d clock cycles in %f seconds.\n",loopCounter,posEdgeCounter,time_taken);

    return 0;
}