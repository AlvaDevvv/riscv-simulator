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
    return 1; // Terminamos si hay error al cargar
  }

  string line;
  cout << "Escribe 'exit' para salir o usa comandos: pc, step, regs, mem."
       << "\n";

  // Bucle interactivo (REPL)
  while (true) {
    cout << "> ";
    if (!getline(cin, line))
      break; // Salimos si se cierra el flujo de entrada

    stringstream ss(line);
    string cmd;
    ss >> cmd;

    if (cmd == "exit" || cmd == "quit") {
      cout << "See you next time..." << "\n";
      break;
    } else if (cmd == "step") {
      sim.step();
      cout << "Ejecutando instrucción." << "\n";
    } else if (cmd == "pc") {
      // Imprime el PC en formato hexadecimal de 8 dígitos
      cout << "pc 0x" << setfill('0') << setw(8) << hex << sim.get_pc() << dec
           << "\n";
    } else if (cmd == "regs") {
      string reg_name;
      // Si el usuario especifica registros específicos (ej. regs x5 x14)
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
        // Si solo escribe "regs", imprimimos los 32 registros
        for (int i = 0; i < 32; i++) {
          cout << "x" << setfill(' ') << setw(2) << dec << i << "=0x"
               << setfill('0') << setw(8) << hex << sim.get_reg(i) << " ";
          if ((i + 1) % 4 == 0)
            cout << "\n"; // 4 columnas
        }
        cout << dec; // Regresamos a decimal por defecto
      }
    } else if (cmd == "mem") {
      string start_str, end_str;
      // Espera dos direcciones en hexadecimal (ej. mem 0x1000 0x1003)
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
          cout << "Error: Las direcciones de memoria deben ser valores "
                  "hexadecimales válidos."
               << "\n";
        }
      } else {
        cout << "Uso: mem <inicio_hex> <fin_hex>" << "\n";
      }
    } else if (!cmd.empty()) {
      cout << "Comando desconocido. Usa: pc, step, regs [xN...], mem "
              "<inicio> <fin>, exit"
           << "\n";
    }
  }
  return 0;
}
