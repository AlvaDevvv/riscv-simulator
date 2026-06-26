#include "simulator.hpp"
#include <fstream>
#include <iostream>
// Inicializamos la memoria con 16MB llenos de ceros
RiscVSimulator::RiscVSimulator() : memory(16 * 1024 * 1024, 0) { reset(); }

void RiscVSimulator::reset() {
  pc = 0;
  regs.fill(0); // Llena los 32 registros con 0
}

uint32_t RiscVSimulator::get_pc() const { return pc; }

void RiscVSimulator::set_pc(uint32_t value) { pc = value; }

uint32_t RiscVSimulator::get_reg(int index) const {
  if (index == 0)
    return 0; // En RISC-V, el registro x0 está "hardwired" a 0
  return regs[index];
}

void RiscVSimulator::set_reg(int index, uint32_t value) {
  if (index == 0)
    return; // Se ignora cualquier intento de escritura en x0
  regs[index] = value;
}

uint8_t RiscVSimulator::read_byte(uint32_t addr) const {
  if (addr >= memory.size()) {
    std::cerr << "Error: Lectura fuera de memoria en address 0x" << std::hex
              << addr << std::dec << std::endl;
    return 0;
  }
  return memory[addr];
}

void RiscVSimulator::write_byte(uint32_t addr, uint8_t value) {
  if (addr >= memory.size()) {
    std::cerr << "Error: Escritura fuera de memoria en address 0x" << std::hex
              << addr << std::dec << std::endl;
    return;
  }
  memory[addr] = value;
}

