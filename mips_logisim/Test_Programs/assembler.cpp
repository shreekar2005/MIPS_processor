#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <sstream>
#include <bitset>
#include <algorithm>
#include <cctype>
#include <iomanip>

using namespace std;

// Structs to hold detailed information for output files
struct InstructionDetail {
    string originalLine;
    string binary;
    string hex;
    int address;
};

struct DataDetail {
    string label;
    int value;
    int address;
};

// --- MIPS ISA Definition for the specified processor ---
set<string> supported_instructions = {
    "add", "sub", "and", "or", "slt",       // R-Type
    "lw", "sw", "beq", "addi",              // I-Type
    "j",                                   // J-Type
    "move", "subi"                         // Pseudo-Instructions
};

set<string> r_type_instructions = {"add", "sub", "and", "or", "slt"};
set<string> i_type_instructions = {"lw", "sw", "beq", "addi"};
set<string> j_type_instructions = {"j"};
set<string> pseudo_instructions = {"move", "subi"};

unordered_map<string, string> instruction_to_op = {
    // I-Type Opcodes
    {"addi", "001000"},
    {"lw",   "100011"}, {"sw",   "101011"}, {"beq",  "000100"},
    // J-Type Opcodes
    {"j",    "000010"},
    // R-Type all have opcode 000000
    {"add", "000000"}, {"sub", "000000"}, {"and", "000000"}, {"or", "000000"}, {"slt", "000000"},
};

unordered_map<string, string> r_type_to_funct = {
    {"add", "100000"}, {"sub", "100010"}, {"and", "100100"},
    {"or",  "100101"}, {"slt", "101010"}
};

unordered_map<string, string> register_to_binary = {
    {"$zero", "00000"}, {"$at", "00001"}, {"$v0", "00010"}, {"$v1", "00011"},
    {"$a0", "00100"}, {"$a1", "00101"}, {"$a2", "00110"}, {"$a3", "00111"},
    {"$t0", "01000"}, {"$t1", "01001"}, {"$t2", "01010"}, {"$t3", "01011"},
    {"$t4", "01100"}, {"$t5", "01101"}, {"$t6", "01110"}, {"$t7", "01111"},
    {"$s0", "10000"}, {"$s1", "10001"}, {"$s2", "10010"}, {"$s3", "10011"},
    {"$s4", "10100"}, {"$s5", "10101"}, {"$s6", "10110"}, {"$s7", "10111"},
    {"$t8", "11000"}, {"$t9", "11001"}, {"$k0", "11010"}, {"$k1", "11011"},
    {"$gp", "11100"}, {"$sp", "11101"}, {"$fp", "11110"}, {"$ra", "11111"}
};

// --- Function Prototypes ---
void trim(string& str);
string binaryToHex(const string& bin);
void exitWithError(const string& message, int lineNum, const string& line);

string convert_r_to_binary(const string& line, int lineNum);
string convert_i_to_binary(const string& line, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, int current_pc, int lineNum);
string convert_j_to_binary(const string& line, const unordered_map<string, int>& textLabels, int lineNum);
vector<string> convert_pseudo_to_binary(const string& line, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, int current_pc, int lineNum);

void writeInstructionMemoryFile(const vector<InstructionDetail>& instructions, const string& dir);
void writeDataMemoryFile(const vector<DataDetail>& data, const string& dir);
void writeAboutFile(const vector<InstructionDetail>& instructions, const vector<DataDetail>& data, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, const string& dir);


