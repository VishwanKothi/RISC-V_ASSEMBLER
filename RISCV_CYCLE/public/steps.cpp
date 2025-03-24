#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <cstdlib> // for system()
#include <bitset>
#include <string>
#include <unordered_map>
#include <algorithm> // for sorting keys in dump functions

using namespace std;

#define MEMORY_BASE 0x10000000 // base address for data memory

// Global output stream for simulation logging.
ofstream outFile;
// Global output stream for web JSON-like logging.
ofstream webFile;

// Global container for holding log messages for the current cycle.
vector<string> cycleLogs;

// New type for sparse memory mapping.
using MemoryMap = unordered_map<uint32_t, uint8_t>;

//---------------------------------------------------------------------
// CPU structure holding registers and temporary registers.
// Changed to 32-bit types.
struct CPU {
    uint32_t PC; // Program counter.
    uint32_t IR; // Instruction register (32-bit instruction word).
    uint32_t regs[32]; // Register file (x0 - x31) 32-bit.
    uint32_t RA; // Temporary register (for operand 1).
    uint32_t RB; // Temporary register (for operand 2).
    uint32_t MAR; // Memory Address Register.
    uint32_t clock; // Clock cycle counter.
};

// Structure to hold decoded instruction fields.
struct DecodedInstruction {
    uint32_t opcode;
    uint32_t rd;
    uint32_t rs1;
    uint32_t rs2;
    int32_t imm; // Immediate (properly sign-extended).
    uint32_t funct3;
    uint32_t funct7;
};

//---------------------------------------------------------------------
// Helper: Convert a 32-bit number to a 32-character binary string (with leading zeros).
string toBinaryString32(uint32_t value) {
    return bitset<32>(value).to_string();
}

// Helper: Print a separator line for the table.
void printSeparator() {
    outFile << "+----------------+------------------------------------------------------------+\n";
}

// Helper: Log a table row with two columns: stage and message.
// Reset the fill character to a space before printing.
void logStage(const string &stage, const string &message) {
    outFile << setfill(' ') 
            << "| " << setw(14) << left << stage 
            << " | " << setw(60) << left << message << " |\n";
    ostringstream oss;
    oss << "{\"stage\":\"" << stage << "\",\"message\":\"" << message << "\"}";
    cycleLogs.push_back(oss.str());
}


// Forward declarations for decode, execute, and writeBack.
DecodedInstruction decode(uint32_t instr);
void execute(CPU &cpu, DecodedInstruction &di, uint32_t &aluResult);
void writeBack(CPU &cpu, DecodedInstruction &di, uint32_t aluResult, uint32_t memData);

//---------------------------------------------------------------------
// Get JSON string for register dump.
string getRegisterDumpJSON(const CPU &cpu) {
    ostringstream oss;
    oss << "[";
    for (int i = 0; i < 32; i++) {
        oss << "{\"reg\":\"x" << dec << i << "\",\"value\":\"0x" << hex << setw(8)
            << setfill('0') << cpu.regs[i] << "\"}";
        if (i != 31)
            oss << ",";
    }
    oss << "]";
    return oss.str();
}

//---------------------------------------------------------------------
// Updated Helper: Get JSON string for memory dump (sparse),
// grouping only the allocated 4-byte blocks. Within each block,
// print the bytes from high (addr+3) to low (addr) left-to-right in 2-digit hex.
// Undefined bytes are shown as "00".
string getMemoryDumpJSON(const MemoryMap &dmem) {
    ostringstream oss;
    oss << "[";
    if (dmem.empty()){
        oss << "]";
        return oss.str();
    }
    
    // Collect and sort all allocated addresses.
    vector<uint32_t> keys;
    for (const auto &pair : dmem)
        keys.push_back(pair.first);
    sort(keys.begin(), keys.end());
    
    // currentBlock holds the base address (multiple of 4) of the current group.
    uint32_t currentBlock = keys[0] - (keys[0] % 4);
    // Initialize a 4-byte group with zeros.
    vector<uint32_t> blockBytes(4, 0);
    
    bool firstLine = true;
    for (uint32_t addr : keys) {
        uint32_t block = addr - (addr % 4);
        // If we've moved to a new block, flush the previous block.
        if (block != currentBlock) {
            if (!firstLine)
                oss << ",";
            else
                firstLine = false;
            oss << "{\"address\":\"0x" 
                << hex << setw(8) << setfill('0') << currentBlock 
                << "\",\"value\":\"";
            // Print bytes from high (addr+3) to low (addr) as 2-digit hex.
            for (int i = 3; i >= 0; i--) {
                oss << setw(2) << setfill('0') << nouppercase << hex << blockBytes[i];
                if (i > 0)
                    oss << " ";
            }
            oss << "\"}";
            // Start a new block and reset blockBytes.
            currentBlock = block;
            blockBytes = vector<uint32_t>(4, 0);
        }
        // Set the corresponding byte from the allocated memory.
        int offset = addr % 4;
        blockBytes[offset] = dmem.at(addr);
    }
    // Flush the final block.
    if (!firstLine)
        oss << ",";
    oss << "{\"address\":\"0x" 
        << hex << setw(8) << setfill('0') << currentBlock 
        << "\",\"value\":\"";
    for (int i = 3; i >= 0; i--) {
        oss << setw(2) << setfill('0') << nouppercase << hex << blockBytes[i];
        if (i > 0)
            oss << " ";
    }
    oss << "\"}";
    
    oss << "]";
    return oss.str();
}


