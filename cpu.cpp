#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <iomanip>

using namespace std;

// Memory Constants
const uint32_t MEM_SIZE = 65536;       // 64KB
const uint32_t MMIO_PRINT = 0x8000;    // Writing here prints a char

class RISCV_CPU {
private:
    uint32_t regs[32] = {0};           // Register File (x0-x31)
    uint32_t pc = 0;                   // Program Counter
    uint8_t memory[MEM_SIZE] = {0};    // Unified Memory
    bool running = true;

public:
    RISCV_CPU() {
        regs[0] = 0; // x0 is hardwired to 0
    }

    void loadBinary(string filename) {
        ifstream file(filename, ios::binary);
        if (!file) {
            cerr << "Error: Could not open file " << filename << endl;
            exit(1);
        }
        file.read(reinterpret_cast<char*>(memory), MEM_SIZE);
    }

    // --- ARITHMETIC LOGIC UNIT (ALU) ---
    uint32_t ALU(uint32_t a, uint32_t b, uint8_t funct3, uint8_t funct7, uint8_t opcode) {
        if (opcode == 0x33) { // R-Type
            if (funct3 == 0x0 && funct7 == 0x00) return a + b; // ADD
            if (funct3 == 0x0 && funct7 == 0x20) return a - b; // SUB
            if (funct3 == 0x7) return a & b;                   // AND
            if (funct3 == 0x6) return a | b;                   // OR
        } 
        if (opcode == 0x13) { // I-Type (ADDI)
            if (funct3 == 0x0) return a + b;
        }
        return 0;
    }

    // --- CONTROL UNIT & EXECUTION ---
    void step() {
        // 1. FETCH
        if (pc + 4 > MEM_SIZE) { running = false; return; }
        uint32_t instr = *reinterpret_cast<uint32_t*>(&memory[pc]);

        // 2. DECODE (Extracting fields per RISC-V spec)
        uint8_t opcode = instr & 0x7F;
        uint8_t rd     = (instr >> 7) & 0x1F;
        uint8_t funct3 = (instr >> 12) & 0x07;
        uint8_t rs1    = (instr >> 15) & 0x1F;
        uint8_t rs2    = (instr >> 20) & 0x1F;
        uint8_t funct7 = (instr >> 25) & 0x7F;

        uint32_t next_pc = pc + 4;

        // 3. EXECUTE (Control Unit logic)
        switch (opcode) {
            case 0x33: { // R-Type
                regs[rd] = ALU(regs[rs1], regs[rs2], funct3, funct7, opcode);
                break;
            }
            case 0x13: { // I-Type (ADDI)
                int32_t imm = (int32_t(instr) >> 20); // Sign-extended 12-bit
                regs[rd] = ALU(regs[rs1], imm, funct3, 0, opcode);
                break;
            }
            case 0x03: { // Load Word (LW)
                int32_t imm = (int32_t(instr) >> 20);
                uint32_t addr = regs[rs1] + imm;
                regs[rd] = *reinterpret_cast<uint32_t*>(&memory[addr]);
                break;
            }
            case 0x23: { // Store Word (SW)
                int32_t imm = ((instr >> 7) & 0x1F) | ((instr >> 25) << 5);
                if (imm & 0x800) imm |= 0xFFFFF000; // Manual sign extend
                uint32_t addr = regs[rs1] + imm;
                
                // Memory Mapped IO Check
                if (addr == MMIO_PRINT) {
                    cout << (char)regs[rs2] << flush;
                } else {
                    *reinterpret_cast<uint32_t*>(&memory[addr]) = regs[rs2];
                }
                break;
            }
            case 0x63: { // B-Type (BEQ)
                int32_t imm = ((instr >> 7) & 0x1E) | ((instr >> 20) & 0x7E0) | 
                              ((instr << 4) & 0x800) | ((instr >> 19) & 0x1000);
                if (imm & 0x1000) imm |= 0xFFFFE000;
                
                if (funct3 == 0x0 && regs[rs1] == regs[rs2]) next_pc = pc + imm;
                break;
            }
            case 0x6F: { // J-Type (JAL)
                int32_t imm = ((instr >> 21) & 0x3FF) << 1 | ((instr >> 20) & 0x1) << 11 |
                              ((instr >> 12) & 0xFF) << 12 | (int32_t(instr) >> 31) << 20;
                regs[rd] = pc + 4;
                next_pc = pc + imm;
                break;
            }
            default:
                running = false;
                break;
        }

        regs[0] = 0; // Ensure x0 is always zero
        pc = next_pc;
    }

    void run() {
        while (running) {
            step();
        }
    }

    // --- MEMORY DUMP ---
    void dumpState() {
        cout << "\n--- CPU State Dump ---" << endl;
        cout << "PC: 0x" << hex << pc << endl;
        for (int i = 0; i < 32; i++) {
            cout << "x" << dec << i << ": 0x" << hex << regs[i] << "  ";
            if ((i + 1) % 4 == 0) cout << endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv << " <binary_file>" << endl;
        return 1;
    }

    RISCV_CPU myCPU;
    myCPU.loadBinary(argv[1]);
    myCPU.run();
    myCPU.dumpState();

    return 0;
}