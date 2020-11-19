NAME_DOWNLOAD = download
SRC_DOWNLOAD = download.c
OBJ_DOWNLOAD = ${SRC_DOWNLOAD:.c=.o}

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

rcom: ${OBJ_DOWNLOAD}
	${CC} ${CFLAGS} -o ${NAME_DOWNLOAD} ${OBJ_DOWNLOAD}
	rm *.o

clean:
	rm -f ${NAME_DOWNLOAD} ${OBJ_DOWNLOAD}

.PHONY: all options clean rcom