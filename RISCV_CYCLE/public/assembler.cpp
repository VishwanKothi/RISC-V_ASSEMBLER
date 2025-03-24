#include <bits/stdc++.h>
using namespace std;

// Utility Functions
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

vector<string> split(const string &s, char delim) {
    vector<string> tokens;
    string token;
    istringstream iss(s);
    while(getline(iss, token, delim)) {
        token = trim(token);
        if(!token.empty())
            tokens.push_back(token);
    }
    return tokens;
}

string replacecommaswithspaces(const string &s) {
    string result = s;
    for (char &c : result) {
        if (c == ',')
            c = ' ';
    }
    return result;
}

// Register mapping
unordered_map<string, int> regmap = {
    {"zero",0},{"ra",1},{"sp",2},{"gp",3},{"tp",4},
    {"t0",5},{"t1",6},{"t2",7},{"s0",8},{"s1",9},
    {"a0",10},{"a1",11},{"a2",12},{"a3",13},{"a4",14},
    {"a5",15},{"a6",16},{"a7",17},{"s2",18},{"s3",19},
    {"s4",20},{"s5",21},{"s6",22},{"s7",23},{"s8",24},
    {"s9",25},{"s10",26},{"s11",27},{"t3",28},{"t4",29},
    {"t5",30},{"t6",31}
};

int register_to_int(const string &reg) {
    if(regmap.find(reg) != regmap.end()){
        return regmap[reg];
    }
    if(reg[0]=='x' || reg[0]=='X')
        return stoi(reg.substr(1));
    return -1;
}

// Parse immediate values from a string supporting multiple formats.
int parseImmediate(const string &s) {
    if(s.size() >= 2 && s.front() == '\'' && s.back() == '\''){
        return static_cast<int>(s[1]);
    }
    if(s.size() > 2 && s[0]=='0' && (s[1]=='b' || s[1]=='B')){
        int val = 0;
        for(size_t i = 2; i < s.size(); i++){
            char c = s[i];
            val = val * 2 + (c - '0');
        }
        return val;
    }
    return stoi(s, nullptr, 0);
}

string toBinaryString(uint32_t value, int width) {
    string result;
    for (int i = width - 1; i >= 0; i--){
        result.push_back((value & (1u << i)) ? '1' : '0');
    }
    return result;
}

string binaryToHex(const string &bin) {
    uint32_t num = 0;
    for(char c : bin) {
        num = (num << 1) + (c - '0');
    }
    stringstream ss;
    ss << "0x" << setw(8) << setfill('0') << hex << uppercase << num;
    return ss.str();
}

// Data Structures
struct assembly_line_structure {
    string original;   // original text (with label removed if needed)
    uint32_t address;  // assigned address (text or data)
    string section;    // "text" or "data"
};

vector<assembly_line_structure> assembly_lines;
unordered_map<string, uint32_t> symbolTable;

uint32_t text_address = 0x00000000; // Code starts at 0x00000000
uint32_t data_address = 0x10000000; // Data starts at 0x10000000
string current_section = "text";

// Instruction format mapping.
struct instruction_format {
    string type;   // "R", "I", "S", "SB", "U", "UJ"
    string opcode;
    string funct3;
    string funct7; // only for R-type
};

unordered_map<string, instruction_format> instruction_map = {
    // R-type
    {"add",{"R","0110011","000","0000000"}},
    {"and",{"R","0110011","111","0000000"}},
    {"or",{"R","0110011","110","0000000"}},
    {"sll",{"R","0110011","001","0000000"}},
    {"slt",{"R","0110011","010","0000000"}},
    {"sra",{"R","0110011","101","0100000"}},
    {"srl",{"R","0110011","101","0000000"}},
    {"sub",{"R","0110011","000","0100000"}},
    {"xor",{"R","0110011","100","0000000"}},
    {"mul",{"R","0110011","000","0000001"}},
    {"div",{"R","0110011","100","0000001"}},
    {"rem",{"R","0110011","110","0000001"}},
    // I-type
    {"addi",{"I","0010011","000","NULL"}},
    {"andi",{"I","0010011","111","NULL"}},
    {"ori",{"I","0010011","110","NULL"}},
    {"lb",{"I","0000011","000","NULL"}},
    {"lh",{"I","0000011","001","NULL"}},
    {"lw",{"I","0000011","010","NULL"}},
    {"ld",{"I","0000011","011","NULL"}},
    {"jalr",{"I","1100111","000","NULL"}},
    // S-type
    {"sb",{"S","0100011","000","NULL"}},
    {"sh",{"S","0100011","001","NULL"}},
    {"sw",{"S","0100011","010","NULL"}},
    {"sd",{"S","0100011","011","NULL"}},
    // SB-type
    {"beq",{"SB","1100011","000","NULL"}},
    {"bne",{"SB","1100011","001","NULL"}},
    {"blt",{"SB","1100011","100","NULL"}},
    {"bge",{"SB","1100011","101","NULL"}},
    // U-type
    {"lui",{"U","0110111","NULL","NULL"}},
    {"auipc",{"U","0010111","NULL","NULL"}},
    // UJ-type
    {"jal",{"UJ","1101111","NULL","NULL"}}
};

