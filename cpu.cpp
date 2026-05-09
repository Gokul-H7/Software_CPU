// ------------------------------------------------------------
// Reduced RISC-V CPU Emulator
//
// Supported Instructions:
//   add, sub
//   addi
//   lw, sw
//   beq
//   jal, jalr
//
// Features:
//   - 32 general-purpose registers
//   - byte-addressable memory
//   - Stack support for recursion
//   - MMIO output support
//   - optional character and verbose modes [-c] [-v]
//
// Memory:
//   64KB total memory
// ------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <cstdlib>

using namespace std;

const uint32_t MEM_SIZE = 65536;       // 64KB
const uint32_t MMIO_PRINT = 1024;      // sw here prints low byte as char

class RISCV_CPU {
private:
    uint32_t regs[32] = {0};
    uint32_t pc = 0;
    uint8_t memory[MEM_SIZE] = {0};
    bool running = true;
    bool char_mode = false; // If true, print as char instead of number
    bool verbose_mode = false;   // If true, print instruction trace

    // Helper function for verbose tracing
    void verbose(const string& msg) {
        if (verbose_mode) {
            cout << msg << "\n";
        }
    }

    // Sign-extend immediate value to 32 bits
    static int32_t signExtend(uint32_t value, int bits) {
        uint32_t mask = 1u << (bits - 1);
        return int32_t((value ^ mask) - mask);
    }

    // Load a 32-bit word from memory (little-endian)
    uint32_t load32(uint32_t addr) {
        if (addr + 3 >= MEM_SIZE) {
            cerr << "Load address out of range: 0x" << hex << addr << dec << "\n";
            running = false;
            return 0;
        }
        return uint32_t(memory[addr]) |
               (uint32_t(memory[addr + 1]) << 8) |
               (uint32_t(memory[addr + 2]) << 16) |
               (uint32_t(memory[addr + 3]) << 24);
    }

    // Store a 32-bit word to memory (little-endian)
    // if value matches MMIO_PRINT address, print it instead of storing
    void store32(uint32_t addr, uint32_t value) {
        if (addr == MMIO_PRINT) {
            if (char_mode) {
                cout << char(value & 0xFF) << flush;
            } else {
                cout << "[MMIO] " << dec << value << "\n";
            }
            return;
        }

        if (addr + 3 >= MEM_SIZE) {
            cerr << "Store address out of range: 0x" << hex << addr << dec << "\n";
            running = false;
            return;
        }

        memory[addr]     = value & 0xFF;
        memory[addr + 1] = (value >> 8) & 0xFF;
        memory[addr + 2] = (value >> 16) & 0xFF;
        memory[addr + 3] = (value >> 24) & 0xFF;
    }

public:
    // Constructor with optional modes
    RISCV_CPU(bool char_mode = false, bool verbose_mode = false) 
        : char_mode(char_mode), verbose_mode(verbose_mode) {}

    // Load binary program into memory
    void loadBinary(const string& filename) {
        ifstream file(filename, ios::binary);
        if (!file) {
            cerr << "Error: Could not open file " << filename << "\n";
            exit(1);
        }
        file.read(reinterpret_cast<char*>(memory), MEM_SIZE);
    }

    // Simple ALU function for supported operations
    uint32_t ALU(uint32_t a, uint32_t b, uint8_t funct3, uint8_t funct7, uint8_t opcode) {
        if (opcode == 0x33) {
            if (funct3 == 0x0 && funct7 == 0x00) return a + b;
            if (funct3 == 0x0 && funct7 == 0x20) return a - b;
        }
        if (opcode == 0x13) {
            if (funct3 == 0x0) return a + b;
        }
        return 0;
    }

