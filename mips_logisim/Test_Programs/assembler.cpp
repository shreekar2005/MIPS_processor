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
#include <tuple>

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

set<string> basic_mips_instructions = {
    "add", "sub", "and", "or", "slt", "nor", // R-Type ALU
    "lw", "sw", "beq", "j", // Core instructions from user's CPU
    "addi", "move", "sll", "bne", "jal", "jr", "syscall", "lui", "ori", "li", "la" // Other standard instructions
};
set<string> r_type_instructions = {"add", "sub", "and", "or", "slt", "nor", "sll", "jr", "syscall"};
set<string> i_type_instructions = {"addi", "lw", "sw", "beq", "bne", "lui", "ori"};
set<string> j_type_instructions = {"j", "jal"};
set<string> pseudo_instructions = {"move", "li", "la"};


unordered_map<string, string> instruction_to_op = {
    {"addi", "001000"}, {"lw", "100011"}, {"sw", "101011"}, {"beq", "000100"},
    {"bne", "000101"}, {"lui", "001111"}, {"ori", "001101"}, {"j", "000010"},
    {"jal", "000011"},
    // All R-Types have a 000000 opcode
    {"add", "000000"}, {"sub", "000000"}, {"and", "000000"}, {"or", "000000"},
    {"slt", "000000"}, {"nor", "000000"}, {"sll", "000000"}, {"jr", "000000"},
    {"syscall", "000000"},
};

unordered_map<string, string> r_type_to_funct = {
    {"add", "100000"}, {"sub", "100010"}, {"and", "100100"}, {"or", "100101"},
    {"slt", "101010"}, {"nor", "100111"}, {"sll", "000000"}, {"jr", "001000"},
    {"syscall", "001100"}
};

// --- MIPS Register Map ---
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

string convert_r_to_binary(const string& line);
string convert_i_to_binary(const string& line, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, int current_pc);
string convert_j_to_binary(const string& line, const unordered_map<string, int>& textLabels);
vector<string> convert_pseudo_to_binary(const string& line, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, int current_pc);