// Flush cycle logs to the web log file.
void flushCycleLogs(const unsigned int cycleNumber, bool &firstCycle, const CPU &cpu, const MemoryMap &dmem) {
    if (!firstCycle)
        webFile << ",\n";
    else
        firstCycle = false;
    webFile << " {\n";
    webFile << " \"cycle\": " << cycleNumber << ",\n";
    webFile << " \"logs\": [\n";
    for (size_t i = 0; i < cycleLogs.size(); i++) {
        webFile << " " << cycleLogs[i];
        if (i < cycleLogs.size() - 1)
            webFile << ",";
        webFile << "\n";
    }
    webFile << " ],\n";
    webFile << " \"registers\": " << getRegisterDumpJSON(cpu) << ",\n";
    webFile << " \"memory\": " << getMemoryDumpJSON(dmem) << "\n";
    webFile << " }";
}

//---------------------------------------------------------------------
// Updated Helper: Print register dump in grid format (8 registers per row).
void printRegisterDump(const uint32_t regs[32], ofstream &outFile) {
    outFile << "\n+-------------------------+\n";
    outFile << "| Register Dump |\n";
    outFile << "+-------------------------+\n";
    for (int i = 0; i < 32; i++) {
        outFile << "x" << setw(2) << right << dec << i << ": 0x"
                << hex << setw(8) << setfill('0') << regs[i] << " ";
        if ((i + 1) % 8 == 0)
            outFile << "\n";
    }
    outFile << dec << setfill(' ') << "\n";
}

//---------------------------------------------------------------------
// Updated Helper: Print data memory dump in a compact format,
// printing 4 bytes per line in big-endian order (high byte first, low byte last).
// The printed address is the first address in the block.
// If a block has fewer than 4 bytes, the missing high-order bytes are printed as "00".
void printMemoryDump(const MemoryMap &dmem, ofstream &outFile) {
    outFile << "\n+-------------------------+\n";
    outFile << "| Data Memory Dump |\n";
    outFile << "+-------------------------+\n";
    
    // Collect all allocated addresses and sort them.
    vector<uint32_t> addresses;
    for (const auto &pair : dmem)
        addresses.push_back(pair.first);
    sort(addresses.begin(), addresses.end());
    
    if (addresses.empty()) {
        outFile << "No data memory allocated.\n";
        return;
    }
    
    vector<uint32_t> block;
    // Use the first allocated address as the block start.
    uint32_t blockStart = addresses[0];
    uint32_t prevAddr = addresses[0] - 1; // so that first comparison works
    
    for (uint32_t addr : addresses) {
        // Check if the address is contiguous and block size is less than 4.
        if (!block.empty() && addr == prevAddr + 1 && block.size() < 4) {
            block.push_back(addr);
        }
        else {
            // Flush the current block if not empty.
            if (!block.empty()) {
                outFile << "0x" << hex << setw(8) << setfill('0') << blockStart << ": ";
                int missing = 4 - block.size();
                // Print missing high-order bytes first.
                for (int i = 0; i < missing; i++)
                    outFile << "00 ";
                // Then print the collected bytes in descending order (big-endian).
                for (int i = block.size() - 1; i >= 0; i--)
                    outFile << setw(2) << setfill('0') << (int)dmem.at(block[i]) << " ";
                outFile << "\n";
                block.clear();
            }
            // Start a new block.
            block.push_back(addr);
            blockStart = addr;
        }
        prevAddr = addr;
        
        // If the block reaches 4 bytes, flush it.
        if (block.size() == 4) {
            outFile << "0x" << hex << setw(8) << setfill('0') << blockStart << ": ";
            for (int i = 3; i >= 0; i--) {
                outFile << setw(2) << setfill('0') << (int)dmem.at(block[i]) << " ";
            }
            outFile << "\n";
            block.clear();
        }
    }
    // Flush any remaining bytes in the final block.
    if (!block.empty()) {
        outFile << "0x" << hex << setw(8) << setfill('0') << blockStart << ": ";
        int missing = 4 - block.size();
        for (int i = 0; i < missing; i++)
            outFile << "00 ";
        for (int i = block.size() - 1; i >= 0; i--)
            outFile << setw(2) << setfill('0') << (int)dmem.at(block[i]) << " ";
        outFile << "\n";
    }
}




