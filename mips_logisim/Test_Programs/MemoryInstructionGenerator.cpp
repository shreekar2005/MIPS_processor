#include <iostream>
#include <fstream> // For ifstream and fstream
#include <string>  // For using string to store lines
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <set>
#include <sstream>
#include <bitset>
#include <cctype>
#include <iomanip> // Required for hex
using namespace std;

set<string> basic_mips_instructions = {"add", "addi", "sub", "lw", "sw", "move", "sll", "beq", "bne", "j","jal", "jr", "syscall", "lui", "ori", "li", "la"};


set<string> r_type_instructions = {"add", "sub", "sll", "jr", "syscall"}; 
set<string> i_type_instructions = {"addi", "lw", "sw", "beq", "bne", "lui", "ori"}; 
set<string> j_type_instructions = {"j", "jal"};
set<string> pseduo_instructions = {"move", "li", "la"};



unordered_map<string, string> instruction_to_op = {
    // I-Type
    {"addi", "001000"},
    {"lw", "100011"},
    {"sw", "101011"},
    {"beq", "000100"},
    {"bne", "000101"},
    {"lui", "001111"},
    {"ori", "001101"},
    // J-Type
    {"j", "000010"},
    {"jal", "000011"},
    // R-Type
    {"add", "000000"},
    {"sub", "000000"},
    {"sll", "000000"},
    {"jr", "000000"},
    {"syscall", "000000"},
};

unordered_map<string, string> r_type_to_funct = {
        {"add", "100000"}, 
        {"sub", "100010"}, 
        {"sll", "000000"}, 
        {"jr",  "001000"}, 
        {"syscall", "001100"}
    };

// I didnt write below unordered map by myself to save time :) 
unordered_map<string, string> register_to_binary = {
    {"$zero", "00000"},
    {"$at",   "00001"},
    {"$v0",   "00010"},
    {"$v1",   "00011"},
    {"$a0",   "00100"},
    {"$a1",   "00101"},
    {"$a2",   "00110"},
    {"$a3",   "00111"},
    {"$t0",   "01000"},
    {"$t1",   "01001"},
    {"$t2",   "01010"},
    {"$t3",   "01011"},
    {"$t4",   "01100"},
    {"$t5",   "01101"},
    {"$t6",   "01110"},
    {"$t7",   "01111"},
    {"$s0",   "10000"},
    {"$s1",   "10001"},
    {"$s2",   "10010"},
    {"$s3",   "10011"},
    {"$s4",   "10100"},
    {"$s5",   "10101"},
    {"$s6",   "10110"},
    {"$s7",   "10111"},
    {"$t8",   "11000"},
    {"$t9",   "11001"},
    {"$k0",   "11010"},
    {"$k1",   "11011"},
    {"$gp",   "11100"},
    {"$sp",   "11101"},
    {"$fp",   "11110"},
    {"$ra",   "11111"}
};

unordered_map<string, int> dataSegLabels; // to store map between lablel : its address (in .data section)
int curr_dataseg_addr = 0; //(base addrs for data segment lables) new label will get this 'curr_dataseg_addr' address and then curr_dataseg_addr+=16
unordered_map<string, int> textSegLabels; // to store map between lablel : its address (in .text section)
int curr_textseg_addr = 0; // after each instruction it will increament by 32

void convert_r_to_binary(vector<string>&instructions, string& line);
void convert_i_to_binary(vector<string>&instructions, string& line);
void convert_j_to_binary(vector<string>&instructions, string& line);
void convert_pseudo_to_binary(vector<string>&instructions, string& line);
string hexCharToBinary(char c); //helper function for hexToBinary
string hexToBinary(const string& hexString);

void trimLeadingWhitespace(string& str) {
    auto it = find_if(str.begin(), str.end(), [](unsigned char ch) {return !isspace(ch);});
    str.erase(str.begin(), it);
}

