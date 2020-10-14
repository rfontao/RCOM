NAME_SERVER = read
SRC_SERVER = read.c common.c state_machine.c
OBJ_SERVER = ${SRC_SERVER:.c=.o}

NAME_CLIENT = write
SRC_CLIENT = write.c common.c state_machine.c
OBJ_CLIENT = ${SRC_CLIENT:.c=.o}

# compiler and linker
CC = gcc
# flags
CFLAGS = -Wall

all: options rcom

options:
	@echo rcom build options:
	@echo "CFLAGS = ${CFLAGS}"
	@echo "CC     = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

rcom: ${OBJ_SERVER} ${OBJ_CLIENT}
	${CC} ${CFLAGS} -o ${NAME_SERVER} ${OBJ_SERVER}
	${CC} ${CFLAGS} -o ${NAME_CLIENT} ${OBJ_CLIENT}
	rm *.o

clean:
	rm -f ${NAME_SERVER} ${OBJ_SERVER} ${NAME_CLIENT} ${OBJ_CLIENT}

.PHONY: all options clean rcom