CXX = g++
CXXFLAGS = -std=c++14 -Wall -MMD -g
EXEC = test
OBJECTS = test.o view.o utils.o model.o
DEPENDS = ${OBJECTS:.o=.d}

${EXEC}: ${OBJECTS}
	${CXX} ${OBJECTS} -o ${EXEC} -lncurses

-include ${DEPENDS}

.PHONY: clean

clean:
	rm ${OBJECTS} ${DEPENDS} ${EXEC}