//---------------------------------------------------------------------
// Memory access helpers for sparse memory.
uint8_t memRead(const MemoryMap &mem, uint32_t addr) {
    auto it = mem.find(addr);
    return (it != mem.end()) ? it->second : 0;
}

void memWrite(MemoryMap &mem, uint32_t addr, uint8_t value) {
    mem[addr] = value;
}

//---------------------------------------------------------------------
// Decode Stage: Convert instruction to binary string and extract fields.
DecodedInstruction decode(uint32_t instr) {
    DecodedInstruction di = {};
    string bin = toBinaryString32(instr);
    string opcodeStr = bin.substr(25, 7);
    di.opcode = stoi(opcodeStr, nullptr, 2);

    if(di.opcode == 0x33) { // R-type
        std::string funct7Str = bin.substr(0, 7);
        std::string rs2Str = bin.substr(7, 5);
        std::string rs1Str = bin.substr(12, 5);
        std::string funct3Str = bin.substr(17, 3);
        std::string rdStr = bin.substr(20, 5);
        di.funct7 = std::stoi(funct7Str, nullptr, 2);
        di.rs2 = std::stoi(rs2Str, nullptr, 2);
        di.rs1 = std::stoi(rs1Str, nullptr, 2);
        di.funct3 = std::stoi(funct3Str, nullptr, 2);
        di.rd = std::stoi(rdStr, nullptr, 2);
        di.imm = 0;
    }
    else if(di.opcode == 0x13 || di.opcode == 0x03 || di.opcode == 0x67) { // I-type
        std::string immStr = bin.substr(0, 12);
        std::string rs1Str = bin.substr(12, 5);
        std::string funct3Str = bin.substr(17, 3);
        std::string rdStr = bin.substr(20, 5);
        int32_t imm = std::stoi(immStr, nullptr, 2);
        if (imm & (1 << 11))
            imm |= 0xFFFFF000;
        di.imm = imm;
        di.rs1 = std::stoi(rs1Str, nullptr, 2);
        di.funct3 = std::stoi(funct3Str, nullptr, 2);
        di.rd = std::stoi(rdStr, nullptr, 2);
        di.rs2 = 0;
        di.funct7 = 0;
    }
    else if(di.opcode == 0x23) { // S-type
        std::string immHigh = bin.substr(0, 7);
        std::string immLow = bin.substr(20, 5);
        std::string rs1Str = bin.substr(12, 5);
        std::string rs2Str = bin.substr(7, 5);
        std::string funct3Str = bin.substr(17, 3);
        std::string immCombined = immHigh + immLow;
        int32_t imm = std::stoi(immCombined, nullptr, 2);
        if (imm & (1 << 11))
            imm |= 0xFFFFF000;
        di.imm = imm;
        di.rs1 = std::stoi(rs1Str, nullptr, 2);
        di.rs2 = std::stoi(rs2Str, nullptr, 2);
        di.funct3 = std::stoi(funct3Str, nullptr, 2);
        di.rd = 0;
        di.funct7 = 0;
    }
    else if(di.opcode == 0x63) { // SB-type (branch)
        std::string imm12 = bin.substr(0, 1);
        std::string imm10_5 = bin.substr(1, 6);
        std::string imm4_1 = bin.substr(20, 4);
        std::string imm11 = bin.substr(24, 1);
        std::string immCombined = imm12 + imm11 + imm10_5 + imm4_1;
        immCombined += "0";
        int32_t imm = std::stoi(immCombined, nullptr, 2);
        if (imm & (1 << 12))
            imm |= 0xFFFFE000;
        di.imm = imm;
        std::string rs1Str = bin.substr(12, 5);
        std::string rs2Str = bin.substr(7, 5);
        std::string funct3Str = bin.substr(17, 3);
        di.rs1 = std::stoi(rs1Str, nullptr, 2);
        di.rs2 = std::stoi(rs2Str, nullptr, 2);
        di.funct3 = std::stoi(funct3Str, nullptr, 2);
        di.rd = 0;
        di.funct7 = 0;
    }
    else if(di.opcode == 0x6F) { // UJ-type: JAL
        std::string imm20 = bin.substr(0, 1);
        std::string imm19_12 = bin.substr(12, 8);
        std::string imm11 = bin.substr(11, 1);
        std::string imm10_1 = bin.substr(1, 10);
        std::string rdStr = bin.substr(20, 5);
        std::string immCombined = imm20 + imm19_12 + imm11 + imm10_1;
        immCombined += "0";
        int32_t imm = std::stoi(immCombined, nullptr, 2);
        if (imm & (1 << 20))
            imm |= 0xFFF00000;
        di.imm = imm;
        di.rd = std::stoi(rdStr, nullptr, 2);
        di.rs1 = 0;
        di.rs2 = 0;
        di.funct3 = 0;
        di.funct7 = 0;
    }
    else if(di.opcode == 0x37 || di.opcode == 0x17) { // U-type: LUI or AUIPC
        std::string rdStr = bin.substr(20, 5);
        std::string immStr = bin.substr(0, 20);
        di.rd = std::stoi(rdStr, nullptr, 2);
        di.imm = std::stoi(immStr, nullptr, 2) << 12;
        di.rs1 = 0;
        di.rs2 = 0;
        di.funct3 = 0;
        di.funct7 = 0;
    }
    std::ostringstream oss;
    oss << "opcode=0x" << std::hex << di.opcode
        << ", rd=" << std::dec << di.rd
        << ", rs1=" << di.rs1
        << ", rs2=" << di.rs2
        << ", imm=" << di.imm
        << ", func3=0x" << std::hex << di.funct3
        << ", func7=0x" << std::hex << di.funct7;
    logStage("Decode", oss.str());
    return di;
    
}

