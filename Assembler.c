#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <limits.h>

#define MAX_INSTRUCTIONS 256
#define MAX_WORDS 3
#define MAX_WORD_LENGTH 20

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

#define r0      0
#define r1      1
#define r2      2
#define r3      3

void removeFirstChar(char *str) {
    if (str && *str) { // Check for NULL pointer and empty string
        memmove(str, str + 1, strlen(str));
    }
}

int stringToInt(const char *str) {
    int num = 0;
    const int n = 10;  // Base 10 for decimal numbers

    // Check for null pointer
    if (str == NULL) {
        return 0; // Return zero if the input is NULL
    }

    // Skip whitespace
    while (*str == ' ') {
        str++;
    }

    // Convert each character to the corresponding integer value
    while (*str >= '0' && *str <= '9') {
        if (num > INT_MAX / n || (num == INT_MAX / n && (*str - '0') > INT_MAX % n)) {
            // Handle overflow
            return INT_MAX;
        }
        num = num * n + (*str - '0');
        str++;
    }

    return num;
}

int varToInt(char *str){
    char var[10];
    strcpy(var,str);
    removeFirstChar(var);
    return stringToInt(var); 
}

int readAssemblyInstructions(const char *filename, char instructions[MAX_INSTRUCTIONS][MAX_WORDS][MAX_WORD_LENGTH]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[3 * MAX_WORD_LENGTH + 2]; // +2 for potential spaces and a newline
    int instr_count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (instr_count >= MAX_INSTRUCTIONS) {
            fprintf(stderr, "Error: Instruction limit exceeded\n");
            break;
        }

        // Remove newline character if present
        line[strcspn(line, "\n")] = 0;

        // Tokenize the instruction line
        char *token = strtok(line, " ");
        int word_count = 0;
        
        while (token != NULL && word_count < MAX_WORDS) {
            strncpy(instructions[instr_count][word_count], token, MAX_WORD_LENGTH);
            instructions[instr_count][word_count][MAX_WORD_LENGTH - 1] = '\0'; // Ensure null-termination
            token = strtok(NULL, " ");
            word_count++;
        }

        // Log the instruction read
        printf("Instruction %d: %s %s %s\n", instr_count, 
               instructions[instr_count][0],
               word_count > 1 ? instructions[instr_count][1] : "",
               word_count > 2 ? instructions[instr_count][2] : "");

        instr_count++;
    }

    fclose(file);
    return instr_count;
}

