CC   = g++
FLAG = -g -Iboard
SRCS = dropfour-text.cpp ioface.cpp board/board.cpp

all: drop4txt

drop4txt: ${SRCS}
	${CC} ${FLAG} -o drop4txt ${SRCS}

clean:
	rm -rf *.o
