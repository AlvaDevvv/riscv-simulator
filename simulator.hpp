#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class RiscVSimulator {
private:
  uint32_t pc;
  std::array<uint32_t, 32> regs;

  // Memoria plana byte-addressable.
  // Usaremos 16 MB, suficiente para los programas de prueba cargados en 0x0.
  std::vector<uint8_t> memory;

  // Métodos auxiliares para extraer los campos de la instrucción RISC-V
  uint32_t get_opcode(uint32_t inst) const { return inst & 0x7F; }
  uint32_t get_rd(uint32_t inst) const { return (inst >> 7) & 0x1F; }
  uint32_t get_funct3(uint32_t inst) const { return (inst >> 12) & 0x7; }
  uint32_t get_rs1(uint32_t inst) const { return (inst >> 15) & 0x1F; }
  uint32_t get_rs2(uint32_t inst) const { return (inst >> 20) & 0x1F; }
  uint32_t get_funct7(uint32_t inst) const { return (inst >> 25) & 0x7F; }

  // Extracción de valores inmediatos (Sign-extended)
  int32_t get_imm_I(uint32_t inst) const {
    return static_cast<int32_t>(inst) >> 20;
  }
  int32_t get_imm_S(uint32_t inst) const {
    return (static_cast<int32_t>(inst & 0xFE000000) >> 20) |
           ((inst >> 7) & 0x1F);
  }
  int32_t get_imm_B(uint32_t inst) const {
    return (static_cast<int32_t>(inst & 0x80000000) >> 19) |
           ((inst & 0x80) << 4) | ((inst >> 20) & 0x7E0) | ((inst >> 7) & 0x1E);
  }
  uint32_t get_imm_U(uint32_t inst) const { return inst & 0xFFFFF000; }
  int32_t get_imm_J(uint32_t inst) const {
    return (static_cast<int32_t>(inst & 0x80000000) >> 11) | (inst & 0xFF000) |
           ((inst >> 9) & 0x800) | ((inst >> 20) & 0x7FE);
  }

  // Función interna para el Decode & Execute
  void execute_instruction(uint32_t inst);

public:
  // Constructor
  RiscVSimulator();

  // Gestión básica del estado
  void reset();
  uint32_t get_pc() const;
  void set_pc(uint32_t value);
  uint32_t get_reg(int index) const;
  void set_reg(int index, uint32_t value);

  // Operaciones de Memoria (Little-endian)
  uint8_t read_byte(uint32_t addr) const;
  void write_byte(uint32_t addr, uint8_t value);

  // RISC-V es de 32 bits, por lo que leemos/escribimos "words" (4 bytes)
  uint32_t read_word(uint32_t addr) const;
  void write_word(uint32_t addr, uint32_t value);
  // para cargar el programa compilado
  bool load_program(const std::string &filename);
  // ejecutar un paso de máquina
  void step();
};

#endif
