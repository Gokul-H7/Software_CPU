// ------------------------------------------------------------
// Reduced RISC-V Assembler
//
// Supported Instructions:
//   R-Type : add, sub
//   I-Type : addi, jalr
//   Load   : lw
//   Store  : sw
//   Branch : beq
//   Jump   : jal
//
// Output:
//   Generates raw binary machine code in output.bin
// ------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

using namespace std;

struct OpConfig {
    uint8_t opcode; // primary instruction opcode
    uint8_t f3;     // funct3 field 
    uint8_t f7;     // funct7 field
    char type;      // instruction format type
};

// ISA lookup table: maps instruction names to opcode, funct3, funct7, and type
unordered_map<string, OpConfig> ISA = {
    {"add",  {0x33, 0x0, 0x00, 'R'}},
    {"sub",  {0x33, 0x0, 0x20, 'R'}},
    {"addi", {0x13, 0x0, 0x00, 'I'}},
    {"lw",   {0x03, 0x2, 0x00, 'L'}},
    {"sw",   {0x23, 0x2, 0x00, 'S'}},
    {"beq",  {0x63, 0x0, 0x00, 'B'}},
    {"jal",  {0x6F, 0x0, 0x00, 'J'}},
    {"jalr", {0x67, 0x0, 0x00, 'I'}}
};

// Helper functions for parsing and encoding instructions
static string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// parse assembly syntax into tokens
static vector<string> tokenize(string line) {
    // Make commas and parens separators, so lw x5, 0(x2) => x5 0 x2
    for (char& c : line) {
        if (c == ',' || c == '(' || c == ')') c = ' ';
    }
    stringstream ss(line);
    vector<string> toks;
    string t;
    while (ss >> t) toks.push_back(t);
    return toks;
}

// convert register name into number (x5 -> 5)
static int getReg(string r) {
    r = trim(r);
    if (r.empty()) throw runtime_error("empty register");
    if (r[0] == 'x') return stoi(r.substr(1));
    return stoi(r);
}

// Get immediate value from either a label or a numeric literal
static int getImmOrLabel(const string& s, const unordered_map<string, int>& labels, int pc) {
    auto it = labels.find(s);
    if (it != labels.end()) return it->second - pc;
    return stoi(s);
}

