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