all: ao_integral ;

#ao_integral: ao_integral.o libint_tools.o
#	g++ -std=c++0x -DHAVE_CONFIG_H  -lzmq -lint2 -o ao_integral ao_integral.o libint_tools.o

ao_integral: ao_integral.o
	g++ -lint2 -lzmq -o ao_integral  ao_integral.o

ao_integral.o: ao_integral.cpp
	g++ -c ao_integral.cpp -o ao_integral.o

libint_tools.o: libint_tools.cpp
	g++ -std=c++0x -DHAVE_CONFIG_H  -c libint_tools.cpp

clean:
	rm -f ao_integral ao_integral.o libint_tools.o libint_tools