int main(int argc, char** argv){
    if(argc!=2){
        cerr<<"Not correct use!"<<endl;
        cout<<"to convert assembly to binary : Use <command> <filename>"<<endl;
        return 1;
    }
    ifstream inputFile;
    inputFile.open(argv[1]);
    if (!inputFile.is_open()) {
        cerr << "Error opening file!" << endl;
        return 1; 
    }
    string line;
    vector<string> instructions;
    string secName = "";
    while(getline(inputFile, line)){
        trimLeadingWhitespace(line);
        cout << "\e[2m"; // to print in in gray
        cout<<line<<endl;
        cout << "\e[0m"; // revert printing changes
        if(line == ".data"){
            secName="data";
            continue;
        }
        if(line == ".text"){
            secName="text";
            continue;
        }
        if(secName=="data"){
            stringstream ss(line);
            string firstword;
            ss >> firstword;
            if(firstword[firstword.size()-1]==':') firstword.pop_back();
            dataSegLabels[firstword]=curr_dataseg_addr;
            curr_dataseg_addr+=16;
        }
        // This code for test section
        if(secName=="text"){
            stringstream ss(line);
            string firstword;
            ss >> firstword;
            if(firstword[firstword.size()-1]==':'){ // if it is label
                firstword.pop_back();
                textSegLabels[firstword]=curr_textseg_addr;
                continue; // do not increment curr_textseg_addr
            }
            if(basic_mips_instructions.find(firstword)==basic_mips_instructions.end()) continue;
            
            if(r_type_instructions.find(firstword)!=r_type_instructions.end()) convert_r_to_binary(instructions, line);
            if(i_type_instructions.find(firstword)!=i_type_instructions.end()) convert_i_to_binary(instructions, line);
            if(j_type_instructions.find(firstword)!=j_type_instructions.end()) convert_j_to_binary(instructions, line);
            if(pseduo_instructions.find(firstword)!=pseduo_instructions.end()) convert_pseudo_to_binary(instructions, line);

            curr_textseg_addr+=16; //increment curr_textseg_addr bcs 1 instruction will take 32 bit;
        }
    }
    cout << "\e[1m"; // to print the line in bold
    cout<<endl<<"Now here are all instructions line by line in binary : "<<endl;
    cout << "\e[0m"; // revert bold effect
    for(auto instruction : instructions){
        cout<<instruction<<endl;
    }
    
    inputFile.close();
}

void convert_r_to_binary(vector<string>&instructions, string& line){
    string instruction="";
    stringstream ss(line);
    string word;
    ss>>word;
    instruction+=instruction_to_op[word]+"|";
    string rd; ss>>rd;
    if(rd[rd.size()-1]==',') rd.pop_back();
    string rs; ss>>rs;
    if(rs[rs.size()-1]==',') rs.pop_back();
    string rt; ss>>rt;
    // if(rt[rt.size()-1]==',') rt.pop_back(); // not necc ig
    if(register_to_binary.find(rs)!=register_to_binary.end()) instruction+=register_to_binary[rs]+"|";
    else instruction+="00000|";
    if(register_to_binary.find(rt)!=register_to_binary.end()) instruction+=register_to_binary[rt]+"|";
    else instruction+="00000|";
    if(register_to_binary.find(rd)!=register_to_binary.end()) instruction+=register_to_binary[rd]+"|";
    else instruction+="00000|";
    instruction+="00000|"; // not considering any shift amount :)
    instruction+=r_type_to_funct[word];
    instructions.push_back(instruction);
    cout<<"    "<<instruction<<endl;
}