// Encode instruction fields into a 32-bit binary instruction
uint32_t pack(const OpConfig& cfg, int rd, int rs1, int rs2, int imm) {
    switch (cfg.type) {
        case 'R':
            return (uint32_t(cfg.f7) << 25) |
                   (uint32_t(rs2 & 0x1F) << 20) |
                   (uint32_t(rs1 & 0x1F) << 15) |
                   (uint32_t(cfg.f3) << 12) |
                   (uint32_t(rd & 0x1F) << 7) |
                   cfg.opcode;

        case 'I':
        case 'L':
            return (uint32_t(imm & 0xFFF) << 20) |
                   (uint32_t(rs1 & 0x1F) << 15) |
                   (uint32_t(cfg.f3) << 12) |
                   (uint32_t(rd & 0x1F) << 7) |
                   cfg.opcode;

        case 'S':
            return (uint32_t((imm >> 5) & 0x7F) << 25) |
                   (uint32_t(rs2 & 0x1F) << 20) |
                   (uint32_t(rs1 & 0x1F) << 15) |
                   (uint32_t(cfg.f3) << 12) |
                   (uint32_t(imm & 0x1F) << 7) |
                   cfg.opcode;

        case 'B':
            return (uint32_t((imm >> 12) & 0x1) << 31) |
                   (uint32_t((imm >> 5) & 0x3F) << 25) |
                   (uint32_t(rs2 & 0x1F) << 20) |
                   (uint32_t(rs1 & 0x1F) << 15) |
                   (uint32_t(cfg.f3) << 12) |
                   (uint32_t((imm >> 1) & 0xF) << 8) |
                   (uint32_t((imm >> 11) & 0x1) << 7) |
                   cfg.opcode;

        case 'J':
            return (uint32_t((imm >> 20) & 0x1) << 31) |
                   (uint32_t((imm >> 1) & 0x3FF) << 21) |
                   (uint32_t((imm >> 11) & 0x1) << 20) |
                   (uint32_t((imm >> 12) & 0xFF) << 12) |
                   (uint32_t(rd & 0x1F) << 7) |
                   cfg.opcode;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    // file handling
    if (argc < 2) {
        cerr << "Usage: rasm <file.asm>\n";
        return 1;
    }

    ifstream infile(argv[1]);
    if (!infile) {
        cerr << "Error: could not open " << argv[1] << "\n";
        return 1;
    }

    ofstream outfile("output.bin", ios::binary);
    if (!outfile) {
        cerr << "Error: could not create output.bin\n";
        return 1;
    }

    // Data structures for first pass
    vector<pair<int, string>> source;
    unordered_map<string, int> labels;
    string line;
    int pc = 0;

    // PASS 1: collect labels and cleaned instructions
    while (getline(infile, line)) {
        line = line.substr(0, line.find('#'));
        line = trim(line);
        if (line.empty()) continue;

        while (true) {
            size_t colon = line.find(':');
            if (colon == string::npos) break;

            string label = trim(line.substr(0, colon));
            if (!label.empty()) labels[label] = pc;

            line = trim(line.substr(colon + 1));
            if (line.empty()) break;
        }

        if (!line.empty()) {
            source.push_back({pc, line});
            pc += 4;
        }
    }

    // PASS 2: encode instructions
    for (const auto& entry : source) {
        int curPC = entry.first;
        vector<string> t = tokenize(entry.second);
        if (t.empty()) continue;

        string op = t[0];
        auto it = ISA.find(op);
        if (it == ISA.end()) {
            cerr << "Unknown instruction at PC " << curPC << ": " << op << "\n";
            return 1;
        }

        OpConfig cfg = it->second;
        int rd = 0, rs1 = 0, rs2 = 0, imm = 0;

        try {
            if (cfg.type == 'R') {
                if (t.size() != 4) throw runtime_error("R format: op rd rs1 rs2");
                rd = getReg(t[1]);
                rs1 = getReg(t[2]);
                rs2 = getReg(t[3]);
            } else if (op == "addi") {
                if (t.size() != 4) throw runtime_error("addi format: addi rd rs1 imm");
                rd = getReg(t[1]);
                rs1 = getReg(t[2]);
                imm = stoi(t[3]);
            } else if (op == "lw") {
                if (t.size() != 4) throw runtime_error("lw format: lw rd imm(rs1)");
                rd = getReg(t[1]);
                imm = stoi(t[2]);
                rs1 = getReg(t[3]);
            } else if (op == "jalr") {
                // Supports both:
                //   jalr x0, 0, x1
                //   jalr x0, 0(x1)
                // tokenize() makes both become: jalr x0 0 x1
                if (t.size() != 4) throw runtime_error("jalr format: jalr rd imm rs1 OR jalr rd imm(rs1)");
                rd = getReg(t[1]);
                imm = stoi(t[2]);
                rs1 = getReg(t[3]);
            } else if (cfg.type == 'S') {
                if (t.size() != 4) throw runtime_error("sw format: sw rs2 imm(rs1)");
                rs2 = getReg(t[1]);
                imm = stoi(t[2]);
                rs1 = getReg(t[3]);
            } else if (cfg.type == 'B') {
                if (t.size() != 4) throw runtime_error("beq format: beq rs1 rs2 label");
                rs1 = getReg(t[1]);
                rs2 = getReg(t[2]);
                if (!labels.count(t[3])) throw runtime_error("unknown label: " + t[3]);
                imm = labels[t[3]] - curPC;
            } else if (cfg.type == 'J') {
                if (t.size() != 3) throw runtime_error("jal format: jal rd label");
                rd = getReg(t[1]);
                if (!labels.count(t[2])) throw runtime_error("unknown label: " + t[2]);
                imm = labels[t[2]] - curPC;
            }
        } catch (const exception& e) {
            cerr << "Assembly error at PC " << curPC << " in line: " << entry.second << "\n";
            cerr << "  " << e.what() << "\n";
            return 1;
        }

        uint32_t bin = pack(cfg, rd, rs1, rs2, imm);
        outfile.write(reinterpret_cast<const char*>(&bin), 4);
    }

    return 0;
}