void writeInstructionMemoryFile(const vector<InstructionDetail>& instructions, const string& dir);
void writeDataMemoryFile(const vector<DataDetail>& data, const string& dir);
void writeAboutFile(const vector<InstructionDetail>& instructions, const vector<DataDetail>& data, const unordered_map<string, int>& textLabels, const string& dir);


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
    cout << "--- Starting Pass 1: Symbol Table Generation ---" << endl;
    unordered_map<string, int> dataSegLabels;
    unordered_map<string, int> textSegLabels;
    vector<DataDetail> dataMemoryLayout;
    string line;
    string currentSection = "";
    int dataAddr = 0;
    int textAddr = 0;
    string lastDataLabel = ""; // Variable to hold the last seen data label

    while (getline(inputFile, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line == ".data") {
            currentSection = "data";
            continue;
        }
        if (line == ".text") {
            currentSection = "text";
            continue;
        }

        // Handle labels
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string label = line.substr(0, colonPos);
            trim(label);
            if (currentSection == "data") {
                dataSegLabels[label] = dataAddr;
                lastDataLabel = label; // Store the most recent data label
            }
            else if (currentSection == "text") {
                textSegLabels[label] = textAddr;
            }
            line = line.substr(colonPos + 1);
            trim(line);
            if(line.empty()) continue;
        }

        stringstream ss(line);
        string firstWord;
        ss >> firstWord;

        if (currentSection == "data") {
            if (firstWord == ".word") {
                string valueStr;
                ss >> valueStr;
                dataMemoryLayout.push_back({lastDataLabel, stoi(valueStr), dataAddr});
                dataAddr += 4;
            }
        } else if (currentSection == "text") {
            if (pseudo_instructions.count(firstWord)) {
                 if (firstWord == "li" || firstWord == "la") {
                    textAddr += 8; // Expands to two instructions (lui, ori)
                } else {
                    textAddr += 4; // Expands to one instruction
                }
            } else if (basic_mips_instructions.count(firstWord)) {
                textAddr += 4;
            }
        }
    }
    cout << "--- Pass 1 Complete ---" << endl << endl;

    // --- PASS 2: Generate Machine Code ---
    cout << "--- Starting Pass 2: Code Generation ---" << endl;
    inputFile.clear();
    inputFile.seekg(0, ios::beg);
    currentSection = "";
    textAddr = 0;
    vector<InstructionDetail> instructionMemoryLayout;


    while (getline(inputFile, line)) {
        string original_line_for_output = line;
        trim(line);
        if (line.empty() || line[0] == '#') continue;

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
            original_line_for_output = line; // Keep the label for the about file
            line = line.substr(colonPos + 1);
            trim(line);
            if (line.empty()) continue;
        } else {
             // For the about file, let's keep the original indentation
            string temp = original_line_for_output;
            trim(temp);
            original_line_for_output = temp;
        }
        
        if (currentSection == "text") {
            stringstream ss(line);
            string firstWord;
            ss >> firstWord;

            if (!basic_mips_instructions.count(firstWord)) continue;

            cout << "Assembling: " << line << endl;
            vector<string> binary_codes;
             if (pseudo_instructions.count(firstWord)) {
                binary_codes = convert_pseudo_to_binary(line, textSegLabels, dataSegLabels, textAddr);
            } else if (r_type_instructions.count(firstWord)) {
                binary_codes.push_back(convert_r_to_binary(line));
            } else if (i_type_instructions.count(firstWord)) {
                binary_codes.push_back(convert_i_to_binary(line, textSegLabels, dataSegLabels, textAddr));
            } else if (j_type_instructions.count(firstWord)) {
                binary_codes.push_back(convert_j_to_binary(line, textSegLabels));
            }

            for(const auto& code : binary_codes){
                instructionMemoryLayout.push_back({original_line_for_output, code, binaryToHex(code), textAddr});
                textAddr += 4;
            }
        }
    }
    cout << "--- Pass 2 Complete ---" << endl << endl;

    // --- Step 3: Write Output Files ---
    cout << "--- Writing output files ---" << endl;
    writeInstructionMemoryFile(instructionMemoryLayout, input_dir);
    writeDataMemoryFile(dataMemoryLayout, input_dir);
    writeAboutFile(instructionMemoryLayout, dataMemoryLayout, textSegLabels, input_dir);
    cout << "--- Assembly complete. Output files generated in '" << input_dir << "' ---" << endl;

    inputFile.close();
    return 0;
}


// --- Helper and Conversion Functions ---

void trim(string& str) {
    // Trim leading and trailing whitespace
    str.erase(0, str.find_first_not_of(" \t\n\r"));
    str.erase(str.find_last_not_of(" \t\n\r") + 1);
    // Remove comments
    size_t commentPos = str.find('#');
    if (commentPos != string::npos) {
        str.erase(commentPos);
    }
    // Trim again after removing comments
    str.erase(str.find_last_not_of(" \t\n\r") + 1);
}


string binaryToHex(const string& bin) {
    if (bin.length() != 32) return "Error";
    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < 32; i += 4) {
        bitset<4> bits(bin.substr(i, 4));
        ss << setw(1) << bits.to_ulong();
    }
    return ss.str();
}


string convert_r_to_binary(const string& line) {
    stringstream ss(line);
    string op, p1, p2, p3;
    ss >> op >> p1 >> p2 >> p3;
    
    // Standardize register cleaning
    auto clean_reg = [](string& reg){
        size_t comma = reg.find(',');
        if(comma != string::npos) reg.erase(comma);
    };
    clean_reg(p1); clean_reg(p2); clean_reg(p3);

    string rs_bin = "00000", rt_bin = "00000", rd_bin = "00000", shamt_bin = "00000";

    if (op == "add" || op == "sub" || op == "and" || op == "or" || op == "slt" || op == "nor") {
        rd_bin = register_to_binary.count(p1) ? register_to_binary[p1] : "00000";
        rs_bin = register_to_binary.count(p2) ? register_to_binary[p2] : "00000";
        rt_bin = register_to_binary.count(p3) ? register_to_binary[p3] : "00000";
    } else if (op == "sll") {
        rd_bin = register_to_binary.count(p1) ? register_to_binary[p1] : "00000";
        rt_bin = register_to_binary.count(p2) ? register_to_binary[p2] : "00000";
        int shamt_val = 0;
        try { shamt_val = stoi(p3); } catch(...) {}
        shamt_bin = bitset<5>(shamt_val).to_string();
        rs_bin = "00000";
    } else if (op == "jr") {
        rs_bin = register_to_binary.count(p1) ? register_to_binary[p1] : "00000";
        rt_bin = "00000";
        rd_bin = "00000";
    } else if (op == "syscall") {
        rs_bin = "00000"; rt_bin = "00000"; rd_bin = "00000"; shamt_bin = "00000";
    }

    return instruction_to_op.at(op) + rs_bin + rt_bin + rd_bin + shamt_bin + r_type_to_funct.at(op);
}

