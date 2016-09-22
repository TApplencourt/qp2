#include <libint2.hpp>

struct Atom_Obs
{
    std::vector<libint2::Atom> atoms;
    libint2::BasisSet obs;
};

Atom_Obs zezfio2libint(void*);