CC=gcc
CFLAGS=-c
LDFLAGS=
 
ecdh_fpga_model: ecdh_fpga_model.o fpga_curve.o fpga_modular.o fpga_lowlevel.o fpga_util.o
	$(CC) $(LDFLAGS) ecdh_fpga_model.o fpga_curve.o fpga_modular.o fpga_lowlevel.o fpga_util.o -o ecdh_fpga_model

ecdh_fpga_model.o: ecdh_fpga_model.cpp ecdh_fpga_model.h fpga_curve.h fpga_modular.h fpga_lowlevel.h fpga_util.h
	$(CC) $(CFLAGS) ecdh_fpga_model.cpp
	
fpga_curve.o: fpga_curve.cpp ecdh_fpga_model.h fpga_lowlevel.h fpga_modular.h fpga_curve.h fpga_util.h
	$(CC) $(CFLAGS) fpga_curve.cpp

fpga_modular.o: fpga_modular.cpp ecdh_fpga_model.h fpga_lowlevel.h fpga_modular.h
	$(CC) $(CFLAGS) fpga_modular.cpp
	
fpga_lowlevel.o: fpga_lowlevel.cpp ecdh_fpga_model.h fpga_lowlevel.h
	$(CC) $(CFLAGS) fpga_lowlevel.cpp
	
fpga_util.o: fpga_util.cpp ecdh_fpga_model.h fpga_lowlevel.h fpga_util.h
	$(CC) $(CFLAGS) fpga_util.cpp

clean:
	rm -f ecdh_fpga_model *.o
