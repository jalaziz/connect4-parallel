CC   = g++
FLAG = -g -pthread -Iboard
SRCS = dropfour-text.cpp ioface.cpp board/t_board.cpp

GUIFLAG = -g -pthread -Iboard
GUISRCS = dropfour-gui.cpp gui.cpp board/t_board.cpp
GUILIB = -lGL -lglut -lGLU

all: drop4txt

drop4txt: ${SRCS}
	${CC} ${FLAG} -o drop4txt ${SRCS}

gui: $(GUISRCS)
	$(CC) $(GUIFLAG) -o drop4gui $(GUISRCS) $(GUILIB)

clean:
	rm -rf *.o drop4txt drop4gui
