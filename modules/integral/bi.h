#include "libint_tools.h"
#include <vector>
#include <unordered_set>


struct Collector_Info
{ 
  void * collector_socket;
  int worker_id;

};

/**
    For a (ij|kl) integral quarter (starting with 0) return a unique id.

    @param i,j,k,l  the indice of the basis shell function
    @return A uniq key
*/
long int bielec_integrals_index(const int, const int, const int, const int);

/**
    For a (mn|rs) shell integrals stored in a buffer
    append if to the vector buffer_i, buffer_value

     @param map the map array
            n{1,4} the number of AO in (mn,rs) respectively
            bf{1,4}_first the index of the first basis function in thhis shell
*/
void append_buffer(std::vector<long int>&,
                   std::vector<double>&,
                   const std::vector<double>&,
                   const double precision,
                   const int, const int, const int, const int,
                   const int, const int, const int, const int,
                   const double*, int&);

Collector_Info initializeCollector(void*, void*);
void sendBiInt(void*, Collector_Info, Atom_Obs);
void sendBiTask(void*, Atom_Obs);
void filinizeCollector(void*, Collector_Info);