string convert_i_to_binary(const string& line, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, int current_pc) {
    stringstream ss(line);
    string op, p1, p2, p3;
    ss >> op >> p1 >> p2 >> p3;

    string rs_bin, rt_bin, imm_bin;
    
    // Standardize register cleaning
    auto clean_reg = [](string& reg){
        size_t comma = reg.find(',');
        if(comma != string::npos) reg.erase(comma);
    };
    clean_reg(p1); clean_reg(p2);

    if (op == "addi" || op == "ori") {
        rt_bin = register_to_binary[p1];
        rs_bin = register_to_binary[p2];
        int imm = 0;
        try {
            if (p3.substr(0, 2) == "0x") imm = stoul(p3, nullptr, 16);
            else imm = stoi(p3);
        } catch(...) {}
        imm_bin = bitset<16>(imm).to_string();
    } else if (op == "lw" || op == "sw") {
        rt_bin = register_to_binary[p1];
        size_t open_paren = p2.find('(');
        if (open_paren != string::npos) { // Format is offset(rs)
            int offset = 0;
            if (open_paren > 0) offset = stoi(p2.substr(0, open_paren));
            string rs_reg = p2.substr(open_paren + 1, p2.find(')') - open_paren - 1);
            rs_bin = register_to_binary[rs_reg];
            imm_bin = bitset<16>(offset).to_string();
        } else { // Format is label (pseudo-instruction)
             rs_bin = "$zero";
             imm_bin = bitset<16>(dataLabels.at(p2)).to_string();
        }
    } else if (op == "beq" || op == "bne") {
        rs_bin = register_to_binary[p1];
        rt_bin = register_to_binary[p2];
        int target_addr = textLabels.at(p3);
        int offset = (target_addr - (current_pc + 4)) / 4;
        imm_bin = bitset<16>(offset).to_string();
    } else if (op == "lui") {
        rt_bin = register_to_binary[p1];
        rs_bin = "00000";
        int imm = 0;
        try {
            if (p2.substr(0, 2) == "0x") imm = stoul(p2, nullptr, 16);
            else imm = stoi(p2);
        } catch(...) {}
        imm_bin = bitset<16>(imm).to_string();
    }

    return instruction_to_op.at(op) + rs_bin + rt_bin + imm_bin;
}

string convert_j_to_binary(const string& line, const unordered_map<string, int>& textLabels) {
    stringstream ss(line);
    string op, label;
    ss >> op >> label;
    int address = textLabels.at(label);
    int jump_target = (address & 0x0FFFFFFF) / 4;
    return instruction_to_op.at(op) + bitset<26>(jump_target).to_string();
}

