LAB_DIR ?= ./calab-verilog/Lab2
CROSS_COMPILE ?= riscv64-elf-
TESTS = $(wildcard $(LAB_DIR)/TestDataTools/ExampleCode/RISCVTest_rv32ui/*.S) \
	    example.S \
		$(EXTRA_TESTS)

# verilator
VERILOG_SRC_DIR ?= $(LAB_DIR)/SourceCode/PART-code
VERILOG_SRCS = $(shell find $(VERILOG_SRC_DIR) -name '*.v' -type f)
TOP = RV32ICore
SEED ?= 12345
TB = ./tb

# test
TEST_ELFS = $(patsubst %.S, %.elf, $(TESTS))
TEST_BINS = $(patsubst %.S, %.bin, $(TESTS))
TEST_DUMPS = $(patsubst %.S, %.dump, $(TESTS))

# assembler
# default cache capacity: 0x4000
TEXT_BASE ?= 0x0000
DATA_BASE ?= 0x3000
# disable gp-relative addressing
CC = $(CROSS_COMPILE)gcc -march=rv32g -mabi=ilp32 -nostdlib \
                        -Wl,--section-start=.text=$(TEXT_BASE),--section-start=.data=$(DATA_BASE) \
						-Wl,--no-relax 	
OBJCOPY = $(CROSS_COMPILE)objcopy -O binary
OBJDUMP = $(CROSS_COMPILE)objdump -b binary -m riscv:rv32 -D

# run tb
RUN_TB = $(TB) +verilator+seed+$(SEED) +verilator+rand+reset+2 \
			-textbase $(TEXT_BASE) -database $(DATA_BASE)

.PHONY: all clean test dump

all: test dump

test: $(TEST_BINS) $(TB)
	@for bin in $(TEST_BINS); do \
		echo "Running test for $$bin"; \
		$(RUN_TB) -bin $$bin; \
		echo "Done, waveform dumped"; \
	done

dump: $(TEST_DUMPS)

%.dump: %.bin
	$(OBJDUMP) $< > $@

%.bin: %.elf
	$(OBJCOPY) $< $@

%.elf: %.S
	$(CC) $< -o $@

# warnings come from lab framework
$(TB): $(VERILOG_SRCS) tb.cc
	verilator --cc $(VERILOG_SRCS) -I$(VERILOG_SRC_DIR) \
			-top $(TOP) \
			--exe tb.cc \
			--trace \
			--trace-max-array 10000 \
			-Wno-fatal
	make CXXFLAGS=-std=c++17 -C obj_dir -f V$(TOP).mk -j`nproc`
	cp obj_dir/V$(TOP) $(TB)

clean:
	rm -rf obj_dir $(TB) $(TEST_BINS)

