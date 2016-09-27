#ifndef LIBINT_TOOLS_H
#define LIBINT_TOOLS_H
#include <vector>
#include <utility>
#include <array>
#include <libint2.hpp>
struct Atom_Obs
{
    std::vector<std::pair<double,std::array<double,3>>> atoms;
    libint2::BasisSet obs;
    std::vector<double> renorm;
};

Atom_Obs zezfio2libint(void*);
#endif