CC=gcc
CFLAGS=-c
LDFLAGS=
 
modexp_fpga_model: modexp_fpga_model_pe.o modexp_fpga_model_montgomery.o modexp_fpga_model.o
	$(CC) $(LDFLAGS) modexp_fpga_model_pe.o modexp_fpga_model_montgomery.o modexp_fpga_model.o -o modexp_fpga_model

modexp_fpga_model_pe.o: modexp_fpga_model_pe.cpp modexp_fpga_model.h modexp_fpga_model_pe.h
	$(CC) $(CFLAGS) modexp_fpga_model_pe.cpp

modexp_fpga_model_montgomery.o: modexp_fpga_model_montgomery.cpp modexp_fpga_model.h modexp_fpga_model_pe.h modexp_fpga_model_montgomery.h
	$(CC) $(CFLAGS) modexp_fpga_model_montgomery.cpp

modexp_fpga_model.o: modexp_fpga_model.cpp modexp_fpga_model.h modexp_fpga_model_montgomery.h test/modexp_fpga_model_vectors.h
	$(CC) $(CFLAGS) modexp_fpga_model.cpp

clean:
	rm -f modexp_fpga_model *.o
