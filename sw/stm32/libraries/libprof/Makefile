LIB = libprof.a

OBJS = gmon.o profil.o profiler.o

# Don't profile the profiling code, because that way lies madness (and recursion).
CFLAGS := $(subst -pg,,$(CFLAGS))

all: $(LIB)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJS)
	$(AR) -r $@ $^

clean:
	rm -f $(OBJS) $(LIB)
