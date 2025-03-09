Project Overview:
This project is a RISC-V assembler implemented in C++. The assembler reads RISC-V assembly code from an `input.asm` file and translates it into 32-bit machine code, outputting the results to an `output.mc` file. This project is part of **Phase 1** of the CS204 Computer Architecture course.

Features:
- Supports conversion of assembly code to machine code for **31 RISC-V instructions**.
- Generates an **output.mc** file with formatted machine code output.

 Supported Instructions:
The assembler supports the following **31 RISC-V instructions**:

 R-Type (Register Instructions):
- add, and, or, sll, slt, sra, srl, sub, xor, mul, div, rem

 I-Type (Immediate Instructions):
- addi, andi, ori, lb, lh, lw, ld, jalr

 S-Type (Store Instructions):
- sb, sh, sw, sd

SB-Type (Branch Instructions):
- beq, bne, blt, bge

U-Type (Upper Immediate Instructions):
- auipc, lui

UJ-Type (Jump Instruction):
- jal

File Structure:
- `assembler.cpp` - The main C++ implementation of the assembler.
- `input.asm` - The input assembly file containing RISC-V instructions.
- `output.mc` - The output file containing machine code.

Compilation:
To compile the assembler, use the following command:
 g++ -o assembler assembler.cpp

Running the Assembler
To execute the assembler:
 ./assembler

Output Format:
Each line in `output.mc` follows this format:

<address> <machine code> , <assembly instruction> # <binary representation>
For example:
0x0 0x003100B3 , add x1, x2, x3 # 0110011-000-0000000-00001-00010-00011-NULL
0x4 0x00A37293 , andi x5, x6, 10 # 0010011-111-NULL-00101-00110-000000001010

Implementation Details
1. First Pass:
   - Parses assembly code.
   - Builds a symbol table for labels.
   - Assigns memory addresses to instructions and data.

2. Second Pass:
   - Converts each instruction into binary machine code.
   - Resolves labels and generates output in hex format.
   - Outputs the final machine code to `output.mc`.


Key points:
-  Error handling: Detects syntax errors, missing labels, and invalid register names.
-  Out of range detection: Ensures that immediate values fit within their required bit-width constraints.
-  Flexible Immediate handling: Supports hexadecimal, decimal, octal, and ASCII characters for immediate values.
-  Offset handling: Correctly processes cases where an offset is given instead of a label.

Assumptions & Limitations
- Pseudo-instructions are NOT supported.
- Floating point operations are NOT supported.
- Instructions are limited to 31 RISC-V instructions as per project requirements.


