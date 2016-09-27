struct Atom_Obs
{
    std::vector<std::pair<double,std::array<double,3>>> atoms;
    libint2::BasisSet obs;
};

Atom_Obs zezfio2libint(void*);