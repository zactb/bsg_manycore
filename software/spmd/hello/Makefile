
bsg_tiles_X = 2
bsg_tiles_Y = 2

include ../Makefile.include

all: hello.run

phasedetector_test.riscv:  phasedetector_test.o $(SPMD_COMMON_OBJECTS) ../common/crt.o
	$(RISCV_LINK) $< -o $@ $(RISCV_LINK_OPTS)

fir.riscv: fir.o $(SPMD_COMMON_OBJECTS) ../common/crt.o
	$(RISCV_LINK) $< -o $@ $(RISCV_LINK_OPTS)

cordiccart2pol.riscv: cordiccart2pol.o $(SPMD_COMMON_OBJECTS) ../common/crt.o
	$(RISCV_LINK) $< -o $@ $(RISCV_LINK_OPTS)

clean:
	-rm *.o *.jou *.log *.pb bsg_rom_*.v *.riscv *.wdb *.bin *.hex
	-rm -rf xsim.dir

hello.o: Makefile










