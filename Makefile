.PHONY: all

all: tm_interpreter tm_reducer

tm_interpreter: tm_interpreter.cpp turing_machine.cpp turing_machine.h
	g++ -Wall -Wshadow $(filter %.cpp,$^) -o $@

tm_reducer: tm_reducer.cpp turing_machine.cpp turing_machine.h
	g++ -Wall -Wshadow $(filter %.cpp,$^) -o $@

clean:
	rm -rf tm_interpreter tm_reducer *~