// --- Main Assembler Logic ---
int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <dir_containing_main.asm>" << endl;
        return 1;
    }
    string input_dir = argv[1];
    ifstream inputFile(input_dir + "/main.asm");
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open file '" << input_dir << "/main.asm'" << endl;
        return 1;
    }

    // --- PASS 1: Build Symbol Tables ---
    unordered_map<string, int> dataSegLabels;
    unordered_map<string, int> textSegLabels;
    vector<DataDetail> dataMemoryLayout;
    string line;
    string currentSection = "";
    int dataAddr = 0;
    int textAddr = 0;
    int lineNum = 0;
    string lastDataLabel = "";

    while (getline(inputFile, line)) {
        lineNum++;
        trim(line);
        if (line.empty()) continue;

        if (line == ".data") {
            currentSection = "data";
            continue;
        }
        if (line == ".text") {
            currentSection = "text";
            continue;
        }

        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string label = line.substr(0, colonPos);
            trim(label);
            if (currentSection == "data") {
                dataSegLabels[label] = dataAddr;
                lastDataLabel = label;
            } else if (currentSection == "text") {
                textSegLabels[label] = textAddr;
            }
            line = line.substr(colonPos + 1);
            trim(line);
            if (line.empty()) continue;
        }

        stringstream ss(line);
        string firstWord;
        ss >> firstWord;

        if (currentSection == "data") {
            if (firstWord == ".word") {
                string valueStr;
                ss >> valueStr;
                try {
                    dataMemoryLayout.push_back({lastDataLabel, stoi(valueStr), dataAddr});
                    dataAddr += 4;
                } catch (const std::invalid_argument& e) {
                    exitWithError("Invalid number for .word directive", lineNum, line);
                }
            }
        } else if (currentSection == "text") {
            if (supported_instructions.count(firstWord)) {
                textAddr += 4;
            } else {
                exitWithError("Unsupported instruction '" + firstWord + "'", lineNum, line);
            }
        }
    }

    // --- PASS 2: Generate Machine Code ---
    inputFile.clear();
    inputFile.seekg(0, ios::beg);
    currentSection = "";
    textAddr = 0;
    lineNum = 0;
    vector<InstructionDetail> instructionMemoryLayout;

    while (getline(inputFile, line)) {
        lineNum++;
        string original_line_for_output = line;
        trim(line);
        if (line.empty()) continue;

        if (line == ".data" || line == ".text") {
            currentSection = line.substr(1);
            continue;
        }
        
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            line = line.substr(colonPos + 1);
            trim(line);
            if (line.empty()) continue;
        }
        
        if (currentSection == "text") {
            stringstream ss(line);
            string firstWord;
            ss >> firstWord;

            if (!supported_instructions.count(firstWord)) continue;
            
            vector<string> binary_codes;
            if (pseudo_instructions.count(firstWord)) {
                binary_codes = convert_pseudo_to_binary(line, textSegLabels, dataSegLabels, textAddr, lineNum);
            } else if (r_type_instructions.count(firstWord)) {
                binary_codes.push_back(convert_r_to_binary(line, lineNum));
            } else if (i_type_instructions.count(firstWord)) {
                binary_codes.push_back(convert_i_to_binary(line, textSegLabels, dataSegLabels, textAddr, lineNum));
            } else if (j_type_instructions.count(firstWord)) {
                binary_codes.push_back(convert_j_to_binary(line, textSegLabels, lineNum));
            }

            for(const auto& code : binary_codes) {
                instructionMemoryLayout.push_back({original_line_for_output, code, binaryToHex(code), textAddr});
                textAddr += 4;
            }
        }
    }

    // --- Step 3: Write Output Files ---
    cout << "Assembly complete. Writing output files..." << endl;
    writeInstructionMemoryFile(instructionMemoryLayout, input_dir);
    writeDataMemoryFile(dataMemoryLayout, input_dir);
    writeAboutFile(instructionMemoryLayout, dataMemoryLayout, textSegLabels, dataSegLabels, input_dir);
    cout << "Output files generated successfully in '" << input_dir << "'." << endl;

    inputFile.close();
    return 0;
}


// --- Helper and Conversion Functions ---

void trim(string& str) {
    str.erase(0, str.find_first_not_of(" \t\n\r"));
    str.erase(str.find_last_not_of(" \t\n\r") + 1);
    size_t commentPos = str.find('#');
    if (commentPos != string::npos) {
        str.erase(commentPos);
    }
    str.erase(str.find_last_not_of(" \t\n\r") + 1);
}

void exitWithError(const string& message, int lineNum, const string& line) {
    cerr << "ASSEMBLY ERROR (Line " << lineNum << "): " << message << endl;
    cerr << "  > " << line << endl;
    exit(1);
}

string binaryToHex(const string& bin) {
    if (bin.length() != 32) return "Error-Length";
    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < 32; i += 4) {
        bitset<4> bits(bin.substr(i, 4));
        ss << setw(1) << bits.to_ulong();
    }
    return ss.str();
}

string convert_r_to_binary(const string& line, int lineNum) {
    stringstream ss(line);
    string op, p1, p2, p3;
    ss >> op >> p1 >> p2 >> p3;
    
    if (ss.rdbuf()->in_avail() != 0) {
         exitWithError("Too many arguments for R-Type instruction", lineNum, line);
    }

    auto clean_reg = [](string& reg){
        size_t comma = reg.find(',');
        if(comma != string::npos) reg.erase(comma);
    };
    clean_reg(p1); clean_reg(p2); clean_reg(p3);

    if (!register_to_binary.count(p1) || !register_to_binary.count(p2) || !register_to_binary.count(p3)) {
        exitWithError("Invalid register name", lineNum, line);
    }

    string rd_bin = register_to_binary.at(p1);
    string rs_bin = register_to_binary.at(p2);
    string rt_bin = register_to_binary.at(p3);
    string shamt_bin = "00000";

    return instruction_to_op.at(op) + rs_bin + rt_bin + rd_bin + shamt_bin + r_type_to_funct.at(op);
}