// Instruction Encoding Functions
string encode_R_format(const string &operation, int rd, int rs1, int rs2) {
    instruction_format fmt = instruction_map[operation];
    string bin;
    bin = fmt.funct7 + toBinaryString(rs2, 5) + toBinaryString(rs1, 5) + fmt.funct3 + toBinaryString(rd, 5) + fmt.opcode;
    return bin;
}

string encode_I_format(const string &operation, int rd, int rs1, int imm) {
    instruction_format fmt = instruction_map[operation];
    string immBin = toBinaryString((uint32_t)(imm & 0xFFF), 12);
    string bin;
    bin = immBin + toBinaryString(rs1, 5) + fmt.funct3 + toBinaryString(rd, 5) + fmt.opcode;
    return bin;
}

string encodeSType(const string &operation, int rs1, int rs2, int imm) {
    instruction_format fmt = instruction_map[operation];
    uint32_t immVal = imm & 0xFFF;
    string immBin = toBinaryString(immVal, 12);
    string immHigh = immBin.substr(0, 7);  // imm[11:5]
    string immLow = immBin.substr(7, 5);     // imm[4:0]
    string bin;
    bin += immHigh;                      // 7 bits
    bin += toBinaryString(rs2, 5);         // rs2 (5 bits)
    bin += toBinaryString(rs1, 5);         // rs1 (5 bits)
    bin += fmt.funct3;                   // funct3 (3 bits)
    bin += immLow;                       // 5 bits
    bin += fmt.opcode;                   // opcode (7 bits)
    return bin;
}

string encodeSBType(const string &operation, int rs1, int rs2, int imm) {
    instruction_format fmt = instruction_map[operation];
    uint32_t immVal = imm & 0x1FFF; // 13 bits immediate (including sign)
    string immBin = toBinaryString(immVal, 13);
    // RISC-V branch encoding: imm[12] | imm[10:5] | rs2 | rs1 | funct3 | imm[4:1] | imm[11] | opcode
    string imm12   = immBin.substr(0, 1);      // bit 12
    string imm10_5 = immBin.substr(2, 6);        // bits 10-5
    string imm4_1  = immBin.substr(8, 4);         // bits 4-1
    string imm11   = immBin.substr(1, 1);         // bit 11
    string bin;
    bin += imm12;
    bin += imm10_5;
    bin += toBinaryString(rs2, 5);
    bin += toBinaryString(rs1, 5);
    bin += fmt.funct3;
    bin += imm4_1;
    bin += imm11;
    bin += fmt.opcode;
    return bin;
}

string encodeUType(const string &operation, int rd, int imm) {
    instruction_format fmt = instruction_map[operation];
    string immBin = toBinaryString((uint32_t)(imm & 0xFFFFF), 20);
    string bin;
    bin += immBin;                       // imm[31:12] (20 bits)
    bin += toBinaryString(rd, 5);          // rd (5 bits)
    bin += fmt.opcode;                   // opcode (7 bits)
    return bin;
}

string encodeUJType(const string &operation, int rd, int imm) {
    instruction_format fmt = instruction_map[operation];
    uint32_t immVal = imm & 0x1FFFFF; // 21 bits immediate
    string immBin = toBinaryString(immVal, 21);
    // Rearrangement for JAL: imm[20] | imm[10:1] | imm[11] | imm[19:12]
    string bit20     = immBin.substr(0, 1);
    string bits19_12 = immBin.substr(1, 8);
    string bit11     = immBin.substr(9, 1);
    string bits10_1  = immBin.substr(10, 10);
    string rearranged = bit20 + bits10_1 + bit11 + bits19_12;
    string bin;
    bin += rearranged;
    bin += toBinaryString(rd, 5);
    bin += fmt.opcode;
    return bin;
}