//---------------------------------------------------------------------
// Execute Stage: Perform ALU operations based on the decoded instruction.
// Using 32-bit arithmetic.
void execute(CPU &cpu, DecodedInstruction &di, uint32_t &aluResult) {
    cpu.RA = cpu.regs[di.rs1];
    if(di.opcode == 0x33)
        cpu.RB = cpu.regs[di.rs2];
    else
        cpu.RB = 0;
    
        ostringstream oss;
        if(di.opcode == 0x33) { // R-type instructions
            if(di.funct3 == 0x0) {
                if(di.funct7 == 0x00) {
                    aluResult = cpu.RA + cpu.RB;
                    oss << "ADD: " << cpu.RA << " + " << cpu.RB << " = " << aluResult;
                } else if(di.funct7 == 0x20) {
                    aluResult = cpu.RA - cpu.RB;
                    oss << "SUB: " << cpu.RA << " - " << cpu.RB << " = " << aluResult;
                } else if(di.funct7 == 0x01) {
                    aluResult = cpu.RA * cpu.RB;
                    oss << "MUL: " << cpu.RA << " * " << cpu.RB << " = " << aluResult;
                } else {
                    oss << "Unsupported R-type funct7 for funct3=0x0.";
                }
            }
            else if(di.funct3 == 0x1 && di.funct7 == 0x00) {
                aluResult = cpu.RA << (cpu.RB & 0x1F);
                oss << "SLL: " << cpu.RA << " << " << (cpu.RB & 0x1F) << " = " << aluResult;
            }
            else if(di.funct3 == 0x2 && di.funct7 == 0x00) {
                aluResult = (static_cast<int32_t>(cpu.RA) < static_cast<int32_t>(cpu.RB)) ? 1 : 0;
                oss << "SLT: (" << cpu.RA << " < " << cpu.RB << ") = " << aluResult;
            }
            else if(di.funct3 == 0x4) {
                if(di.funct7 == 0x00) {
                    aluResult = cpu.RA ^ cpu.RB;
                    oss << "XOR: " << cpu.RA << " ^ " << cpu.RB << " = " << aluResult;
                } else if(di.funct7 == 0x01) {
                    if(cpu.RB == 0)
                        aluResult = static_cast<uint32_t>(-1);
                    else
                        aluResult = static_cast<uint32_t>(static_cast<int32_t>(cpu.RA) / static_cast<int32_t>(cpu.RB));
                    oss << "DIV: " << cpu.RA << " / " << cpu.RB << " = " << aluResult;
                } else {
                    oss << "Unsupported R-type funct7 for funct3=0x4.";
                }
            }
            else if(di.funct3 == 0x5) {
                if(di.funct7 == 0x00) {
                    aluResult = cpu.RA >> (cpu.RB & 0x1F);
                    oss << "SRL: " << cpu.RA << " >> " << (cpu.RB & 0x1F) << " = " << aluResult;
                } else if(di.funct7 == 0x20) {
                    aluResult = static_cast<uint32_t>(static_cast<int32_t>(cpu.RA) >> (cpu.RB & 0x1F));
                    oss << "SRA: " << cpu.RA << " >> " << (cpu.RB & 0x1F) << " (arith) = " << aluResult;
                } else {
                    oss << "Unsupported R-type funct7 for funct3=0x5.";
                }
            }
            else if(di.funct3 == 0x6) {
                if(di.funct7 == 0x00) {
                    aluResult = cpu.RA | cpu.RB;
                    oss << "OR: " << cpu.RA << " | " << cpu.RB << " = " << aluResult;
                } else if(di.funct7 == 0x01) {
                    if(cpu.RB == 0)
                        aluResult = cpu.RA;
                    else
                        aluResult = static_cast<uint32_t>(static_cast<int32_t>(cpu.RA) % static_cast<int32_t>(cpu.RB));
                    oss << "REM: " << cpu.RA << " % " << cpu.RB << " = " << aluResult;
                } else {
                    oss << "Unsupported R-type funct7 for funct3=0x6.";
                }
            }
            else if(di.funct3 == 0x7 && di.funct7 == 0x00) {
                aluResult = cpu.RA & cpu.RB;
                oss << "AND: " << cpu.RA << " & " << cpu.RB << " = " << aluResult;
            }
            else {
                oss << "Unknown R-type instruction.";
            }
        }
        else if(di.opcode == 0x13) { // I-type arithmetic
            if(di.funct3 == 0x0) {
                aluResult = cpu.RA + di.imm;
                oss << "ADDI: " << cpu.RA << " + " << di.imm << " = " << aluResult;
            } else if(di.funct3 == 0x7) {
                aluResult = cpu.RA & di.imm;
                oss << "ANDI: " << cpu.RA << " & " << di.imm << " = " << aluResult;
            } else if(di.funct3 == 0x6) {
                aluResult = cpu.RA | di.imm;
                oss << "ORI: " << cpu.RA << " | " << di.imm << " = " << aluResult;
            } else {
                aluResult = 0;
                oss << "Unsupported I-type arithmetic funct3.";
            }
        }
        else if(di.opcode == 0x03) { // Load instructions
            aluResult = cpu.RA + di.imm;
            oss << "Load Addr: " << cpu.RA << " + " << di.imm << " = " << aluResult;
        }
        else if(di.opcode == 0x67) { // JALR
            aluResult = cpu.regs[di.rs1] + di.imm;
            oss << "JALR Target = " << aluResult;
        }
        else if(di.opcode == 0x23) { // Store instructions
            aluResult = cpu.regs[di.rs1] + di.imm;
            oss << "Store Addr: " << cpu.regs[di.rs1] << " + " << di.imm << " = " << aluResult;
        }
        else if(di.opcode == 0x63) { // Branch instructions
            bool taken = false;
            switch(di.funct3) {
                case 0x0: // BEQ
                    taken = (cpu.regs[di.rs1] == cpu.regs[di.rs2]);
                    break;
                case 0x1: // BNE
                    taken = (cpu.regs[di.rs1] != cpu.regs[di.rs2]);
                    break;
                case 0x4: // BLT
                    taken = (static_cast<int32_t>(cpu.regs[di.rs1]) < static_cast<int32_t>(cpu.regs[di.rs2]));
                    break;
                case 0x5: // BGE
                    taken = (static_cast<int32_t>(cpu.regs[di.rs1]) >= static_cast<int32_t>(cpu.regs[di.rs2]));
                    break;
                case 0x6: // BLTU
                    taken = (cpu.regs[di.rs1] < cpu.regs[di.rs2]);
                    break;
                case 0x7: // BGEU
                    taken = (cpu.regs[di.rs1] >= cpu.regs[di.rs2]);
                    break;
                default:
                    break;
            }
            oss << "Branch: reg[" << di.rs1 << "]=" << cpu.regs[di.rs1]
                << ", reg[" << di.rs2 << "]=" << cpu.regs[di.rs2]
                << ", " << (taken ? "taken" : "not taken");
            // Note: Branch target calculation (if applicable) can be done here as well.
        }
        else if(di.opcode == 0x6F) { // JAL
            aluResult = cpu.PC + di.imm;
            oss << "JAL Target = " << aluResult;
        }
        else if(di.opcode == 0x37) { // LUI
            aluResult = di.imm;
            oss << "LUI: imm = " << aluResult;
        }
        else if(di.opcode == 0x17) { // AUIPC
            aluResult = cpu.PC + di.imm;
            oss << "AUIPC: PC + " << di.imm << " = " << aluResult;
        }
        else {
            oss << "Unknown opcode.";
        }
        cpu.MAR = aluResult;
        logStage("Execute", oss.str());
        
}