string convert_i_to_binary(const string& line, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, int current_pc, int lineNum) {
    stringstream ss(line);
    string op, p1, p2, p3;
    ss >> op >> p1 >> p2 >> p3;
    
    if (op != "lw" && op != "sw" && ss.rdbuf()->in_avail() != 0) {
         exitWithError("Too many arguments for I-Type instruction", lineNum, line);
    }

    string rs_bin, rt_bin, imm_bin;
    
    auto clean_reg = [](string& reg){
        size_t comma = reg.find(',');
        if(comma != string::npos) reg.erase(comma);
    };
    clean_reg(p1); clean_reg(p2);

    if (op == "addi") {
        if (!register_to_binary.count(p1) || !register_to_binary.count(p2)) {
            exitWithError("Invalid register name", lineNum, line);
        }
        rt_bin = register_to_binary.at(p1);
        rs_bin = register_to_binary.at(p2);
        int imm = 0;
        try {
            imm = stoi(p3);
        } catch(...) {
            exitWithError("Immediate value must be an integer", lineNum, line);
        }
        imm_bin = bitset<16>(imm).to_string();
    } else if (op == "lw" || op == "sw") {
        if (!register_to_binary.count(p1)) {
            exitWithError("Invalid register name for rt", lineNum, line);
        }
        rt_bin = register_to_binary.at(p1);
        
        size_t open_paren = p2.find('(');
        if (open_paren != string::npos) { // Format is offset(register)
            size_t close_paren = p2.find(')');
            if (close_paren == string::npos || close_paren < open_paren) {
                exitWithError("Invalid format for lw/sw. Mismatched parentheses.", lineNum, line);
            }
            int offset = 0;
            try {
                if (open_paren > 0) offset = stoi(p2.substr(0, open_paren));
            } catch(...) {
                exitWithError("Invalid offset in lw/sw", lineNum, line);
            }
            string rs_reg = p2.substr(open_paren + 1, close_paren - open_paren - 1);
            if (!register_to_binary.count(rs_reg)) {
                exitWithError("Invalid base register in lw/sw", lineNum, line);
            }
            rs_bin = register_to_binary.at(rs_reg);
            imm_bin = bitset<16>(offset).to_string();
        } else { // Format is label
            if (!dataLabels.count(p2)) {
                exitWithError("Undefined data label '" + p2 + "'", lineNum, line);
            }
            rs_bin = register_to_binary.at("$zero");
            imm_bin = bitset<16>(dataLabels.at(p2)).to_string();
        }
    } else if (op == "beq") {
        if (!register_to_binary.count(p1) || !register_to_binary.count(p2)) {
            exitWithError("Invalid register name", lineNum, line);
        }
        rs_bin = register_to_binary.at(p1);
        rt_bin = register_to_binary.at(p2);
        if (!textLabels.count(p3)) {
            exitWithError("Undefined label '" + p3 + "'", lineNum, line);
        }
        int target_addr = textLabels.at(p3);
        int offset = (target_addr - (current_pc + 4)) / 4;
        imm_bin = bitset<16>(offset).to_string();
    }
    
    return instruction_to_op.at(op) + rs_bin + rt_bin + imm_bin;
}

string convert_j_to_binary(const string& line, const unordered_map<string, int>& textLabels, int lineNum) {
    stringstream ss(line);
    string op, label;
    ss >> op >> label;

    if (ss.rdbuf()->in_avail() != 0) {
         exitWithError("Too many arguments for J-Type instruction", lineNum, line);
    }

    if (!textLabels.count(label)) {
        exitWithError("Undefined label '" + label + "'", lineNum, line);
    }
    int address = textLabels.at(label);
    int jump_target = (address & 0x0FFFFFFF) / 4;
    return instruction_to_op.at(op) + bitset<26>(jump_target).to_string();
}

