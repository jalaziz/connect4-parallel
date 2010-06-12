CC   = g++
FLAG = -g -pthread -Iboard
SRCS = dropfour-text.cpp ioface.cpp board/t_board.cpp

GUISRCS = dropfour-gui.cpp gui.cpp board/t_board.cpp
GUILIB = -lGL -lglut -lGLU

BINDIR= bin

all: drop4txt

txt: ${SRCS}
	if [ ! -d "${BINDIR}" ]; then mkdir bin; fi
	${CC} ${FLAG} -o ${BINDIR}/drop4txt ${SRCS}

gui: ${GUISRCS}
	if [ ! -d "${BINDIR}" ]; then mkdir bin; fi
	${CC} ${FLAG} -o ${BINDIR}/drop4gui ${GUISRCS} ${GUILIB}

clean:
	rm -rf ${BINDIR} *.o
