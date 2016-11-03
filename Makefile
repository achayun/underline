CFLAGS += -Wall -Werror -g -I./src  -Os

SRC = src/underline.c src/vt.c
OBJ = $(addprefix build/,$(SRC:.c=.o))

#Object dependencies
#Extra explicit include dirs to remove makedepend warnings
#Not perfect, as makedepend will only be effective the next time we run
#include build/Makefile.depend
#DEPEND_CFLAGS= $(CFLAGS) -I/usr/include -I/usr/include/linux
#build/Makefile.depend:
#	touch $@
#	makedepend -f $@ -p build/ -- $(DEPEND_CFLAGS) -- $(SRC)


#Implicit rule for out-of-tree build
build/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o $@

#all: $(DIR) build/Makefile.depend libunderline.a
all: $(DIR)  libunderline.a examples

libunderline.a: $(OBJ)
	$(AR) rcs $@ $^

examples: build/examples/shell build/examples/posix_aio

.PHONY: examples

build/examples/shell: libunderline.a build/examples/shell.o

#Only for posix_aio
LDFLAGS=-lrt
build/examples/posix_aio: libunderline.a build/examples/posix_aio.o


clean:
	$(RM) -r build/* libunderline.a

.PHONY: clean


