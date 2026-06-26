#include "simulator.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
int main(int argc, char *argv[]) {
  // Verificamos que se pase el archivo binario como argumento
  if (argc != 2) {
    cerr << "Uso: " << argv[0] << " <programa.bin>" << "\n";
    return 1;
  }

  RiscVSimulator sim;

  // Intentamos cargar el programa en memoria
  if (!sim.load_program(argv[1])) {
    return 1;
  }

  string line;
  string last_line = ""; // Guarda el último comando ingresado

  cout << "Escribe 'exit' para salir o usa comandos: run, pc, step, regs, mem."
       << "\n";

  // Bucle interactivo (REPL)
  while (true) {
    cout << "> ";
    if (!getline(cin, line))
      break;

    // CORRECCIÓN: Si el usuario presiona Enter (línea vacía), usamos el último
    // comando
    if (line.empty()) {
      line = last_line;
      // Si es la primera vez y presionan Enter, no hacemos nada
      if (line.empty())
        continue;
    } else {
      last_line = line; // Guardamos el nuevo comando
    }

    stringstream ss(line);
    string cmd;
    ss >> cmd;

    if (cmd == "exit" || cmd == "quit") {
      cout << "See you next time..." << "\n";
      break;
    } else if (cmd == "step") {
      sim.step();
      cout << "Ejecutando instrucción. (PC: 0x" << hex << sim.get_pc() << dec
           << ")" << "\n";
    } else if (cmd == "run") {
      // NUEVO COMANDO: Ejecuta automáticamente hasta encontrar el final del
      // programa (memoria en 0)
      cout << "Ejecutando programa automáticamente..." << "\n";
      int steps = 0;
      while (steps < 100000) { // Límite de seguridad
        // Leemos la instrucción actual. Si es 0x00000000 asumimos que terminó
        // el código
        uint32_t inst = sim.read_word(sim.get_pc());
        if (inst == 0x00000000)
          break;

        sim.step();
        steps++;
      }
      cout << "Ejecución finalizada en " << steps << " pasos."
           << "\n";
    } else if (cmd == "pc") {
      cout << "pc 0x" << setfill('0') << setw(8) << hex << sim.get_pc() << dec
           << "\n";
    } else if (cmd == "regs") {
      string reg_name;
      if (ss >> reg_name) {
        do {
          if (reg_name.size() > 1 && reg_name[0] == 'x') {
            int reg_num = stoi(reg_name.substr(1));
            if (reg_num >= 0 && reg_num < 32) {
              cout << reg_name << "=0x" << setfill('0') << setw(8) << hex
                   << sim.get_reg(reg_num) << dec << "\n";
            }
          }
        } while (ss >> reg_name);
      } else {
        for (int i = 0; i < 32; i++) {
          cout << "x" << setfill(' ') << setw(2) << dec << i << "=0x"
               << setfill('0') << setw(8) << hex << sim.get_reg(i) << " ";
          if ((i + 1) % 4 == 0)
            cout << "\n";
        }
        cout << dec;
      }
    } else if (cmd == "mem") {
      string start_str, end_str;
      if (ss >> start_str >> end_str) {
        try {
          uint32_t start = stoul(start_str, nullptr, 16);
          uint32_t end = stoul(end_str, nullptr, 16);
          cout << "Memoria (0x" << hex << start << " - 0x" << end << "): ";
          for (uint32_t addr = start; addr <= end; addr++) {
            cout << setfill('0') << setw(2) << hex << (int)sim.read_byte(addr)
                 << " ";
          }
          cout << dec << "\n";
        } catch (...) {
          cout << "Error: Las direcciones deben ser valores hexadecimales."
               << "\n";
        }
      } else {
        cout << "Uso: mem <inicio_hex> <fin_hex>" << "\n";
      }
    } else {
      cout << "Comando desconocido. Usa: run, pc, step, regs [xN...], mem "
              "<inicio> <fin>, exit"
           << "\n";
    }
  }
  return 0;
}
