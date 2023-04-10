# Simmulate calab-verilog with Verilator

## Description

This repo allows you to simulate calab-verilog of USTC CompArch course with Verilator instead of Vivado. Building the testbench takes less than 20s and then the simulation can be done almost instantly. It also supports generating waveforms from assmbly sources directly with a simple `make`. You can use waveform viewers like GTKWave to display the `*.vcd`  files.

## Dependencies

- Verilator
- A cross-compiler for RISCV32 bare-metal target

If you're using Archlinux, you can install those with

``` shell
# pacman -S verilator riscv64-elf-gcc
```

## Usage

- Use `git submodule update --init` to fetch calab-verilog. Or set `LAB_DIR` to the path to the lab directory manually.
- Set `CROSS_COMILE` to prefix of your cross-compiler. If you're using Archlinux, it is `riscv64-elf-`.
- The 3 testcases from calab-verilog will be added to `TESTS` automatically. You only need to specify the path to your custom testcase with `EXTRA_TESTS` if any.

Here is an example.

``` shell
$ make LAB_DIR=/path/to/calab-verilog/Lab2 \
       CROSS_COMPILE=riscv64-elf- \
       EXTRA_TESTS=path/to/source.S
```

You should get a `source.dump` and a `source.vcd` in the same directory as the `source.S`.
