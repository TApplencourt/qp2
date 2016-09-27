#include <libint2.hpp>
#include "libint_tools.h"
#include "mono.h" //Atom_Obs 
#include <vector>

void sendMono(void* zezfio_socket, Atom_Obs AO){


    libint2::init();

    libint2::BasisSet obs = AO.obs;


    libint2::OneBodyEngine overlap_engine(libint2::OneBodyEngine::overlap, // will compute overlap ints
                                          obs.max_nprim(), // max # of primitives in shells this engine will accept
                                          obs.max_l()      // max angular momentum of shells this engine will accept
                                         );

    libint2::OneBodyEngine kinetic_engine(libint2::OneBodyEngine::kinetic,obs.max_nprim(), obs.max_l());
    libint2::OneBodyEngine nuclear_engine(libint2::OneBodyEngine::nuclear,obs.max_nprim(), obs.max_l());
    nuclear_engine.set_params(AO.atoms); // atoms specifies the charge and position of each nucleus


    auto shell2bf = obs.shell2bf(); // maps shell index to basis function index
                                    // shell2bf[0] = index of the first basis function in shell 0
                                    // shell2bf[1] = index of the first basis function in shell 1


    std::vector<double> renorm = AO.renorm;

    for(auto s1=0; s1!=obs.size(); ++s1) {
      for(auto s2=0; s2!=obs.size(); ++s2) {
    
        const auto* ints_shellset_overlap = overlap_engine.compute(obs[s1], obs[s2]);
        const auto* ints_shellset_nuclear = nuclear_engine.compute(obs[s1], obs[s2]);
        const auto* ints_shellset_kinetic = kinetic_engine.compute(obs[s1], obs[s2]);
    
        auto bf1 = shell2bf[s1];  // first basis function in first shell
        auto n1 = obs[s1].size(); // number of basis functions in first shell
        auto bf2 = shell2bf[s2];  // first basis function in second shell
        auto n2 = obs[s2].size(); // number of basis functions in second shell
    
        for(auto f1=0; f1!=n1; ++f1)
          for(auto f2=0; f2!=n2; ++f2)
          {
            const double fn2 = renorm[bf1] * renorm[bf2] * ints_shellset_overlap[f1*n2+f2];
            std::cout << "  " << bf1+f1 << " " << bf2+f2 << " " << fn2 << std::endl;
          }
      }
    }

   libint2::finalize();

}