    // Execute one instruction at the current PC
    void step() {
        if (pc + 3 >= MEM_SIZE) {
            running = false;
            return;
        }

        // fetch instruction
        uint32_t instr = load32(pc);
        if (!running) return;

        if(verbose_mode) {
            cout << "\n[FETCH] PC=0x" << hex << pc
            << " INSTR=0x" << setw(8) << setfill('0') << instr 
            << setfill(' ') << dec << "\n";
        }

        // Treat all-zero memory after the program as halt.
        if (instr == 0) {
            running = false;
            return;
        }

        // Decode instruction fields
        uint8_t opcode = instr & 0x7F;
        uint8_t rd     = (instr >> 7) & 0x1F;
        uint8_t funct3 = (instr >> 12) & 0x07;
        uint8_t rs1    = (instr >> 15) & 0x1F;
        uint8_t rs2    = (instr >> 20) & 0x1F;
        uint8_t funct7 = (instr >> 25) & 0x7F;
        uint32_t next_pc = pc + 4;

        if (verbose_mode) {
            cout << "[DECODE] opcode=0x" << hex << int(opcode)
            << " rd=x" << dec << int(rd)
            << " rs1=x" << int(rs1)
            << " rs2=x" << int(rs2)
            << " funct3=0x" << hex << int(funct3)
            << " funct7=0x" << int(funct7)
            << dec << "\n";
        }

        switch (opcode) {
            case 0x33: { // R-type: add/sub
                regs[rd] = ALU(regs[rs1], regs[rs2], funct3, funct7, opcode);
                if (verbose_mode) {
                    cout << "[COMPUTE] R-type x" << int(rd)
                    << " = x" << int(rs1)
                    << " op x" << int(rs2)
                    << " -> " << regs[rd] << "\n";
                }
                break;
            }

            case 0x13: { // addi
                int32_t imm = signExtend(instr >> 20, 12);
                regs[rd] = ALU(regs[rs1], uint32_t(imm), funct3, 0, opcode);
                if (verbose_mode) {
                    cout << "[COMPUTE] I-type x" << int(rd)
                    << " = x" << int(rs1)
                    << " op " << imm
                    << " -> " << regs[rd] << "\n";
                }
                break;
            }

            case 0x03: { // lw
                if (funct3 != 0x2) { running = false; break; }
                int32_t imm = signExtend(instr >> 20, 12);
                uint32_t addr = regs[rs1] + imm;
                regs[rd] = load32(addr);
                if (verbose_mode) {
                    cout << "[STORE] lw loaded memory[0x" << hex << addr
                    << "] into x" << dec << int(rd)
                    << " = " << regs[rd] << "\n";
                }
                break;
            }

            case 0x23: { // sw
                if (funct3 != 0x2) { running = false; break; }
                uint32_t imm_raw = ((instr >> 7) & 0x1F) | (((instr >> 25) & 0x7F) << 5);
                int32_t imm = signExtend(imm_raw, 12);
                uint32_t addr = regs[rs1] + imm;
                if (verbose_mode) {
                    cout << "[STORE] sw x" << int(rs2)
                    << " value=" << regs[rs2]
                    << " to memory/MMIO[0x" << hex << addr << dec << "]\n";
                }
                store32(addr, regs[rs2]);     
                break;
            }

            case 0x63: { // beq
                uint32_t imm_raw = (((instr >> 31) & 0x1) << 12) |
                                   (((instr >> 7)  & 0x1) << 11) |
                                   (((instr >> 25) & 0x3F) << 5) |
                                   (((instr >> 8)  & 0xF) << 1);
                int32_t imm = signExtend(imm_raw, 13);
                if (funct3 == 0x0 && regs[rs1] == regs[rs2]) {
                    next_pc = pc + imm;
                }
                if (verbose_mode) {
                    cout << "[COMPUTE] beq x" << int(rs1)
                    << " == x" << int(rs2)
                    << " ? " << (regs[rs1] == regs[rs2] ? "taken" : "not taken")
                    << "\n";
                }
                break;
            }

            case 0x6F: { // jal
                uint32_t imm_raw = (((instr >> 31) & 0x1) << 20) |
                                   (((instr >> 12) & 0xFF) << 12) |
                                   (((instr >> 20) & 0x1) << 11) |
                                   (((instr >> 21) & 0x3FF) << 1);
                int32_t imm = signExtend(imm_raw, 21);
                regs[rd] = pc + 4;
                next_pc = pc + imm;
                if (verbose_mode) {
                    cout << "[COMPUTE] jal: x" << int(rd)
                    << " = return address 0x" << hex << regs[rd]
                    << ", next PC=0x" << next_pc << dec << "\n";
                }
                break;
            }

            case 0x67: { // jalr
                if (funct3 != 0x0) { running = false; break; }
                int32_t imm = signExtend(instr >> 20, 12);
                uint32_t return_addr = pc + 4;
                next_pc = (regs[rs1] + imm) & ~1u;
                regs[rd] = return_addr;
                if (verbose_mode) {
                    cout << "[COMPUTE] jalr: x" << int(rd)
                    << " = return address 0x" << hex << return_addr
                    << ", next PC=0x" << next_pc << dec << "\n";
                }
                break;
            }

            default:
                running = false;
                break;
        }

        regs[0] = 0;
        pc = next_pc;
    }

    // CPU run loop
    void run() {
        while (running) step();
    }

    // Dump CPU state (for debugging)
    void dumpState() const {
        cout << "\n--- CPU State Dump ---\n";
        cout << "PC: 0x" << hex << pc << dec << "\n";
        for (int i = 0; i < 32; i++) {
            cout << "x" << dec << i << ": 0x" << hex << regs[i] << "  ";
            if ((i + 1) % 4 == 0) cout << "\n";
        }
        cout << dec;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <binary_file> [-c]\n";
        return 1;
    }

    bool char_mode = false;
    bool verbose_mode = false;

    // optional -c/-v flag
    for (int i = 2; i < argc; i++) {
        string arg = argv[i];

        if (arg == "-c") {
            char_mode = true;
        } else if (arg == "-v") {
            verbose_mode = true;
        } else {
            cerr << "Unknown option: " << arg << "\n";
            cout << "Usage: " << argv[0] << " <binary_file> [-c] [-v]\n";
            return 1;
        }
    }


    RISCV_CPU myCPU(char_mode, verbose_mode);
    myCPU.loadBinary(argv[1]);
    myCPU.run();
    myCPU.dumpState();
    return 0;
}