void convert_i_to_binary(vector<string>&instructions, string& line){
    // addi, ori
    // lw,sw
    // beq, bne
    // lui
    string instruction="";
    stringstream ss(line);
    string word;
    ss>>word;
    instruction+=instruction_to_op[word]+"|";
    if(word=="addi" || word =="ori"){
        string rt; ss>>rt;
        if(rt[rt.size()-1]==',') rt.pop_back();
        string rs; ss>>rs;
        if(rs[rs.size()-1]==',') rs.pop_back();
        string immediate; ss>>immediate;
        // if(immediate[immediate.size()-1]==',') immediate.pop_back(); // not necc ig
        string imm_bin_string;
        if(word=="addi"){
            int imm=0;
            if(immediate.size()!=0) imm=stoi(immediate);
            bitset<16> imm_in_binary(imm); 
            imm_bin_string = imm_in_binary.to_string();
        }
        if(word=="ori"){ // ori should have hex immediate to do OR
            string hexnum = immediate.substr(2); // hexnum = immediate[2]+immediate[3]+...
            imm_bin_string = hexToBinary(hexnum);
        }
        if(register_to_binary.find(rs)!=register_to_binary.end()) instruction+=register_to_binary[rs]+"|";
        else instruction+="00000|";
        if(register_to_binary.find(rt)!=register_to_binary.end()) instruction+=register_to_binary[rt]+"|";
        else instruction+="00000|";
        instruction+=imm_bin_string;

        // cout<<instruction<<endl;
    }
    else if(word=="lw" || word=="sw"){
        string rt; ss>>rt;
        if(rt[rt.size()-1]==',') rt.pop_back();
        string rs; ss>>rs;
        // if(rs[rs.size()-1]==',') rs.pop_back(); // not necc ig
        if(!isalpha(rs[0])){ //first letter of rs is number (e.g. 4($s0))
            string offset="";
            for(char c:rs){
                if (c=='(') break;
                offset+=c;
            }
            rs=rs.substr(offset.size(), 3); // rs will have only string like $at or $t1 etc.
            
            int off=0;
            // cout<<offset<<"hi"<<endl;
            if(offset.size()!=0) off=stoi(offset);
            bitset<16> off_to_bin(off);
            string off_bin_string = off_to_bin.to_string();
            if(register_to_binary.find(rs)!=register_to_binary.end()) instruction+=register_to_binary[rs]+"|";
            else instruction+="00000|";
            if(register_to_binary.find(rt)!=register_to_binary.end()) instruction+=register_to_binary[rt]+"|";
            else instruction+="00000|";
            instruction+=off_bin_string;
        }
        else { // rs will be label
            if(dataSegLabels.find(rs)==dataSegLabels.end()) {
                cerr<<"-----"<<"your given label not defined"<<endl;
                return;
            }
            int imm=dataSegLabels[rs];
            // cout<<offset<<"hi"<<endl;
            bitset<16> imm_to_bin(imm);
            string imm_bin_string = imm_to_bin.to_string();
            if(register_to_binary.find(rs)!=register_to_binary.end()) instruction+=register_to_binary[rs]+"|";
            else instruction+="00000|";
            if(register_to_binary.find(rt)!=register_to_binary.end()) instruction+=register_to_binary[rt]+"|";
            else instruction+="00000|";
            instruction+=imm_bin_string;
        }

        // cout<<instruction<<endl;
    }
    else if(word=="beq" || word=="bne"){
        // here order of rs and rt wont matter :)
        string rs; ss>>rs;
        if(rs[rs.size()-1]==',') rs.pop_back();
        string rt; ss>>rt;
        if(rt[rt.size()-1]==',') rt.pop_back();
        string label; ss>>label;
        if(textSegLabels.find(label)==textSegLabels.end()) {
            cerr<<"-----"<<"this label is not defined in text segment"<<endl;
            return;
        }
        int imm=textSegLabels[label];
        bitset<16> imm_to_bin(imm);
        string imm_bin_string = imm_to_bin.to_string();
        if(register_to_binary.find(rs)!=register_to_binary.end()) instruction+=register_to_binary[rs]+"|";
        else instruction+="00000|";
        if(register_to_binary.find(rt)!=register_to_binary.end()) instruction+=register_to_binary[rt]+"|";
        else instruction+="00000|";
        instruction+=imm_bin_string;

        // cout<<instruction<<endl;
    }
    else if(word=="lui"){
        string rt; ss>>rt;
        if(rt[rt.size()-1]==',') rt.pop_back();
        string hexnum; ss>>hexnum;
        hexnum=hexnum.substr(2);
        string imm_bin_string = hexToBinary(hexnum);
        instruction+="00000|"; // rs is 00000 for lui
        if(register_to_binary.find(rt)!=register_to_binary.end()) instruction+=register_to_binary[rt]+"|";
        else instruction+="00000|";
        instruction+=imm_bin_string;

        // cout<<instruction<<endl;
    }
    instructions.push_back(instruction);
    cout<<"    "<<instruction<<endl;
}

void convert_j_to_binary(vector<string>&instructions, string& line){
    string instruction="";
    stringstream ss(line);
    string word;
    ss>>word;
    instruction+=instruction_to_op[word]+"|";
    string label;
    ss>>label;
    if(textSegLabels.find(label)==textSegLabels.end()){
        cerr<<"-----"<<"label is not found in text section"<<endl;
        return;
    }
    bitset<26> bin_addr(textSegLabels[label]);
    instruction+=bin_addr.to_string();
    instructions.push_back(instruction);
    cout<<"    "<<instruction<<endl;
}