void writeBinaryFile(const char *filename, const uint8_t *values, size_t length) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    for (size_t i = 0; i < length; ++i) {
        for (int j = 7; j >= 0; --j) {
            // Write each bit of the byte
            fprintf(file, "%c", (values[i] & (1 << j)) ? '1' : '0');
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

int main() {
    char instructions[MAX_INSTRUCTIONS][MAX_WORDS][MAX_WORD_LENGTH] = {0};
    uint8_t ram[MAX_INSTRUCTIONS] = {0};
    uint8_t variableLocations[MAX_INSTRUCTIONS] = {0};
    uint8_t locationLocations[MAX_INSTRUCTIONS] = {0};
    uint8_t num_memops[MAX_INSTRUCTIONS] = {0};
    int ram_location = 0;
    
    int instr_count = readAssemblyInstructions("assembly.txt", instructions);

    for(int i = 0; i < instr_count; i++){
        if(isalpha(instructions[i][0][0])){                                 // for all commands
            // add register values to command first

            // For two-variable operations, add register A
            if(!strcmp(instructions[i][0], "and")   ||                      
            !strcmp(instructions[i][0], "or")       || 
            !strcmp(instructions[i][0], "xor")      || 
            !strcmp(instructions[i][0], "add")      ||
            !strcmp(instructions[i][0], "sub")      ||
            !strcmp(instructions[i][0], "loadl")    ||
            !strcmp(instructions[i][0], "wrtl") )    
            {
                ram[ram_location] += (instructions[i][1][1] - '0') << 2;    // add register A
            }

            // For all operations that use at least one register, add register B
            if(!strcmp(instructions[i][0], "load")  ||                      // mem operations pull register B from slot 1
            !strcmp(instructions[i][0], "wrt")      || 
            !strcmp(instructions[i][0], "jmpz")     || 
            !strcmp(instructions[i][0], "notb")     || 
            !strcmp(instructions[i][0], "shiftb")   || 
            !strcmp(instructions[i][0], "lshiftb"))
            {
                ram[ram_location] += (instructions[i][1][1]) - '0'; 
            }
            else if(strcmp(instructions[i][0], "halt") && strcmp(instructions[i][0], "jmp")){    // HALT and JUMP have no register calls. This last case may be redundant
                ram[ram_location] += (instructions[i][2][1]) - '0';
            }

            // add correct command opcode
            if(!strcmp(instructions[i][0], "load")  ||                      // Can't do case statement on strings apparently
            !strcmp(instructions[i][0], "wrt")      || 
            !strcmp(instructions[i][0], "jmpz")     || 
            !strcmp(instructions[i][0], "jmp"))
            {
                printf("mem operation\n");
                if(!strcmp(instructions[i][0], "load")){
                    ram[ram_location] += LOAD;
                }
                else if(!strcmp(instructions[i][0], "wrt")){
                    ram[ram_location] += WRT;
                }
                else if(!strcmp(instructions[i][0], "jmpz")){
                    ram[ram_location] += JMPZ;
                }
                else if(!strcmp(instructions[i][0], "jmp")){
                    ram[ram_location] += JMP;
                }
                else{
                    printf("weehee woohoo!\n");
                }
                ram_location += 2;                                          // each memory operation is followed by a variable which is filled in later
                num_memops[i+1]++;                                          // track number of memops for variable offset later
            }
            else{
                printf("arithmetic operation\n");
                if(!strcmp(instructions[i][0], "and")){
                    ram[ram_location] += AND;
                }
                else if(!strcmp(instructions[i][0], "or")){
                    ram[ram_location] += OR;
                }
                else if(!strcmp(instructions[i][0], "xor")){
                    ram[ram_location] += XOR;
                }
                else if(!strcmp(instructions[i][0], "add")){
                    ram[ram_location] += ADD;
                }
                else if(!strcmp(instructions[i][0], "sub")){
                    ram[ram_location] += SUB;
                }
                else if(!strcmp(instructions[i][0], "notb")){
                    ram[ram_location] += NOTB;
                }
                else if(!strcmp(instructions[i][0], "shiftb")){
                    ram[ram_location] += SHIFTB;
                }
                else if(!strcmp(instructions[i][0], "lshiftb")){
                    ram[ram_location] += LSHIFTB;
                }
                else if(!strcmp(instructions[i][0], "halt")){
                    ram[ram_location] += HALT;
                }
                else if(!strcmp(instructions[i][0], "loadl")){              //not ALUops, but only 1 byte
                    ram[ram_location] += LOADL;
                }
                else if(!strcmp(instructions[i][0], "wrtl")){
                    ram[ram_location] += WRTL;
                }
                else{
                    printf("wahoo weehee!\n");
                }
                ram_location++;
            }
        }
        else{                                                               // for all data variables / jump locations
            int varNum = 0;
            if(instructions[i][0][0] == '$'){   // variable
                printf("variable\n");
                ram[ram_location] = stringToInt(instructions[i][1]);        // enter binary data for variable at variable location
                varNum = varToInt(instructions[i][0]);
                variableLocations[varNum] = ram_location;                   // log variable location
            }
            else{                               // location
                printf("location\n");
                ram[ram_location] = stringToInt(instructions[i][1]);
                varNum = varToInt(instructions[i][0]);
                locationLocations[varNum] = ram_location;
            }
            ram_location++;
        }
        if(i != 0) num_memops[i] += num_memops[i-1];                        // running tally of memops along instruction set
    }

    // backtrack through the RAM and fill in the nextVals for all memory operations
    int backtrack = 0;
    int varNum = 0;
    uint8_t locationAlreadySet[MAX_INSTRUCTIONS] = {0};
    for(int i = 0; i < instr_count; i++){
        if(instructions[i][2][0] == '$'){                                   // for commands with variables in word 2,
            backtrack++;                                                    // move to variable location position
            varNum = varToInt(instructions[i][2]);                          // parse variable name
            ram[backtrack] = variableLocations[varNum];                     // place variable location in position
        }                                                                   // (no need for binary offset, these are true program values set by user)
        if(instructions[i][2][0] == '#'){                                   // for commands with locations in word 2 (jmpz)
            backtrack++;
            varNum = varToInt(instructions[i][2]);
            ram[backtrack] = locationLocations[varNum];
            //offset jump address by number of memops up to that point to make jump position accurate to the binary file size
            if(!locationAlreadySet[varNum]){
                ram[locationLocations[varNum]] += num_memops[ram[locationLocations[varNum]]];  
                locationAlreadySet[varNum] = 1;
            }
        }
        if(instructions[i][1][0] == '#'){                                   // for commands with locations in word 1 (jmp)
            backtrack++;
            varNum = varToInt(instructions[i][1]);
            ram[backtrack] = locationLocations[varNum];
            if(!locationAlreadySet[varNum]){
                ram[locationLocations[varNum]] += num_memops[ram[locationLocations[varNum]]];  
                locationAlreadySet[varNum] = 1;
            } 
        }
        backtrack++;
    }
    
    // print RAM
    for(int i = 0; i < ram_location; i++){
        printf("%u\n", ram[i]);
    }

    writeBinaryFile("RAM.txt", ram, ram_location);

    return 0;
}
