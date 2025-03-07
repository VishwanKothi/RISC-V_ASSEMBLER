#include <bits/stdc++.h>
using namespace std;

string trim(const string &s) {
    size_t start=s.find_first_not_of(" \t\r\n");
    if(start==string::npos) return "";
    size_t end=s.find_last_not_of(" \t\r\n");
    return s.substr(start,end-start+1);
}

vector<string> split(const string &s,char delim) {
    vector<string> tokens;
    string token;
    istringstream iss(s);
    while(getline(iss,token,delim)) {
        token=trim(token);
        if(!token.empty())
            tokens.push_back(token);
    }
    return tokens;
}

string replaceCommasWithSpaces(const string &s) {
    string result=s;
    for(char &c : result){
        if(c==',')
            c=' ';
    }
    return result;
}

int registerToInt(const string &reg) {
    if(reg[0]=='x' || reg[0]=='X')
        return stoi(reg.substr(1));
    return -1;
}


string toBinaryString(uint32_t value, int width) {
    string result;
    for(int i=width-1;i>=0;i--) {
        result.push_back((value & (1u<<i)) ? '1' : '0');
    }
    return result;
}
string binaryToHex(const string &bin) {
    uint32_t num=0;
    for(char c : bin) {
        num=(num << 1)+(c-'0');
    }
    stringstream ss;
    ss << "0x" << hex << setw(8) << setfill('0') << num;
    return ss.str();
}

struct AsmLine {
    string original;
    uint32_t address;
    string section;   
};

vector<AsmLine> asmLines;

unordered_map<string, uint32_t> symbolTable;

uint32_t textAddress = 0x00000000;
uint32_t dataAddress = 0x10000000;

string currentSection = "text";

struct InstrFormat {
    string type;
    string opcode;
    string funct3;
    string funct7;
};

unordered_map<string, InstrFormat> instrMap = {

    {"add",{"R","0110011","000","0000000"}},
    {"and",{"R","0110011","111","0000000"}},
    {"or",{"R","0110011","110","0000000"}},
    {"sll",{"R","0110011","001","0000000"}},
    {"slt",{"R","0110011","010","0000000"}},
    {"sra",{"R","0110011","101","0100000"}},
    {"srl",{"R","0110011","101","0000000"}},
    {"sub",{"R", "0110011", "000", "0100000"}},
    {"xor",{"R", "0110011", "100", "0000000"}},
    {"mul",{"R", "0110011", "000", "0000001"}},
    {"div",{"R", "0110011", "100", "0000001"}},
    {"rem",{"R","0110011","110","0000001"}},

    {"addi",{"I","0010011","000","NULL"}},
    {"andi",{"I","0010011","111","NULL"}},
    {"ori",{"I","0010011","110","NULL"}},
    {"lb",{"I","0000011","000","NULL"}},
    {"lh",{"I","0000011","001","NULL"}},
    {"lw",{"I","0000011","010","NULL"}},
    {"ld",{"I","0000011","011","NULL"}},
    {"jalr",{"I","1100111","000","NULL"}},

    {"sb", {"S","0100011","000","NULL"}},
    {"sh", {"S","0100011","001","NULL"}},
    {"sw", {"S","0100011","010","NULL"}},
    {"sd", {"S","0100011","011","NULL"}},
 
    {"beq",{"SB","1100011","000","NULL"}},
    {"bne",{"SB","1100011","001","NULL"}},
    {"blt",{"SB","1100011","100","NULL"}},
    {"bge",{"SB","1100011","101","NULL"}},
 
    {"lui",{"U","0110111","NULL","NULL"}},
    {"auipc",{"U","0010111","NULL","NULL"}},
  
    {"jal",{"UJ","1101111","NULL","NULL"}}
};

string encodeRType(const string &mnemonic,int rd,int rs1,int rs2){
    InstrFormat fmt=instrMap[mnemonic];
    string bin;
    bin+=fmt.funct7;
    bin+=toBinaryString(rs2, 5);
    bin+=toBinaryString(rs1, 5);
    bin+=fmt.funct3;
    bin+=toBinaryString(rd, 5);
    bin+=fmt.opcode;
    return bin;
}

string encodeIType(const string &mnemonic,int rd,int rs1,int imm){
    InstrFormat fmt=instrMap[mnemonic];
    string immBin=toBinaryString((uint32_t)(imm & 0xFFF),12);
    string bin;
    bin+=immBin;
    bin+=toBinaryString(rs1,5);
    bin+=fmt.funct3;
    bin+=toBinaryString(rd,5);
    bin+=fmt.opcode;
    return bin;
}