// Main Assembler Routine
int main() {
    ifstream infile("input.asm");
    if (!infile.is_open()){
        cerr << "failed to open input.asm" << endl;
        return 1;
    }

    // First Pass: Read lines, assign addresses, and record labels.
    string line;
    while(getline(infile, line)){
        // Remove inline comments (everything after '#')
        size_t hashPos = line.find('#');
        if(hashPos != string::npos)
            line = line.substr(0, hashPos);
            
        line = trim(line);
        if(line.empty())
            continue;
        
        size_t colon_position = line.find(':');
        if(colon_position != string::npos){
            string label = trim(line.substr(0, colon_position));
            if(current_section == "text")
                symbolTable[label] = text_address;
            else
                symbolTable[label] = data_address;
     
            line = trim(line.substr(colon_position + 1));
            if(line.empty())
                continue;
        }

        // Process directives (lines starting with '.')
        if(line[0] == '.'){
            if(line.find(".asciz") == string::npos)
                line = replacecommaswithspaces(line);
            vector<string> tokens = split(line, ' ');
            string directive = tokens[0];
            if(directive == ".text"){
                current_section = "text";
            }
            else if(directive == ".data"){
                current_section = "data";
            }
            else if(directive == ".byte"){
                if(tokens.size() > 1){
                    int count = tokens.size() - 1;
                    data_address += count;
                }
            }
            else if(directive == ".half"){
                if(tokens.size() > 1) {
                    int count = tokens.size() - 1;
                    data_address += 2 * count;
                }
            }
            else if(directive == ".word") {
                if(tokens.size() > 1){
                    int count = tokens.size() - 1;
                    data_address += 4 * count;
                }
            }
            else if(directive == ".dword"){
                if(tokens.size() > 1){
                    int count = tokens.size() - 1;
                    data_address += 8 * count;
                }
            }
            else if(directive == ".asciz"){
                size_t first_quote = line.find("\"");
                size_t last_quote = line.rfind("\"");
                if(first_quote != string::npos && last_quote != string::npos && last_quote > first_quote) {
                    string str = line.substr(first_quote + 1, last_quote - first_quote - 1);
                    data_address += str.size() + 1;
                }
            }
            assembly_line_structure al;
            al.original = line;
            al.address = (current_section == "text") ? text_address : data_address;
            al.section = current_section;
            assembly_lines.push_back(al);
        }
        // Otherwise, treat as an instruction (or data value) line.
        else {
            if(current_section == "text")
                line = replacecommaswithspaces(line);
            assembly_line_structure al;
            al.original = line;
            al.section = current_section;
            if(current_section == "text"){
                al.address = text_address;
                text_address += 4;  // each instruction is 4 bytes
            }
            else{
                al.address = data_address;
                data_address += 4;  // assume 4-byte words for data if not a directive
            }
            assembly_lines.push_back(al);
        }
    }
    infile.close();

    // Second Pass: Validate and generate machine code only if there are no errors.
    vector<string> outputLines;
    vector<string> errorMessages;
    uint32_t current_data_address = 0x10000000; // start of data

    for(auto &al : assembly_lines){
        string original = al.original;
        string machineCode;
        string comment;
        
        // Process directives.
        if(original[0] == '.'){
            vector<string> tokens = split(original, ' ');
            if(tokens.empty())
                continue;
            string directive = tokens[0];
            if(directive == ".text" || directive == ".data"){
                continue;
            }
            else if(directive == ".byte"){
                for (size_t i = 1; i < tokens.size(); i++){
                    int byteVal;
                    try{
                        byteVal = parseImmediate(tokens[i]);
                    } catch(...){
                        errorMessages.push_back("Invalid immediate in .byte directive: " + original);
                        continue;
                    }
                    stringstream ss;
                    ss << "0x" << hex << current_data_address;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(2) << setfill('0') << (byteVal & 0xFF);
                    machineCode = ss.str();
                    outputLines.push_back(addrStr + " " + machineCode + " , " + directive + " " + tokens[i] + " # data");
                    current_data_address += 1;
                }
                continue;
            }
            else if(directive == ".half"){
                for(size_t i = 1; i < tokens.size(); i++){
                    int halfVal;
                    try{
                        halfVal = parseImmediate(tokens[i]);
                    } catch(...){
                        errorMessages.push_back("Invalid immediate in .half directive: " + original);
                        continue;
                    }
                    stringstream ss;
                    ss << "0x" << hex << current_data_address;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(4) << setfill('0') << (halfVal & 0xFFFF);
                    machineCode = ss.str();
                    outputLines.push_back(addrStr + " " + machineCode + " , " + directive + " " + tokens[i] + " # data");
                    current_data_address += 2;
                }
                continue;
            }
            else if(directive == ".word"){
                for(size_t i = 1; i < tokens.size(); i++){
                    int wordVal;
                    try{
                        wordVal = parseImmediate(tokens[i]);
                    }
                    catch(...){
                        errorMessages.push_back("Invalid immediate in .word directive: " + original);
                        continue;
                    }
                    stringstream ss;
                    ss << "0x" << hex << current_data_address;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(8) << setfill('0') << (wordVal & 0xFFFFFFFF);
                    machineCode = ss.str();
                    outputLines.push_back(addrStr + " " + machineCode + " , " + directive + " " + tokens[i] + " # data");
                    current_data_address += 4;
                }
                continue;
            }
            else if(directive == ".dword"){
                for(size_t i = 1; i < tokens.size(); i++){
                    uint64_t dwordVal;
                    try{
                        dwordVal = stoull(tokens[i], nullptr, 0);
                    } catch(...){
                        errorMessages.push_back("Invalid immediate in .dword directive: " + original);
                        continue;
                    }
                    stringstream ss;
                    ss << "0x" << hex << current_data_address;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(16) << setfill('0') << dwordVal;
                    machineCode = ss.str();
                    outputLines.push_back(addrStr + " " + machineCode + " , " + directive + " " + tokens[i] + " # data");
                    current_data_address += 8;
                }
                continue;
            }
            else if(directive == ".asciz"){
                size_t first_quote = original.find("\"");
                size_t last_quote = original.rfind("\"");
                if(first_quote == string::npos || last_quote == string::npos || last_quote <= first_quote) {
                    errorMessages.push_back("Invalid .asciz directive: missing quotes in: " + original);
                    continue;
                }
                string str = original.substr(first_quote + 1, last_quote - first_quote - 1);
                for(char c : str){
                    stringstream ss;
                    ss << "0x" << hex << current_data_address;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char)c;
                    machineCode = ss.str();
                    outputLines.push_back(addrStr + " " + machineCode + " , " + directive + " " + string(1, c) + " # data");
                    current_data_address++;
                }
                {
                    stringstream ss;
                    ss << "0x" << hex << current_data_address;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(2) << setfill('0') << 0;
                    machineCode = ss.str();
                    outputLines.push_back(addrStr + " " + machineCode + " , " + directive + " \\0" + " # data");
                    current_data_address++;
                }
                continue;
            }
            else{
                errorMessages.push_back("Unrecognized directive: " + original);
                continue;
            }
        }
        // Process instructions in text section.
        else if(al.section == "text"){
            string processed = replacecommaswithspaces(original);
            vector<string> tokens = split(processed, ' ');
            if(tokens.empty()) continue;
            string operation = tokens[0];
            string bin;
            if(instruction_map.find(operation) == instruction_map.end()){
                errorMessages.push_back("Unrecognized operation: " + original);
                continue;
            }
            string type = instruction_map[operation].type;
            if(type == "R") {
                if(tokens.size() < 4){
                    errorMessages.push_back("Invalid R-type: " + original);
                    continue;
                }
                int rd = register_to_int(tokens[1]);
                int rs1 = register_to_int(tokens[2]);
                int rs2 = register_to_int(tokens[3]);
                if(rd < 0 || rs1 < 0 || rs2 < 0) {
                    errorMessages.push_back("Invalid register in R-type: " + original);
                    continue;
                }
                bin = encode_R_format(operation, rd, rs1, rs2);
                comment = instruction_map[operation].opcode + "-" + instruction_map[operation].funct3 + "-" +
                          instruction_map[operation].funct7 + "-" + toBinaryString(rd, 5) + "-" +
                          toBinaryString(rs1, 5) + "-" + toBinaryString(rs2, 5) + "-NULL";
            }
            else if(type == "I"){
                if(tokens.size() < 4){
                    if((operation == "lb" || operation == "lh" || operation == "lw" || 
                        operation == "ld" || operation == "jalr") && tokens.size() == 3){
                        if(tokens[2].find('(') == string::npos){
                            errorMessages.push_back("Invalid load instruction, expected offset(rs1) format: " + original);
                            continue;
                        }
                    }
                    else{
                        errorMessages.push_back("Invalid arithmetic immediate instruction: " + original);
                        continue;
                    }
                }
                int rd = register_to_int(tokens[1]);
                int rs1, imm;
                if(operation == "lb" || operation == "lh" || operation == "lw" || 
                   operation == "ld" || operation == "jalr") {
                    if(tokens.size() == 3){
                        if(tokens[2].find('(') == string::npos){
                            errorMessages.push_back("Invalid load instruction, expected offset(rs1) format: " + original);
                            continue;
                        }
                        size_t pos1 = tokens[2].find('(');
                        size_t pos2 = tokens[2].find(')');
                        imm = parseImmediate(tokens[2].substr(0, pos1));
                        string rs1Str = tokens[2].substr(pos1 + 1, pos2 - pos1 - 1);
                        rs1 = register_to_int(rs1Str);
                    }
                    else if(tokens.size() >= 4) {
                        if(operation == "jalr"){
                            rs1 = register_to_int(tokens[2]);
                            imm = parseImmediate(tokens[3]);
                        }
                        else{
                            imm = parseImmediate(tokens[2]);
                            rs1 = register_to_int(tokens[3]);
                        }
                    }
                } else {
                    if(tokens[2].find('(') != string::npos) {
                        errorMessages.push_back("Invalid arithmetic immediate instruction, unexpected load format: " + original);
                        continue;
                    }
                    if(tokens.size() < 4) {
                        errorMessages.push_back("Invalid arithmetic immediate instruction: " + original);
                        continue;
                    }
                    rs1 = register_to_int(tokens[2]);
                    imm = parseImmediate(tokens[3]);
                }
                // Range check for 12-bit signed immediate (-2048 to 2047)
                if(imm < -2048 || imm > 2047){
                    errorMessages.push_back("Immediate out of range for I-type instruction: " + original);
                    continue;
                }
                if(rd < 0 || rs1 < 0) {
                    errorMessages.push_back("Invalid register in I-type: " + original);
                    continue;
                }
                bin = encode_I_format(operation, rd, rs1, imm);
                comment = instruction_map[operation].opcode + "-" + instruction_map[operation].funct3 + "-NULL-" +
                          toBinaryString(rd, 5) + "-" + toBinaryString(rs1, 5) + "-NULL-" +
                          toBinaryString(imm & 0xFFF, 12);
            }
            else if(type == "S") {
                if(tokens.size() < 3) {
                    errorMessages.push_back("Invalid S-type: " + original);
                    continue;
                }
                int rs2 = register_to_int(tokens[1]);
                int rs1, imm;
                if(tokens[2].find('(') != string::npos) {
                    size_t pos1 = tokens[2].find('(');
                    size_t pos2 = tokens[2].find(')');
                    imm = parseImmediate(tokens[2].substr(0, pos1));
                    string rs1Str = tokens[2].substr(pos1 + 1, pos2 - pos1 - 1);
                    rs1 = register_to_int(rs1Str);
                } else if(tokens.size() >= 4) {
                    imm = parseImmediate(tokens[2]);
                    rs1 = register_to_int(tokens[3]);
                } else {
                    errorMessages.push_back("Invalid S-type instruction format: " + original);
                    continue;
                }
                // Range check for 12-bit signed immediate (-2048 to 2047)
                if(imm < -2048 || imm > 2047){
                    errorMessages.push_back("Immediate out of range for S-type instruction: " + original);
                    continue;
                }
                if(rs1 < 0 || rs2 < 0) {
                    errorMessages.push_back("Invalid register in S-type: " + original);
                    continue;
                }
                bin = encodeSType(operation, rs1, rs2, imm);
                comment = instruction_map[operation].opcode + "-" + instruction_map[operation].funct3 + "-NULL-" +
                          toBinaryString(rs1, 5) + "-" + toBinaryString(rs2, 5) + "-" +
                          toBinaryString(imm & 0xFFF, 12);
            }
            else if(type == "SB") {
                if(tokens.size() < 4) {
                    errorMessages.push_back("Invalid SB-type: " + original);
                    continue;
                }
                int rs1 = register_to_int(tokens[1]);
                int rs2 = register_to_int(tokens[2]);
                int offset;
                if(symbolTable.find(tokens[3]) != symbolTable.end()){
                    int targetAddress = symbolTable[tokens[3]];
                    offset = targetAddress - al.address;
                } else {
                    offset = parseImmediate(tokens[3]);
                }
                // SB-type (branch) offset must be a multiple of 2.
                if(offset % 2 != 0){
                    errorMessages.push_back("Branch offset must be a multiple of 2 for SB-type instruction: " + original);
                    continue;
                }
                // Range check for branch offset (13-bit signed, effectively -4096 to 4094)
                if(offset < -4096 || offset > 4094){
                    errorMessages.push_back("Branch offset out of range for SB-type instruction: " + original);
                    continue;
                }
                if(rs1 < 0 || rs2 < 0) {
                    errorMessages.push_back("Invalid register in SB-type: " + original);
                    continue;
                }
                bin = encodeSBType(operation, rs1, rs2, offset);
                comment = instruction_map[operation].opcode + "-" + instruction_map[operation].funct3 + "-NULL-" +
                          toBinaryString(rs1, 5) + "-" + toBinaryString(rs2, 5) + "-" +
                          toBinaryString(offset & 0x1FFF, 13);
            }
            else if(type == "U") {
                if(tokens.size() < 3) {
                    errorMessages.push_back("Invalid U-type: " + original);
                    continue;
                }
                int rd = register_to_int(tokens[1]);
                int imm = parseImmediate(tokens[2]);
                // Range check for 20-bit immediate (-524288 to 524287)
                if(imm < -524288 || imm > 524287){
                    errorMessages.push_back("Immediate out of range for U-type instruction: " + original);
                    continue;
                }
                if(rd < 0) {
                    errorMessages.push_back("Invalid register in U-type: " + original);
                    continue;
                }
                bin = encodeUType(operation, rd, imm);
                comment = instruction_map[operation].opcode + "-NULL-NULL-" +
                          toBinaryString(rd, 5) + "-NULL-NULL-" +
                          toBinaryString(imm, 20);
            }
            else if(type == "UJ") {
                if(tokens.size() < 3) {
                    errorMessages.push_back("Invalid UJ-type: " + original);
                    continue;
                }
                int rd = register_to_int(tokens[1]);
                int offset;
                if(symbolTable.find(tokens[2]) != symbolTable.end()){
                    int targetAddress = symbolTable[tokens[2]];
                    offset = targetAddress - al.address;
                } else {
                    offset = parseImmediate(tokens[2]);
                }
                // Range check for 21-bit jump offset (-1048576 to 1048575)
                if(offset < -1048576 || offset > 1048575){
                    errorMessages.push_back("Offset out of range for UJ-type instruction: " + original);
                    continue;
                }
                if(rd < 0) {
                    errorMessages.push_back("Invalid register in UJ-type: " + original);
                    continue;
                }
                bin = encodeUJType(operation, rd, offset);
                comment = instruction_map[operation].opcode + "-NULL-NULL-" +
                          toBinaryString(rd, 5) + "-NULL-NULL-" +
                          toBinaryString(offset & 0x1FFFFF, 21);
            }
            else {
                errorMessages.push_back("Unknown instruction type for: " + operation);
                continue;
            }
            machineCode = binaryToHex(bin);
            stringstream ss;
            ss << "0x" << hex << al.address;
            outputLines.push_back(ss.str() + " " + machineCode + " , " + original + " # " + comment);
        }
    }

    // If any errors were detected, report them and abort the conversion.
    if(!errorMessages.empty()){
        for(const auto &err : errorMessages){
            cerr << err << "\n";
        }
        cerr << "Assembly failed due to errors. No machine code generated." << "\n";
        return 1;
    } else {
        ofstream outfile("output.mc");
        if(!outfile.is_open()){
            cerr << "Failed to open output.mc for writing" << "\n";
            return 1;
        }
        for(const auto &line : outputLines){
            outfile << line << "\n";
        }
        // Termination code for text segment.
        stringstream ss;
        ss << "0x" << hex << text_address;
        outfile << ss.str() << " " << "0x00000000" << " , TERMINATION" << "\n";
        outfile.close();
        cout << "Assembly complete. See output.mc" << "\n";
    }
    return 0;
}