//---------------------------------------------------------------------
// Memory Access Stage: Now using sparse memory (MemoryMap).
// Supports lb, lh, lw for loads and sb, sh, sw for stores.
void memoryAccess(CPU &cpu, DecodedInstruction &di, uint32_t &aluResult, 
                  MemoryMap &dmem, uint32_t &memData) {
    ostringstream oss;
    // Use the absolute memory address.
    uint32_t addr = cpu.MAR;
    switch(di.opcode) {
        case 0x03: // Load instructions.
            switch(di.funct3) {
                case 0x0: { // LB: load byte (sign–extended)
                    int8_t val = static_cast<int8_t>(memRead(dmem, addr));
                    memData = static_cast<int32_t>(val);
                    oss << "LB from 0x" << hex << setw(8) << setfill('0') << cpu.MAR 
                        << " = " << dec << (int)val;
                    break;
                }
                case 0x1: { // LH: load halfword (2 bytes)
                    int16_t val = memRead(dmem, addr) | (memRead(dmem, addr + 1) << 8);
                    memData = static_cast<int32_t>(val);
                    oss << "LH from 0x" << hex << setw(8) << setfill('0') << cpu.MAR 
                        << " = " << dec << val;
                    break;
                }
                case 0x2: { // LW: load word (4 bytes)
                    int32_t val = memRead(dmem, addr) | (memRead(dmem, addr + 1) << 8) |
                                  (memRead(dmem, addr + 2) << 16) | (memRead(dmem, addr + 3) << 24);
                    memData = val;
                    oss << "LW from 0x" << hex << setw(8) << setfill('0') << cpu.MAR 
                        << " = " << dec << val;
                    break;
                }
                default:
                    oss << "Invalid load funct3.";
                    break;
            }
            break;
        case 0x23: // Store instructions.
            switch(di.funct3) {
                case 0x0: { // SB: store byte.
                    uint8_t byteVal = cpu.regs[di.rs2] & 0xFF;
                    memWrite(dmem, addr, byteVal);
                    oss << "SB: Store byte " << (int)byteVal 
                        << " at 0x" << hex << setw(8) << setfill('0') << cpu.MAR;
                    break;
                }
                case 0x1: { // SH: store halfword (2 bytes)
                    uint16_t halfVal = cpu.regs[di.rs2] & 0xFFFF;
                    memWrite(dmem, addr, halfVal & 0xFF);
                    memWrite(dmem, addr + 1, (halfVal >> 8) & 0xFF);
                    oss << "SH: Store half " << halfVal
                        << " at 0x" << hex << setw(8) << setfill('0') << cpu.MAR;
                    break;
                }
                case 0x2: { // SW: store word (4 bytes)
                    uint32_t wordVal = cpu.regs[di.rs2];
                    memWrite(dmem, addr, wordVal & 0xFF);
                    memWrite(dmem, addr + 1, (wordVal >> 8) & 0xFF);
                    memWrite(dmem, addr + 2, (wordVal >> 16) & 0xFF);
                    memWrite(dmem, addr + 3, (wordVal >> 24) & 0xFF);
                    oss << "SW: Store word " << wordVal
                        << " at 0x" << hex << setw(8) << setfill('0') << cpu.MAR;
                    break;
                }
                default:
                    oss << "Invalid store funct3.";
                    break;
            }
            break;
        default:
            oss << "No memory op.";
            break;
    }
    logStage("Memory", oss.str());
}

