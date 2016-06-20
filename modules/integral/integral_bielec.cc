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

    libint2::init(); // safe to use libint now

    libint2::TwoBodyEngine < libint2::Coulomb > coulomb_engine(obs.max_nprim(), obs.max_l(), 0);

    for (auto s1 = 0; s1 != obs.size(); ++s1) {
        std::cout << s1 << "/" << obs.size() << std::endl;
        for (auto s2 = 0; s2 != obs.size(); ++s2) {
            for (auto s3 = 0; s3 != obs.size(); ++s3) {
                for (auto s4 = 0; s4 != obs.size(); ++s4) {

                    const auto * buf = coulomb_engine.compute(obs[s1], obs[s2], obs[s3], obs[s4]);

                }
            }
        }
    }

    libint2::finalize(); // do not use libint after this

    return 0;
}