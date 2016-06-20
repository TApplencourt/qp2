#include <libint2.hpp>

int main(int argc, char* argv[]) {


    /*** =========================== ***/
    /*** initialize molecule         ***/
    /*** =========================== ***/
    std::string ezfio_filename = "";
    std::string xyz_path = ezfio_filename + std::string("/libint/xyz");
    // read geometry from a filename
    std::ifstream input_file(xyz_path);
    std::vector<libint2::Atom> atoms = libint2::read_dotxyz(input_file);

    /*** =========================== ***/
    /*** create basis set            ***/
    /*** =========================== ***/

    std::string basis_path = ezfio_filename + std::string("/libint");
    setenv("LIBINT_DATA_PATH", basis_path.c_str(), 1);

    libint2::BasisSet shells("basis", atoms);
    shells.set_pure(false); // use cartesian gaussians


  libint2::init();  // safe to use libint now

  // all other code snippets go here

  libint2::finalize();  // do not use libint after this

  // can repeat the libint2::initialize() ... finalize() cycle as many times as
  // necessary

  return 0;
}