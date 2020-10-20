# NAME_SERVER = read
# SRC_SERVER = read.c common.c state_machine.c
# OBJ_SERVER = ${SRC_SERVER:.c=.o}

# NAME_CLIENT = write
# SRC_CLIENT = write.c common.c state_machine.c
# OBJ_CLIENT = ${SRC_CLIENT:.c=.o}

NAME_APPLICATION = application
SRC_APPLICATION = application.c read.c write.c common.c state_machine.c
OBJ_APPLICATION = ${SRC_APPLICATION:.c=.o}

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

rcom: ${OBJ_APPLICATION}
	${CC} ${CFLAGS} -o ${NAME_APPLICATION} ${OBJ_APPLICATION}
	rm *.o

clean:
	rm -f ${NAME_APPLICATION} ${OBJ_APPLICATION}

.PHONY: all options clean rcom