string encodeSType(const string &mnemonic,int rs1,int rs2,int imm){
    InstrFormat fmt=instrMap[mnemonic];
    uint32_t immVal=imm & 0xFFF;
    string immBin=toBinaryString(immVal, 12);
    string immHigh=immBin.substr(0,7);
    string immLow=immBin.substr(7,5);
    string bin;
    bin+=immHigh;
    bin+=toBinaryString(rs2, 5);
    bin+=toBinaryString(rs1, 5);
    bin+=fmt.funct3;
    bin+=immLow; 
    bin+= fmt.opcode;
    return bin;
}

string encodeSBType(const string &mnemonic, int rs1, int rs2, int imm) {
    InstrFormat fmt = instrMap[mnemonic];
    uint32_t immVal = imm & 0x1FFF; // 13 bits immediate (including sign)
    string immBin = toBinaryString(immVal, 13);
    // RISC-V branch encoding: imm[12] | imm[10:5] | rs2 | rs1 | funct3 | imm[4:1] | imm[11] | opcode
    string imm12   = immBin.substr(0,1);      // bit 12
    string imm10_5 = immBin.substr(2,6);        // bits 10-5
    string imm4_1  = immBin.substr(8,4);         // bits 4-1
    string imm11   = immBin.substr(1,1);         // bit 11
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

string encodeUType(const string &mnemonic, int rd, int imm) {
    InstrFormat fmt = instrMap[mnemonic];
    string immBin = toBinaryString((uint32_t)(imm & 0xFFFFF), 20);
    string bin;
    bin += immBin;                       // imm[31:12] (20 bits)
    bin += toBinaryString(rd, 5);          // rd (5 bits)
    bin += fmt.opcode;                   // opcode (7 bits)
    return bin;
}

string encodeUJType(const string &mnemonic, int rd, int imm) {
    InstrFormat fmt = instrMap[mnemonic];
    uint32_t immVal = imm & 0x1FFFFF; // 21 bits immediate
    string immBin = toBinaryString(immVal, 21);
    // Rearrangement for JAL: imm[20] | imm[10:1] | imm[11] | imm[19:12]
    string bit20    = immBin.substr(0,1);
    string bits19_12= immBin.substr(1,8);
    string bit11    = immBin.substr(9,1);
    string bits10_1 = immBin.substr(10,10);
    string rearranged = bit20 + bits10_1 + bit11 + bits19_12;
    string bin;
    bin += rearranged;
    bin += toBinaryString(rd, 5);
    bin += fmt.opcode;
    return bin;
}

// ----- Main Assembler Routine -----
int main() {
    ifstream infile("input.asm");
    if (!infile.is_open()) {
        cerr << "Failed to open input.asm" << endl;
        return 1;
    }

    // First Pass: Read lines, assign addresses, and record labels.
    string line;
    while(getline(infile, line)) {
        line = trim(line);
        if(line.empty() || line[0] == '#')
            continue;


        size_t colonPos = line.find(':');
        if(colonPos != string::npos) {
            string label = trim(line.substr(0, colonPos));
            if(currentSection == "text")
                symbolTable[label] = textAddress;
            else
                symbolTable[label] = dataAddress;
            
            line = trim(line.substr(colonPos+1));
            if(line.empty())
                continue;
        }

        if(line[0] == '.') {
          
            if(line.find(".asciz") == string::npos)
                line = replaceCommasWithSpaces(line);
            vector<string> tokens = split(line, ' ');
            string directive = tokens[0];
            if(directive == ".text") {
                currentSection = "text";
            } else if(directive == ".data") {
                currentSection = "data";
            } else if(directive == ".byte") {
                if(tokens.size() > 1) {
                    int count = tokens.size() - 1;
                    dataAddress += count;
                }
            } else if(directive == ".half") {
                if(tokens.size() > 1) {
                    int count = tokens.size() - 1;
                    dataAddress += 2 * count;
                }
            } else if(directive == ".word") {
                if(tokens.size() > 1) {
                    int count = tokens.size() - 1;
                    dataAddress += 4 * count;
                }
            } else if(directive == ".dword") {
                if(tokens.size() > 1) {
                    int count = tokens.size() - 1;
                    dataAddress += 8 * count;
                }
            } else if(directive == ".asciz") {
                size_t firstQuote = line.find("\"");
                size_t lastQuote = line.rfind("\"");
                if(firstQuote != string::npos && lastQuote != string::npos && lastQuote > firstQuote) {
                    string str = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    dataAddress += str.size() + 1;
                }
            }
            AsmLine al;
            al.original = line;
            al.address = (currentSection == "text") ? textAddress : dataAddress;
            al.section = currentSection;
            asmLines.push_back(al);
        }
 
        else {
            if(currentSection == "text")
                line = replaceCommasWithSpaces(line);
            AsmLine al;
            al.original = line;
            al.section = currentSection;
            if(currentSection == "text") {
                al.address = textAddress;
                textAddress += 4;
            } else {
                al.address = dataAddress;
                dataAddress += 4;
            }
            asmLines.push_back(al);
        }
    }
    infile.close();

 
    uint32_t currentDataAddress = 0x10000000;
    ofstream outfile("output.mc");
    if (!outfile.is_open()) {
        cerr << "Failed to open output.mc for writing" << endl;
        return 1;
    }

    for(auto &al : asmLines){
        string original = al.original;
        string machineCode;
        string comment;
        
        if(original[0] == '.'){
            vector<string> tokens = split(original, ' ');
            string directive = tokens[0];
            if(directive == ".text" || directive == ".data") {
                continue;
            }
            else if(directive == ".byte") {
                for (size_t i = 1; i < tokens.size(); i++) {
                    int byteVal = stoi(tokens[i]);
                    stringstream ss;
                    ss << "0x" << hex << currentDataAddress;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(2) << setfill('0') << (byteVal & 0xFF);
                    machineCode = ss.str();
                    outfile << addrStr << " " << machineCode << " , " << directive << " " << tokens[i] << " # data" << "\n";
                    currentDataAddress += 1;
                }
                continue;
            }
            else if(directive == ".half") {
                for (size_t i = 1; i < tokens.size(); i++) {
                    int halfVal = stoi(tokens[i]);
                    stringstream ss;
                    ss << "0x" << hex << currentDataAddress;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(4) << setfill('0') << (halfVal & 0xFFFF);
                    machineCode = ss.str();
                    outfile << addrStr << " " << machineCode << " , " << directive << " " << tokens[i] << " # data" << "\n";
                    currentDataAddress += 2;
                }
                continue;
            }
            else if(directive == ".word") {
                for (size_t i = 1; i < tokens.size(); i++) {
                    int wordVal = stoi(tokens[i]);
                    stringstream ss;
                    ss << "0x" << hex << currentDataAddress;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(8) << setfill('0') << (wordVal & 0xFFFFFFFF);
                    machineCode = ss.str();
                    outfile << addrStr << " " << machineCode << " , " << directive << " " << tokens[i] << " # data" << "\n";
                    currentDataAddress += 4;
                }
                continue;
            }
            else if(directive == ".dword") {
                for (size_t i = 1; i < tokens.size(); i++) {
                    uint64_t dwordVal = stoull(tokens[i]);
                    stringstream ss;
                    ss << "0x" << hex << currentDataAddress;
                    string addrStr = ss.str();
                    ss.str("");
                    ss << "0x" << hex << setw(16) << setfill('0') << dwordVal;
                    machineCode = ss.str();
                    outfile << addrStr << " " << machineCode << " , " << directive << " " << tokens[i] << " # data" << "\n";
                    currentDataAddress += 8;
                }
                continue;
            }
            else if(directive == ".asciz") {
                size_t firstQuote = original.find("\"");
                size_t lastQuote = original.rfind("\"");
                if(firstQuote != string::npos && lastQuote != string::npos && lastQuote > firstQuote) {
                    string str = original.substr(firstQuote+1, lastQuote-firstQuote-1);
                    for (char c : str) {
                        stringstream ss;
                        ss << "0x" << hex << currentDataAddress;
                        string addrStr = ss.str();
                        ss.str("");
                        ss << "0x" << hex << setw(2) << setfill('0') << (int)(unsigned char)c;
                        machineCode = ss.str();
                        outfile << addrStr << " " << machineCode << " , " << directive << " " << c << " # data" << "\n";
                        currentDataAddress++;
                    }

                    {
                        stringstream ss;
                        ss << "0x" << hex << currentDataAddress;
                        string addrStr = ss.str();
                        ss.str("");
                        ss << "0x" << hex << setw(2) << setfill('0') << 0;
                        machineCode = ss.str();
                        outfile << addrStr << " " << machineCode << " , " << directive << " \\0" << " # data" << "\n";
                        currentDataAddress++;
                    }
                }
                continue;
            }
        }
  
        else if(al.section == "text") {
            string processed = replaceCommasWithSpaces(original);
            vector<string> tokens = split(processed, ' ');
            if(tokens.empty()) continue;
            string mnemonic = tokens[0];
            string bin;
            if(instrMap.find(mnemonic) != instrMap.end()) {
                string type = instrMap[mnemonic].type;
                if(type == "R") {
                    if(tokens.size() < 4) {
                        cerr << "Invalid R-type: " << original << endl;
                        continue;
                    }
                    int rd  = registerToInt(tokens[1]);
                    int rs1 = registerToInt(tokens[2]);
                    int rs2 = registerToInt(tokens[3]);
                    bin = encodeRType(mnemonic, rd, rs1, rs2);
                    comment = instrMap[mnemonic].opcode + "-" + instrMap[mnemonic].funct3 + "-" +
                              instrMap[mnemonic].funct7 + "-" + toBinaryString(rd, 5) + "-" +
                              toBinaryString(rs1, 5) + "-" + toBinaryString(rs2, 5) + "-NULL";
                }
                else if(type == "I") {
                    if(tokens.size() < 4) {
                        if(tokens.size() < 3) {
                            cerr << "Invalid I-type: " << original << endl;
                            continue;
                        }
                    }
                    int rd  = registerToInt(tokens[1]);
                    int rs1, imm;
                    if(tokens[2].find('(') != string::npos) {
                        size_t pos1 = tokens[2].find('(');
                        size_t pos2 = tokens[2].find(')');
                        imm = stoi(tokens[2].substr(0, pos1));
                        string rs1Str = tokens[2].substr(pos1 + 1, pos2 - pos1 - 1);
                        rs1 = registerToInt(rs1Str);
                    } else {
                        rs1 = registerToInt(tokens[2]);
                        imm = stoi(tokens[3]);
                    }
                    bin = encodeIType(mnemonic, rd, rs1, imm);
                    comment = instrMap[mnemonic].opcode + "-" + instrMap[mnemonic].funct3 + "-NULL-" +
                              toBinaryString(rd, 5) + "-" + toBinaryString(rs1, 5) + "-NULL-" +
                              toBinaryString(imm & 0xFFF, 12);
                }
                else if(type == "S") {
                    if(tokens.size() < 3) {
                        cerr << "Invalid S-type: " << original << endl;
                        continue;
                    }
                    int rs2 = registerToInt(tokens[1]);
                    size_t pos1 = tokens[2].find('(');
                    size_t pos2 = tokens[2].find(')');
                    if(pos1 == string::npos || pos2 == string::npos) {
                        cerr << "Invalid addressing: " << tokens[2] << endl;
                        continue;
                    }
                    int imm = stoi(tokens[2].substr(0, pos1));
                    string rs1Str = tokens[2].substr(pos1+1, pos2-pos1-1);
                    int rs1 = registerToInt(rs1Str);
                    bin = encodeSType(mnemonic, rs1, rs2, imm);
                    comment = instrMap[mnemonic].opcode + "-" + instrMap[mnemonic].funct3 + "-NULL-" +
                              toBinaryString(rs1, 5) + "-" + toBinaryString(rs2, 5) + "-" +
                              toBinaryString(imm & 0xFFF, 12);
                }
                else if(type == "SB") {
                    if(tokens.size() < 4) {
                        cerr << "Invalid SB-type: " << original << endl;
                        continue;
                    }
                    int rs1 = registerToInt(tokens[1]);
                    int rs2 = registerToInt(tokens[2]);
                    string label = tokens[3];
                    int targetAddress = symbolTable[label];
                    int offset = targetAddress - al.address;
                    bin = encodeSBType(mnemonic, rs1, rs2, offset);
                    comment = instrMap[mnemonic].opcode + "-" + instrMap[mnemonic].funct3 + "-NULL-" +
                              toBinaryString(rs1, 5) + "-" + toBinaryString(rs2, 5) + "-" +
                              toBinaryString(offset & 0x1FFF, 13);
                }
                else if(type == "U") {
                    if(tokens.size() < 3) {
                        cerr << "Invalid U-type: " << original << endl;
                        continue;
                    }
                    int rd = registerToInt(tokens[1]);
                    int imm = stoi(tokens[2], nullptr, 0);
                    bin = encodeUType(mnemonic, rd, imm);
                    comment = instrMap[mnemonic].opcode + "-NULL-NULL-" +
                              toBinaryString(rd, 5) + "-NULL-NULL-" +
                              toBinaryString(imm, 20);
                }
                else if(type == "UJ") {
                    if(tokens.size() < 3) {
                        cerr << "Invalid UJ-type: " << original << endl;
                        continue;
                    }
                    int rd = registerToInt(tokens[1]);
                    string label = tokens[2];
                    int targetAddress = symbolTable[label];
                    int offset = targetAddress - al.address;
                    bin = encodeUJType(mnemonic, rd, offset);
                    comment = instrMap[mnemonic].opcode + "-NULL-NULL-" +
                              toBinaryString(rd, 5) + "-NULL-NULL-" +
                              toBinaryString(offset & 0x1FFFFF, 21);
                }
                else {
                    cerr << "Unknown instruction type for: " << mnemonic << endl;
                    continue;
                }
                machineCode = binaryToHex(bin);
            }
            else{
                continue;
            }
            stringstream ss;
            ss << "0x" << hex << al.address;
            outfile << ss.str() << " " << machineCode << " , " << original << " # " << comment << "\n";
        }
    }

    {
        stringstream ss;
        ss << "0x" << hex << textAddress;
        outfile << ss.str() << " " << "0x00000000" << " , TERMINATION" << "\n";
    }
    outfile.close();
    cout << "Assembly complete. See output.mc" << endl;
    return 0;
}
