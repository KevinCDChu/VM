CXX = g++
CXXFLAGS = -std=c++14 -Wall -MMD -g
EXEC = vm
OBJECTS = main.o view/view.o utils.o model/model.o controller/controller.o controller/action.o view/printing.o model/undo.o
DEPENDS = ${OBJECTS:.o=.d}

${EXEC}: ${OBJECTS}
	${CXX} ${OBJECTS} -o ${EXEC} -lncurses

-include ${DEPENDS}

.PHONY: clean

clean:
	rm ${OBJECTS} ${DEPENDS} ${EXEC}
