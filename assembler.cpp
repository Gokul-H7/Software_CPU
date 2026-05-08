#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstdint>

using namespace std;

// Struct to store instruction metadata for dispatch
struct OpConfig { uint8_t opcode, f3, f7; char type; };

// ISA Configuration Map
unordered_map<string, OpConfig> ISA = {
    {"add",  {0x33, 0x0, 0x00, 'R'}}, {"sub",  {0x33, 0x0, 0x20, 'R'}},
    {"addi", {0x13, 0x0, 0x00, 'I'}}, {"lw",   {0x03, 0x2, 0x00, 'I'}},
    {"sw",   {0x23, 0x2, 0x00, 'S'}}, {"beq",  {0x63, 0x0, 0x00, 'B'}},
    {"jal",  {0x6F, 0x0, 0x00, 'J'}}
};

// Helper: Remove commas and whitespace
string clean(string s) {
    s.erase(remove_if(s.begin(), s.end(), [](char c) { 
        return c == ',' || isspace(c) || c == '(' || c == ')'; 
    }), s.end());
    return s;
}

int getReg(string r) {
    r = clean(r);
    // Explicitly check the character at index 0
    if (r.length() > 0 && r.at(0) == 'x') {
        return stoi(r.substr(1));
    }
    // Fallback if the string is just a number or empty
    return (r.length() > 0) ? stoi(r) : 0;
}

// Pass 2: Binary Encoders
uint32_t pack(const OpConfig& cfg, int rd, int rs1, int rs2, int imm) {
    if (cfg.type == 'R') return (cfg.f7 << 25) | (rs2 << 20) | (rs1 << 15) | (cfg.f3 << 12) | (rd << 7) | cfg.opcode;
    if (cfg.type == 'I') return ((imm & 0xFFF) << 20) | (rs1 << 15) | (cfg.f3 << 12) | (rd << 7) | cfg.opcode;
    if (cfg.type == 'S') return (((imm >> 5) & 0x7F) << 25) | (rd << 20) | (rs1 << 15) | (cfg.f3 << 12) | ((imm & 0x1F) << 7) | cfg.opcode;
    if (cfg.type == 'B') return (((imm >> 12) & 1) << 31) | (((imm >> 5) & 0x3F) << 25) | (rs2 << 20) | (rs1 << 15) | (cfg.f3 << 12) | (((imm >> 1) & 0xF) << 8) | (((imm >> 11) & 1) << 7) | cfg.opcode;
    if (cfg.type == 'J') return (((imm >> 20) & 1) << 31) | (((imm >> 1) & 0x3FF) << 21) | (((imm >> 11) & 1) << 20) | (((imm >> 12) & 0xFF) << 12) | (rd << 7) | cfg.opcode;
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) return cout << "Usage: rasm <file.asm>\n", 1;
    ifstream infile(argv[1]);
    ofstream outfile("output.bin", ios::binary);
    
    vector<pair<int, string>> source;
    unordered_map<string, int> labels;
    string line;
    int pc = 0;

    // PASS 1: Labels & Cleaning
    while (getline(infile, line)) {
        line = line.substr(0, line.find('#')); // Strip comments
        stringstream ss(line); string word; ss >> word;
        if (word.empty()) continue;
        if (word.back() == ':') { labels[word.substr(0, word.size()-1)] = pc; ss >> word; }
        if (!word.empty() && word.back() != ':') { source.push_back({pc, line.substr(line.find(word))}); pc += 4; }
    }

    // PASS 2: Encoding
    for (auto& entry : source) {
        stringstream ss(entry.second);
        string op, s_rd, s_rs1, s_rs2;
        ss >> op >> s_rd >> s_rs1 >> s_rs2;
        
        auto it = ISA.find(op);
        if (it == ISA.end()) continue;
        
        int rd = 0, rs1 = 0, rs2 = 0, imm = 0;
        OpConfig cfg = it->second;

        if (cfg.type == 'R') { rd = getReg(s_rd); rs1 = getReg(s_rs1); rs2 = getReg(s_rs2); }
        else if (cfg.type == 'I') { rd = getReg(s_rd); rs1 = getReg(s_rs1); imm = stoi(s_rs2); }
        else if (cfg.type == 'S') { rs2 = getReg(s_rd); imm = stoi(s_rs1); rs1 = getReg(s_rs2); } // rs2, imm(rs1)
        else if (cfg.type == 'B' || cfg.type == 'J') {
            rd = (cfg.type == 'J') ? getReg(s_rd) : 0;
            rs1 = (cfg.type == 'B') ? getReg(s_rd) : 0;
            rs2 = (cfg.type == 'B') ? getReg(s_rs1) : 0;
            string lab = (cfg.type == 'J') ? s_rs1 : s_rs2;
            imm = labels[clean(lab)] - entry.first;
        }

        uint32_t bin = pack(cfg, rd, rs1, rs2, imm);
        outfile.write(reinterpret_cast<const char*>(&bin), 4); 
    }
    return 0;
}