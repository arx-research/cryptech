APPLICATION	= uart-test
SRCDIR  	= src/$(APPLICATION)

all: target

world: target

target:
	make -C $(SRCDIR)

flash-target:
	make -C $(SRCDIR) flash-target

clean:
	make -C $(SRCDIR) clean

really-clean:
	make -C $(SRCDIR) really-clean
