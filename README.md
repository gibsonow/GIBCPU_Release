# GIBCPU
## An 8-bit Von Neumann CPU Designed from Scratch

Video Explainer: https://youtu.be/7w8IeXKstJY

## Pitch

A fully custom CPU design, emulated in C. Features:
- 256 bytes of addressable memory
- 4 general purpose registers
- Asynchronous internal module operation
- Fully Custom Assembly Instruction Set, including:
	- 8 arithmetic operations
	- 7 memory operations (including conditional branching)

Also included:
- Assembler that generates accurate bytecode from assembly
- Breakout text files to show assembly and bytecode
- Excel sheet containing:
	- Full description of the Custom Assembly Instruction Set
	- Examples of some common programming loops in Assembly
	- Several example programs, including **Conway's Game of Life**

## Execution

- Copy and paste a pre-written Assembly program or write your own program in "assembly.txt".
	- "assembly.txt" is pre-loaded with Conway's Game of Life.
- Run "Assembler.c". This should generate a text file named "RAM.txt", which contains CPU-readable bytecode, or replace the existing bytecode if the file already exists.
- Run "CPU_Emulator.c".
	- WARNING: Emulator currently prints a memory-mapped range of RAM addresses that correspond to the latest version of Conway's Game of Life.