//---------------------------------------------------------------------
// Write Back Stage: Write results from ALU or memory back to the register file.
void writeBack(CPU &cpu, DecodedInstruction &di, uint32_t aluResult, uint32_t memData) {
    ostringstream oss;
    switch(di.opcode) {
        case 0x33:
        case 0x13:
            if(di.rd != 0) {
                cpu.regs[di.rd] = aluResult;
                oss << "Reg x" << di.rd << " = " << aluResult;
            } else {
                oss << "Attempt to write to reg x0 ignored.";
            }
            break;
        case 0x03:
            if(di.rd != 0) {
                cpu.regs[di.rd] = memData;
                oss << "Reg x" << di.rd << " = " << memData;
            } else {
                oss << "Attempt to write to reg x0 ignored.";
            }
            break;
        case 0x67:
            if(di.rd != 0) {
                cpu.regs[di.rd] = cpu.PC + 4;
                oss << "JALR: Reg x" << di.rd << " = " << cpu.PC+4;
            } else {
                oss << "Attempt to write to reg x0 ignored.";
            }
            break;
        case 0x6F:
            if(di.rd != 0) {
                cpu.regs[di.rd] = cpu.PC + 4;
                oss << "JAL: Reg x" << di.rd << " = " << cpu.PC+4;
            } else {
                oss << "Attempt to write to reg x0 ignored.";
            }
            break;
        case 0x37:
        case 0x17:
            if(di.rd != 0) {
                cpu.regs[di.rd] = aluResult;
                oss << "Reg x" << di.rd << " = " << aluResult;
            } else {
                oss << "Attempt to write to reg x0 ignored.";
            }
            break;
        case 0x63:
            oss << "No reg write for branch.";
            break;
        default:
            oss << "No write back.";
            break;
    }
    // Ensure that register x0 is always 0.
    cpu.regs[0] = 0;
    logStage("WriteBack", oss.str());
}

