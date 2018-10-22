CC=gcc
CFLAGS=-c
LDFLAGS=
 
ecdsa_model_fpga: ecdsa_model_fpga.o fpga_curve.o fpga_modular.o fpga_lowlevel.o fpga_util.o
	$(CC) $(LDFLAGS) ecdsa_model_fpga.o fpga_curve.o fpga_modular.o fpga_lowlevel.o fpga_util.o -o ecdsa_model_fpga

ecdsa_model_fpga.o: ecdsa_model_fpga.cpp ecdsa_model.h fpga_curve.h fpga_modular.h fpga_lowlevel.h fpga_util.h
	$(CC) $(CFLAGS) ecdsa_model_fpga.cpp
	
fpga_curve.o: fpga_curve.cpp ecdsa_model.h fpga_lowlevel.h fpga_modular.h fpga_curve.h fpga_util.h
	$(CC) $(CFLAGS) fpga_curve.cpp

fpga_modular.o: fpga_modular.cpp ecdsa_model.h fpga_lowlevel.h fpga_modular.h
	$(CC) $(CFLAGS) fpga_modular.cpp
	
fpga_lowlevel.o: fpga_lowlevel.cpp ecdsa_model.h fpga_lowlevel.h
	$(CC) $(CFLAGS) fpga_lowlevel.cpp
	
fpga_util.o: fpga_util.cpp ecdsa_model.h fpga_lowlevel.h fpga_util.h
	$(CC) $(CFLAGS) fpga_util.cpp

clean:
	rm -f ecdsa_model_fpga *.o
