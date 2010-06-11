CC   = g++
FLAG = -g -pthread -Iboard
SRCS = dropfour-text.cpp ioface.cpp board/t_board.cpp

GUISRCS = dropfour-gui.cpp gui.cpp board/t_board.cpp
GUILIB = -lGL -lglut -lGLU

all: txt gui

txt: ${SRCS}
	${CC} ${FLAG} -o drop4txt ${SRCS}

gui: ${GUISRCS}
	${CC} ${FLAG} -o drop4gui ${GUISRCS} ${GUILIB}

clean:
	rm -rf *.o drop4txt drop4gui
