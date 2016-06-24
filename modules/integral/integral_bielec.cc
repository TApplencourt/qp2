#include <libint2.hpp>
#include <iostream> 
using namespace std;

int main(int argc, char * argv[]) {



    /*** =========================== ***/
    /*** initialize molecule         ***/
    /*** =========================== ***/
    std::string xyz_path = argv[1];
    std::ifstream input_file(xyz_path);
    std::vector < libint2::Atom > atoms = libint2::read_dotxyz(input_file);

    /*** =========================== ***/
    /*** create basis set            ***/
    /*** =========================== ***/
    std::string basis_name = argv[2];
    //export LIBINT_DATA_PATH="$qp_root"/usr/share/libint/2.1.0-beta/basis/

    libint2::BasisSet obs(basis_name, atoms);
    obs.set_pure(false); // use cartesian gaussians
    
    auto shell2bf = obs.shell2bf(); // maps shell index to basis function index
                                    // shell2bf[0] = index of the first basis function in shell 0
                                    // shell2bf[1] = index of the first basis function in shell 1
                                    // ...

    libint2::init(); // safe to use libint now

    libint2::TwoBodyEngine < libint2::Coulomb > coulomb_engine(obs.max_nprim(), obs.max_l(), 0);

    for (auto s1 = 0; s1 != obs.size(); ++s1) {
        std::cout << s1 << "/" << obs.size() << std::endl;
        for (auto s2 = 0; s2 != obs.size(); ++s2)
            for (auto s3 = 0; s3 != obs.size(); ++s3)
                for (auto s4 = 0; s4 != obs.size(); ++s4) {

                    const auto * buf = coulomb_engine.compute(obs[s1], obs[s2], obs[s3], obs[s4]);

                    auto bf1 = shell2bf[s1];  // first basis function in first shell
                    auto n1 = obs[s1].size(); // number of basis functions in first shell
                    auto bf2 = shell2bf[s2];  // first basis function in second shell
                    auto n2 = obs[s2].size(); // number of basis functions in second shell
                    auto bf3 = shell2bf[s3];  // first basis function in third shell
                    auto n3 = obs[s3].size(); // number of basis functions in third shell
                    auto bf4 = shell2bf[s4];  // first basis function in fourth shell
                    auto n4 = obs[s4].size(); // number of basis functions in fourth shell
                
                    // integrals are packed into buf in row-major (C) form
                    // this iterates over integrals in this order
                    for(auto f1=0; f1!=n1; ++f1)
                        for(auto f2=0; f2!=n2; ++f2)
                          for(auto f3=0; f3!=n3; ++f3)
                            for(auto f4=0; f4!=n4; ++f4) {
                                auto f1234 = f1*n2*n3*n4+f2*n3*n4+f3*n4+f4;
                                cout << "  " << bf1+f1 << " " << bf2+f2 << " " << bf3+f3 << " " << bf4+f4 << " " << buf[f1234] << endl;
                            }
                }
    }

    libint2::finalize(); // do not use libint after this

    return 0;
}