//---------------------------------------------------------------------
// isExitInstruction: Terminate when machine code equals 0x00000000.
bool isExitInstruction(uint32_t instr) {
    return (instr == 0x00000000);
}

//---------------------------------------------------------------------
// Fetch Stage: Using PC, get the instruction from instruction memory.
void fetch(CPU &cpu, const vector<uint32_t>& imem) {
    size_t index = cpu.PC / 4;
    if (index >= imem.size()) {
        logStage("Fetch", "PC out of range");
        cpu.IR = 0;
        return;
    }
    cpu.IR = imem[index];
    ostringstream oss;
    oss << "PC = 0x" << hex << setw(8) << setfill('0') << cpu.PC 
        << ", IR = 0x" << setw(8) << cpu.IR;
    logStage("Fetch", oss.str());
}

//---------------------------------------------------------------------
// Main Simulation Loop.
int main() {
    // Open simulation log file.
    outFile.open("simulation_output.txt");
    if (!outFile) {
        cerr << "Error: Could not open simulation_output.txt for writing." << endl;
        return 1;
    }
    // Open web log file.
    webFile.open("web.txt");
    if (!webFile) {
        cerr << "Error: Could not open web.txt for writing." << endl;
        return 1;
    }
    
    webFile << "[\n";
    printSeparator();
    logStage("SIMULATOR", "RISC-V 32-bit Functional Simulator");
    printSeparator();

    // Initialize CPU.
    CPU cpu;
    cpu.PC = 0;
    cpu.IR = 0;
    cpu.clock = 0;
    cpu.RA = 0;
    cpu.RB = 0;
    cpu.MAR = 0;
    for (int i = 0; i < 32; i++)
        cpu.regs[i] = 0;
    // Keep the stack pointer (x2) as originally intended.
    cpu.regs[2] = 0x7FFFFFDC;
    // Ensure reg x0 is always 0.
    cpu.regs[0] = 0;
    cpu.regs[3] = 0x10000000;
    cpu.regs[10] = 1;
    
    // Read machine code from "output.mc".
    vector<uint32_t> imem;
    MemoryMap dmem; // Use sparse memory mapping for data memory.
    ifstream mcFile("output.mc");
    if (!mcFile) {
        logStage("ERROR", "Could not open machine code file 'output.mc'.");
        outFile.close();
        webFile.close();
        return 1;
    }
    string line;
    while(getline(mcFile, line)) {
        if(line.empty())
            continue;
        istringstream iss(line);
        string addrToken, codeToken;
        iss >> addrToken >> codeToken;
        if (!codeToken.empty() && codeToken.back() == ',')
            codeToken.pop_back();
        uint32_t addr = stoul(addrToken, nullptr, 16);
        uint32_t code = stoul(codeToken, nullptr, 16);
        // Text segment: addresses below MEMORY_BASE.
        if(addr < MEMORY_BASE) {
            size_t index = addr / 4;
            if(imem.size() <= index)
                imem.resize(index + 1, 0);
            imem[index] = code;
        } else { // Data segment: store as a single byte.
            dmem[addr] = code & 0xFF;
        }
    }
    mcFile.close();

    bool running = true;
    bool firstCycle = true;
    while (running) {
        cycleLogs.clear();
        printSeparator();
        {
            ostringstream oss;
            oss << "Clock Cycle: " << (cpu.clock + 1);
            logStage("Cycle", oss.str());
        }
        printSeparator();
        
        if (cpu.PC / 4 >= imem.size()) {
            logStage("ERROR", "PC out of range. Ending simulation.");
            flushCycleLogs(cpu.clock, firstCycle, cpu, dmem);
            break;
        }
        
        // Fetch.
        fetch(cpu, imem);
        if (isExitInstruction(cpu.IR)) {
            logStage("EXIT", "Exit instruction encountered. Terminating simulation.");
            flushCycleLogs(cpu.clock + 1, firstCycle, cpu, dmem);
            ofstream dmemFile("dmem.mc");
            // Dump sparse memory to file (sorted by address).
            vector<uint32_t> keys;
            for (const auto &pair : dmem)
                keys.push_back(pair.first);
            sort(keys.begin(), keys.end());
            for (uint32_t addr : keys) {
                dmemFile << hex << setw(8) << setfill('0') << addr << " ";
                dmemFile << hex << setw(2) << setfill('0') << (int)memRead(dmem, addr) << "\n";
            }
            dmemFile.close();
            running = false;
            break;
        }
        
        // Decode.
        DecodedInstruction di = decode(cpu.IR);
        // Execute.
        uint32_t aluResult = 0;
        execute(cpu, di, aluResult);
        // Memory Access.
        uint32_t memData = 0;
        memoryAccess(cpu, di, aluResult, dmem, memData);
        // Write Back.
        writeBack(cpu, di, aluResult, memData);
        
        // Update PC.
        if (di.opcode == 0x63) {
            if (di.funct3 == 0x0) {
                if (cpu.regs[di.rs1] == cpu.regs[di.rs2]) {
                    logStage("Branch", "BEQ taken");
                    cpu.PC = cpu.PC + di.imm;
                } else {
                    logStage("Branch", "BEQ not taken");
                    cpu.PC += 4;
                }
            } else if (di.funct3 == 0x1) {
                if (cpu.regs[di.rs1] != cpu.regs[di.rs2]) {
                    logStage("Branch", "BNE taken");
                    cpu.PC = cpu.PC + di.imm;
                } else {
                    logStage("Branch", "BNE not taken");
                    cpu.PC += 4;
                }
            } else if (di.funct3 == 0x4) {
                if (static_cast<int32_t>(cpu.regs[di.rs1]) < static_cast<int32_t>(cpu.regs[di.rs2])) {
                    logStage("Branch", "BLT taken");
                    cpu.PC = cpu.PC + di.imm;
                } else {
                    logStage("Branch", "BLT not taken");
                    cpu.PC += 4;
                }
            } else if (di.funct3 == 0x5) {
                if (static_cast<int32_t>(cpu.regs[di.rs1]) >= static_cast<int32_t>(cpu.regs[di.rs2])) {
                    logStage("Branch", "BGE taken");
                    cpu.PC = cpu.PC + di.imm;
                } else {
                    logStage("Branch", "BGE not taken");
                    cpu.PC += 4;
                }
            } else {
                cpu.PC += 4;
            }
        }
        else if (di.opcode == 0x6F || di.opcode == 0x67) {
            cpu.PC = aluResult;
        }
        else {
            cpu.PC += 4;
        }
        
        cpu.clock++;
        logStage("Cycle End", ("End of cycle " + to_string(cpu.clock)));
        printRegisterDump(cpu.regs, outFile);
        printMemoryDump(dmem, outFile);
        flushCycleLogs(cpu.clock, firstCycle, cpu, dmem);
    }
    
    printSeparator();
    {
        ostringstream oss;
        oss << "Simulation finished. Total clock cycles: " << cpu.clock;
        logStage("FINAL", oss.str());
    }
    printSeparator();
    printMemoryDump(dmem, outFile);
    outFile.close();
    webFile << "\n]\n";
    webFile.close();
    
    char guiChoice;
    cout << "Simulation finished. Total clock cycles: " << cpu.clock << "\n";
    cout << "Do you want to open the simulation output in a text editor? (y/n): ";
    cin >> guiChoice;
    if(guiChoice == 'y' || guiChoice == 'Y') {
        #ifdef _WIN32
            system("notepad simulation_output.txt");
        #else
            system("xdg-open simulation_output.txt");
        #endif
    }
    
    return 0;
}