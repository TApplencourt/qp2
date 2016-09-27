all: ao_integral ;

ao_integral: ao_integral.o libint_tools.o mo_elec.o
	g++ ao_integral.o libint_tools.o mo_elec.o -lint2 -lzmq -o ao_integral 

ao_integral.o: ao_integral.cpp
	g++ -c ao_integral.cpp

libint_tools.o: libint_tools.cpp
	g++ -c libint_tools.cpp

mo_elec.o: mo_elec.cpp
	g++ -c mo_elec.cpp

clean:
	rm -f ao_integral ao_integral.o libint_tools.o mo_elec.o