uint32_t RiscVSimulator::read_word(uint32_t addr) const {
  // Little-endian: el byte menos significativo está en la dirección más baja
  uint32_t b0 = read_byte(addr);
  uint32_t b1 = read_byte(addr + 1);
  uint32_t b2 = read_byte(addr + 2);
  uint32_t b3 = read_byte(addr + 3);
  return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void RiscVSimulator::write_word(uint32_t addr, uint32_t value) {
  // Little-endian: desarmamos la palabra de 32 bits en 4 bytes
  write_byte(addr, value & 0xFF);
  write_byte(addr + 1, (value >> 8) & 0xFF);
  write_byte(addr + 2, (value >> 16) & 0xFF);
  write_byte(addr + 3, (value >> 24) & 0xFF);
}

bool RiscVSimulator::load_program(const std::string &filename) {
  // Abrimos el archivo en modo binario y nos posicionamos al final (ios::ate)
  // para saber el tamaño
  std::ifstream file(filename, std::ios::binary | std::ios::ios_base::ate);

  if (!file.is_open()) {
    std::cerr << "Error: No se pudo abrir el archivo binario: " << filename
              << std::endl;
    return false;
  }

  // Obtenemos el tamaño del archivo en bytes
  std::streamsize size = file.tellg();

  // Regresamos el puntero del archivo al principio para empezar a leer
  file.seekg(0, std::ios::beg);

  // Validamos que el programa no exceda el espacio asignado en nuestra memoria
  if (size > static_cast<std::streamsize>(memory.size())) {
    std::cerr << "Error: El archivo binario es demasiado grande para la "
                 "memoria simula."
              << std::endl;
    return false;
  }

  // Leemos todo el contenido del archivo binario directamente en el inicio de
  // nuestro vector de memoria (address 0x00000000)
  if (file.read(reinterpret_cast<char *>(memory.data()), size)) {
    std::cout << "Programa '" << filename << "' cargado exitosamente (" << size
              << " bytes) en la direccion 0x00000000." << std::endl;

    // Al cargar un nuevo programa, reiniciamos el estado del PC y los registros
    reset();
    return true;
  }

  std::cerr << "Error al leer el contenido del archivo." << std::endl;
  return false;
}

void RiscVSimulator::step() {
  // 1. Fetch: Leer la instrucción de 32 bits apuntada por el Program Counter
  // (PC)
  uint32_t inst = read_word(pc);

  // 2. Actualizar estado: Avanzamos el PC en 4 bytes.
  // Se hace aquí por si la instrucción resulta ser un salto (branch o jump) que
  // modifique el PC nuevamente.
  pc += 4;

  // 3. Decode y Execute
  execute_instruction(inst);
}

void RiscVSimulator::execute_instruction(uint32_t inst) {
  uint32_t opcode = get_opcode(inst);
  uint32_t rd = get_rd(inst);
  uint32_t rs1 = get_rs1(inst);
  uint32_t rs2 = get_rs2(inst);
  uint32_t funct3 = get_funct3(inst);
  uint32_t funct7 = get_funct7(inst);

  // Guardamos el PC original de esta instrucción por si hacemos un salto
  // relativo
  uint32_t current_pc = pc - 4;

  switch (opcode) {
  case 0x33: { // TIPO R (Operaciones Aritmético/Lógicas)
    uint32_t val1 = get_reg(rs1);
    uint32_t val2 = get_reg(rs2);
    if (funct3 == 0x0) {
      if (funct7 == 0x00)
        set_reg(rd, val1 + val2); // add
      else if (funct7 == 0x20)
        set_reg(rd, val1 - val2); // sub
    } else if (funct3 == 0x1)
      set_reg(rd, val1 << (val2 & 0x1F)); // sll
    else if (funct3 == 0x2)
      set_reg(rd, (int32_t)val1 < (int32_t)val2 ? 1 : 0); // slt
    else if (funct3 == 0x3)
      set_reg(rd, val1 < val2 ? 1 : 0); // sltu
    else if (funct3 == 0x4)
      set_reg(rd, val1 ^ val2); // xor
    else if (funct3 == 0x5) {
      if (funct7 == 0x00)
        set_reg(rd, val1 >> (val2 & 0x1F)); // srl
      else if (funct7 == 0x20)
        set_reg(rd, (int32_t)val1 >> (val2 & 0x1F)); // sra
    } else if (funct3 == 0x6)
      set_reg(rd, val1 | val2); // or
    else if (funct3 == 0x7)
      set_reg(rd, val1 & val2); // and
    break;
  }

  case 0x13: { // TIPO I (Aritmético/Lógicas con Inmediato)
    uint32_t val1 = get_reg(rs1);
    int32_t imm = get_imm_I(inst);
    if (funct3 == 0x0)
      set_reg(rd, val1 + imm); // addi
    else if (funct3 == 0x2)
      set_reg(rd, (int32_t)val1 < imm ? 1 : 0); // slti
    else if (funct3 == 0x3)
      set_reg(rd, val1 < (uint32_t)imm ? 1 : 0); // sltiu
    else if (funct3 == 0x4)
      set_reg(rd, val1 ^ imm); // xori
    else if (funct3 == 0x6)
      set_reg(rd, val1 | imm); // ori
    else if (funct3 == 0x7)
      set_reg(rd, val1 & imm); // andi
    else if (funct3 == 0x1)
      set_reg(rd, val1 << (imm & 0x1F)); // slli
    else if (funct3 == 0x5) {
      if (funct7 == 0x00)
        set_reg(rd, val1 >> (imm & 0x1F)); // srli
      else if (funct7 == 0x20)
        set_reg(rd, (int32_t)val1 >> (imm & 0x1F)); // srai
    }
    break;
  }

  case 0x03: { // TIPO I (Load)
    uint32_t addr = get_reg(rs1) + get_imm_I(inst);
    if (funct3 == 0x0)
      set_reg(rd, (int8_t)read_byte(addr)); // lb
    else if (funct3 == 0x1)
      set_reg(rd,
              (int16_t)(read_byte(addr) | (read_byte(addr + 1) << 8))); // lh
    else if (funct3 == 0x2)
      set_reg(rd, read_word(addr)); // lw
    else if (funct3 == 0x4)
      set_reg(rd, read_byte(addr)); // lbu
    else if (funct3 == 0x5)
      set_reg(rd,
              (uint16_t)(read_byte(addr) | (read_byte(addr + 1) << 8))); // lhu
    break;
  }

  case 0x23: { // TIPO S (Store)
    uint32_t addr = get_reg(rs1) + get_imm_S(inst);
    uint32_t val2 = get_reg(rs2);
    if (funct3 == 0x0)
      write_byte(addr, val2 & 0xFF); // sb
    else if (funct3 == 0x1) {        // sh
      write_byte(addr, val2 & 0xFF);
      write_byte(addr + 1, (val2 >> 8) & 0xFF);
    } else if (funct3 == 0x2)
      write_word(addr, val2); // sw
    break;
  }

  case 0x63: { // TIPO B (Branch)
    int32_t val1 = get_reg(rs1);
    int32_t val2 = get_reg(rs2);
    int32_t imm = get_imm_B(inst);
    bool take_branch = false;
    if (funct3 == 0x0)
      take_branch = (val1 == val2); // beq
    else if (funct3 == 0x1)
      take_branch = (val1 != val2); // bne
    else if (funct3 == 0x4)
      take_branch = (val1 < val2); // blt
    else if (funct3 == 0x5)
      take_branch = (val1 >= val2); // bge
    else if (funct3 == 0x6)
      take_branch = ((uint32_t)val1 < (uint32_t)val2); // bltu
    else if (funct3 == 0x7)
      take_branch = ((uint32_t)val1 >= (uint32_t)val2); // bgeu

    if (take_branch) {
      pc = current_pc + imm; // Salto relativo al PC de la instrucción branch
    }
    break;
  }

  case 0x6F: { // TIPO J (jal)
    int32_t imm = get_imm_J(inst);
    set_reg(rd, current_pc + 4); // Guarda dirección de retorno
    pc = current_pc + imm;
    break;
  }

  case 0x67: { // TIPO I (jalr)
    int32_t imm = get_imm_I(inst);
    uint32_t target =
        (get_reg(rs1) + imm) & ~1; // Poner a 0 el bit menos significativo
    set_reg(rd, current_pc + 4);
    pc = target;
    break;
  }

  case 0x37: { // TIPO U (lui)
    set_reg(rd, get_imm_U(inst));
    break;
  }

  case 0x17: { // TIPO U (auipc)
    set_reg(rd, current_pc + get_imm_U(inst));
    break;
  }

  default:
    std::cerr << "Opcode detectado: 0x" << std::hex << opcode << std::dec
              << std::endl;
    break;
  }
}
