all: ao_integral ;

ao_integral: ao_integral.o libint_tools.o
	g++ ao_integral.o libint_tools.o -lint2 -lzmq -o ao_integral 

ao_integral.o: ao_integral.cpp
	g++ -c ao_integral.cpp

libint_tools.o: libint_tools.cpp
	g++ -c libint_tools.cpp

clean:
	rm -f ao_integral ao_integral.o libint_tools.o libint_tools