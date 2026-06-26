# RISC-V Simulator

Simulador básico de un procesador RISC-V desarrollado en C++.

## Requisitos

- Compilador compatible con C++ (por ejemplo, `g++`).
- Un archivo `.bin` de entrada con el programa a ejecutar.

## Compilación

Ejecuta el siguiente comando en la carpeta del proyecto:

```bash
g++ main.cpp simulator.cpp -o awesome-simulator
```

## Ejecución

El simulador **siempre debe ejecutarse acompañado de un archivo `.bin` como argumento**:

```bash
./awesome-simulator archivo.bin
```

## Ejemplo

```bash
./awesome-simulator riscvtest.bin
```
