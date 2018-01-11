.SUFFIXES: .o
.PHONY: clean

CFLAGS += -std=c99 -pedantic -Wall -Wextra -Werror

libc8asm.a: c8asm.o
	ar -cvq $@ $^

clean:
	$(RM) libc8asm.a c8asm.o
