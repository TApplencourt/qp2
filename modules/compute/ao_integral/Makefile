CXX_FLAGS+="-std=c++11"
all: ao_integral

ao_integral: ao_integral.o libint_tools.o mono.o bi.o
	$(CXX) $(CXX_FLAGS) ao_integral.o libint_tools.o mono.o bi.o -lint2 -lzmq -o ao_integral 

ao_integral.o: ao_integral.cpp
	$(CXX) $(CXX_FLAGS) -c ao_integral.cpp

libint_tools.o: libint_tools.cpp
	$(CXX) $(CXX_FLAGS) -c libint_tools.cpp

mono.o: mono.cpp
	$(CXX) $(CXX_FLAGS) -c mono.cpp

bi.o: bi.cpp
	$(CXX) $(CXX_FLAGS) -c bi.cpp

clean:
	rm -f -- ao_integral ao_integral.o libint_tools.o mono.o bi.o