void convert_pseudo_to_binary(vector<string>&instructions, string& line){
    string instruction="";
    stringstream ss(line);
    string word; // using this variable to take word of line one by one
    ss>>word;
    if(word=="move"){ //break "move $t1, $t2" into "add $t1, $t2, $zero"
        string intermediate_instruction = "add";
        while(ss>>word){
            intermediate_instruction+=" "+word;
        }
        intermediate_instruction+=", $zero";
        cout<<"(THIS IS INTERMEDIATE INSTRUCTION : "<<intermediate_instruction<<" )"<<endl;
        convert_r_to_binary(instructions, intermediate_instruction);
    }
    else if(word=="li"){ // e.g. li $t0, 10
        string reg1; ss>>reg1;
        string decimalString; ss>>decimalString;
        int decimalNumber = stoi(decimalString);
        stringstream ss_temp;
        ss_temp << setw(8) << setfill('0')<< hex << decimalNumber;
        string hexdecnum = ss_temp.str();
        // now hexdecnum have hexadecimal number of given decimal number 
        string upper="0x", lower="0x";
        for(int i=0; i<8; i++){
            if(i<4) upper+=hexdecnum[i];
            else lower+=hexdecnum[i];
        }
        string intermediate_instruction1="lui ";
        intermediate_instruction1+=reg1+" "+upper;

        string intermediate_instruction2="ori ";
        intermediate_instruction2+=reg1+" "+reg1+" "+lower;
        cout<<"     "<<"(THIS IS INTERMEDIATE INSTRUCTION 1/2: "<<intermediate_instruction1<<" )"<<endl;
        convert_i_to_binary(instructions, intermediate_instruction1);
        cout<<"     "<<"(THIS IS INTERMEDIATE INSTRUCTION 2/2: "<<intermediate_instruction2<<" )"<<endl;
        convert_i_to_binary(instructions, intermediate_instruction2);
    }
    else if(word=="la"){ // la $t1, label
        string reg1; ss>>reg1;
        string label; ss>>label;
        if(dataSegLabels.find(label)==dataSegLabels.end()){
            cerr<<"-----"<<"label not found in data section"<<endl;
            return;
        }
        int decimalNumber = dataSegLabels[label];
        stringstream ss_temp;
        ss_temp << setw(8) << setfill('0')<< hex << decimalNumber;
        string hexdecnum = ss_temp.str();
        // now hexdecnum have hexadecimal number of given decimal number 
        string upper="0x", lower="0x";
        for(int i=0; i<8; i++){
            if(i<4) upper+=hexdecnum[i];
            else lower+=hexdecnum[i];
        }
        string intermediate_instruction1="lui ";
        intermediate_instruction1+=reg1+" "+upper;

        string intermediate_instruction2="ori ";
        intermediate_instruction2+=reg1+" "+reg1+" "+lower;
        cout<<"     "<<"(THIS IS INTERMEDIATE INSTRUCTION 1/2: "<<intermediate_instruction1<<" )"<<endl;
        convert_i_to_binary(instructions, intermediate_instruction1);
        cout<<"     "<<"(THIS IS INTERMEDIATE INSTRUCTION 2/2: "<<intermediate_instruction2<<" )"<<endl;
        convert_i_to_binary(instructions, intermediate_instruction2);
    }
}

string hexCharToBinary(char c) {
    switch (toupper(c)) {
        case '0': return "0000";
        case '1': return "0001";
        case '2': return "0010";
        case '3': return "0011";
        case '4': return "0100";
        case '5': return "0101";
        case '6': return "0110";
        case '7': return "0111";
        case '8': return "1000";
        case '9': return "1001";
        case 'A': return "1010";
        case 'B': return "1011";
        case 'C': return "1100";
        case 'D': return "1101";
        case 'E': return "1110";
        case 'F': return "1111";
        default: return ""; // for invalid hex characters
    }
}

string hexToBinary(const string& hexString) {
    string binaryString = "";
    for (char c : hexString) {
        binaryString += hexCharToBinary(c);
    }
    return binaryString;
}