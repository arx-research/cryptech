CURVE25519_OBJS = curve25519/curve25519_fpga_model.o \
	curve25519/curve25519_fpga_lowlevel.o \
	curve25519/curve25519_fpga_multiword.o \
	curve25519/curve25519_fpga_modular.o \
	curve25519/curve25519_fpga_microcode.o

X25519_OBJS = x25519/x25519_fpga_model.o \
	x25519/x25519_fpga_curve_abstract.o \
	x25519/x25519_fpga_curve_microcode.o

ED25519_OBJS = ed25519/ed25519_fpga_model.o \
	ed25519/ed25519_fpga_curve_abstract.o \
	ed25519/ed25519_fpga_curve_microcode.o

CPPFLAGS = -Wall

CPPFLAGS += -I./curve25519
CPPFLAGS += -I./x25519
CPPFLAGS += -I./ed25519
CPPFLAGS += -I./vectors/x25519
CPPFLAGS += -I./vectors/ed25519

all: x25519_fpga_model ed25519_fpga_model

x25519_fpga_model: $(CURVE25519_OBJS) $(X25519_OBJS)
	$(CC) $(LDFLAGS) $(CURVE25519_OBJS) $(X25519_OBJS) -o $@

ed25519_fpga_model: $(CURVE25519_OBJS) $(ED25519_OBJS)
	$(CC) $(LDFLAGS) $(CURVE25519_OBJS) $(ED25519_OBJS) -o $@
		
clean:
	rm -f x25519_fpga_model
	rm -f ed25519_fpga_model
	rm -f curve25519/*.o
	rm -f x25519/*.o
	rm -f ed25519/*.o