vector<string> convert_pseudo_to_binary(const string& line, const unordered_map<string, int>& textLabels, const unordered_map<string, int>& dataLabels, int current_pc) {
    stringstream ss(line);
    string op;
    ss >> op;
    vector<string> instructions;
    
    // Standardize register cleaning
    auto clean_reg = [](string& reg){
        size_t comma = reg.find(',');
        if(comma != string::npos) reg.erase(comma);
    };

    if (op == "move") {
        string rd, rs;
        ss >> rd >> rs;
        clean_reg(rd); clean_reg(rs);
        instructions.push_back(convert_r_to_binary("add " + rd + " " + rs + " $zero"));
    } else if (op == "li") {
        string rt, immStr;
        ss >> rt >> immStr;
        clean_reg(rt);
        int imm = 0;
        try {
            if (immStr.substr(0, 2) == "0x") imm = stoul(immStr, nullptr, 16);
            else imm = stoi(immStr);
        } catch(...) {}

        int upper = (imm >> 16) & 0xFFFF;
        int lower = imm & 0xFFFF;

        if (upper != 0) {
             instructions.push_back(convert_i_to_binary("lui " + rt + " " + to_string(upper), textLabels, dataLabels, current_pc));
             if (lower != 0) {
                 instructions.push_back(convert_i_to_binary("ori " + rt + " " + rt + " " + to_string(lower), textLabels, dataLabels, current_pc + 4));
             }
        } else {
             instructions.push_back(convert_i_to_binary("ori " + rt + " $zero " + to_string(lower), textLabels, dataLabels, current_pc));
        }


    } else if (op == "la") {
        string rt, label;
        ss >> rt >> label;
        clean_reg(rt);
        int addr = dataLabels.at(label);
        
        int upper = (addr >> 16) & 0xFFFF;
        int lower = addr & 0xFFFF;

        instructions.push_back(convert_i_to_binary("lui " + rt + " " + to_string(upper), textLabels, dataLabels, current_pc));
        if (lower != 0) {
            instructions.push_back(convert_i_to_binary("ori " + rt + " " + rt + " " + to_string(lower), textLabels, dataLabels, current_pc + 4));
        }
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
    cout << "  -> " << dir << "/instruction_memory.hex created." << endl;
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
    cout << "  -> " << dir << "/data_memory.hex created." << endl;
}

void writeAboutFile(const vector<InstructionDetail>& instructions, const vector<DataDetail>& data, const unordered_map<string, int>& textLabels, const string& dir) {
    ofstream outfile(dir + "/about_program.txt");
    if (!outfile.is_open()) {
        cerr << "Error: Could not create about_program.txt" << endl;
        return;
    }

    outfile << "Label and Address Mappings:" << endl << endl;

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
    set<string> printedDataLabels;
    for (const auto& item : data) {
        if (printedDataLabels.find(item.label) == printedDataLabels.end()) {
            stringstream addr_ss;
            addr_ss << "0x" << hex << setfill('0') << setw(8) << item.address;
            outfile << "# " << left << setw(20) << item.label << addr_ss.str() << endl;
            printedDataLabels.insert(item.label);
        }
    }
    outfile << endl << endl;

    outfile << "About Instruction Memory:" << endl << endl;
    outfile << "# " << left
            << setw(10) << "Label"
            << setw(14) << "Address"
            << setw(30) << "Original Instruction"
            << "Hex Code" << endl;
    outfile << "#" << string(80, '-') << endl;

    unordered_map<int, string> addrToLabel;
    for(const auto& pair : textLabels){
        addrToLabel[pair.second] = pair.first;
    }

    for (const auto& instr : instructions) {
        stringstream addr_ss;
        addr_ss << "0x" << hex << setfill('0') << setw(8) << instr.address;
        
        string label_str = "";
        string instruction_str = instr.originalLine;
        
        size_t colonPos = instruction_str.find(':');
        if(colonPos != string::npos){
            label_str = instruction_str.substr(0, colonPos+1);
            instruction_str = instruction_str.substr(colonPos+1);
            trim(instruction_str);
        }


        outfile << "# " << left
                << setw(10) << label_str
                << setw(14) << addr_ss.str()
                << setw(30) << instruction_str
                << instr.hex << endl;
    }

    outfile << endl << endl << "About Data Memory:" << endl << endl;
    outfile << "# " << left
            << setw(20) << "Memory Address"
            << setw(18) << "Hex Value"
            << setw(18) << "Decimal Value"
            << "Variable Name" << endl;
    outfile << "#" << string(80, '-') << endl;

    for (const auto& item : data) {
        stringstream addr_ss, hex_ss;
        addr_ss << "0x" << hex << setfill('0') << setw(8) << item.address;
        hex_ss << "0x" << hex << setfill('0') << setw(8) << item.value;

        outfile << "# " << left
                << setw(20) << addr_ss.str()
                << setw(18) << hex_ss.str()
                << setw(18) << dec << item.value
                << item.label << endl;
    }

    outfile.close();
    cout << "  -> " << dir << "/about_program.txt created." << endl;
}