vector<string> convert_pseudo_to_binary(const string& line, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, int current_pc, int lineNum) {
    stringstream ss(line);
    string op, p1, p2, p3;
    ss >> op >> p1 >> p2;

    vector<string> instructions;
    
    auto clean_reg = [](string& reg){
        size_t comma = reg.find(',');
        if(comma != string::npos) reg.erase(comma);
    };
    clean_reg(p1);

    if (op == "move") {
        if (ss.rdbuf()->in_avail() != 0) {
            exitWithError("Too many arguments for move instruction. Use: move $rd, $rs", lineNum, line);
        }
        clean_reg(p2);
        string add_line = "add " + p1 + ", " + p2 + ", $zero";
        instructions.push_back(convert_r_to_binary(add_line, lineNum));
    } else if (op == "subi") {
        ss >> p3;
        if (ss.rdbuf()->in_avail() != 0) {
            exitWithError("Too many arguments for subi. Use: subi $rt, $rs, imm", lineNum, line);
        }
        clean_reg(p2);
        int imm = 0;
        try {
            imm = stoi(p3);
        } catch(...) {
            exitWithError("Immediate value for subi must be an integer", lineNum, line);
        }
        string addi_line = "addi " + p1 + ", " + p2 + ", " + to_string(-imm);
        instructions.push_back(convert_i_to_binary(addi_line, textLabels, dataLabels, current_pc, lineNum));
    }
    return instructions;
}

void writeInstructionMemoryFile(const vector<InstructionDetail>& instructions, const string& dir) {
    ofstream outfile(dir + "/instruction_memory.hex");
    if (!outfile.is_open()) {
        cerr << "Error: Could not create instruction_memory.hex" << endl;
        return;
    }
    outfile << "v2.0 raw" << endl;
    for (const auto& instr : instructions) {
        outfile << instr.hex << endl;
    }
    outfile.close();
}

void writeDataMemoryFile(const vector<DataDetail>& data, const string& dir) {
    ofstream outfile(dir + "/data_memory.hex");
    if (!outfile.is_open()) {
        cerr << "Error: Could not create data_memory.hex" << endl;
        return;
    }
    outfile << "v2.0 raw" << endl;
    for (const auto& item : data) {
        outfile << hex << setfill('0') << setw(8) << item.value << endl;
    }
    outfile.close();
}

void writeAboutFile(const vector<InstructionDetail>& instructions, const vector<DataDetail>& data, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, const string& dir) {
    ofstream outfile(dir + "/about_program.txt");
    if (!outfile.is_open()) {
        cerr << "Error: Could not create about_program.txt" << endl;
        return;
    }

    outfile << "--- LABEL & ADDRESS MAPPINGS ---" << endl << endl;

    outfile << "# Text Section Labels:" << endl;
    outfile << "# " << left << setw(20) << "Label Name" << "Hex Address" << endl;
    outfile << "#" << string(40, '-') << endl;
    for (const auto& pair : textLabels) {
        stringstream addr_ss;
        addr_ss << "0x" << hex << setfill('0') << setw(8) << pair.second;
        outfile << "# " << left << setw(20) << pair.first << addr_ss.str() << endl;
    }
    outfile << endl;

    outfile << "# Data Section Labels:" << endl;
    outfile << "# " << left << setw(20) << "Label Name" << "Hex Address" << endl;
    outfile << "#" << string(40, '-') << endl;
    for (const auto& pair : dataLabels) {
        stringstream addr_ss;
        addr_ss << "0x" << hex << setfill('0') << setw(8) << pair.second;
        outfile << "# " << left << setw(20) << pair.first << addr_ss.str() << endl;
    }
    outfile << endl << endl;


    outfile << "--- TEXT SECTION (Instructions) ---" << endl << endl;
    outfile << left
            << setw(14) << "Address (PC)"
            << setw(35) << "Original Instruction"
            << "Hex Code" << endl;
    outfile << string(80, '-') << endl;

    for (const auto& instr : instructions) {
        stringstream addr_ss;
        addr_ss << "0x" << hex << setfill('0') << setw(8) << instr.address;
        
        string trimmed_line = instr.originalLine;
        trim(trimmed_line);

        outfile << left
                << setw(14) << addr_ss.str()
                << setw(35) << trimmed_line
                << instr.hex << endl;
    }

    outfile << endl << endl << "--- DATA SECTION ---" << endl << endl;
    outfile << left
            << setw(20) << "Memory Address"
            << setw(18) << "Hex Value"
            << setw(18) << "Decimal Value"
            << "Label" << endl;
    outfile << string(80, '-') << endl;

    for (const auto& item : data) {
        stringstream addr_ss, hex_ss;
        addr_ss << "0x" << hex << setfill('0') << setw(8) << item.address;
        hex_ss << "0x" << hex << setfill('0') << setw(8) << item.value;

        outfile << left
                << setw(20) << addr_ss.str()
                << setw(18) << hex_ss.str()
                << setw(18) << dec << item.value
                << item.label << endl;
    }

    outfile.close();
}

