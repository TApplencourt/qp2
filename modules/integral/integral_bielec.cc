#include <libint2.hpp>
#include <iostream> 
#include <stdlib.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>


long int bielec_integrals_index(int i, int j, int k, int l) {
  int p,q,r,s;
  int i1, i2;

  p = i < k ? i : k ;
  r = i < k ? k : i ;
  p += (r*r-r) >> 1;

  q = j < l ? j : l;
  s = j < l ? l : j;
  q += (s*s-s) >> 1;

  i1 = p < q ? p : q;
  i2 = p < q ? q : p;
  return i1+ ( (i2*i2-i2) >> 1);
}



using namespace std;

int main(int argc, char * argv[]) {


/* TODO

* Check if xyz exists
* Check if basis file exists

*/

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

    /*** ============================ **/
    /*** mmap dirname                 **/
    /*** ============================ **/
    char *filename = argv[3];




    libint2::BasisSet obs(basis_name, atoms);
    obs.set_pure(false); // use cartesian gaussians
    size_t nao = 0;
    for (auto s=0; s<obs.size(); ++s)
      nao += obs[s].size();
    
    auto shell2bf = obs.shell2bf(); // maps shell index to basis function index
                                    // shell2bf[0] = index of the first basis function in shell 0
                                    // shell2bf[1] = index of the first basis function in shell 1
                                    // ...

    libint2::init(); // safe to use libint now

    libint2::TwoBodyEngine < libint2::Coulomb > coulomb_engine(obs.max_nprim(), obs.max_l(), 0);

    /* Data for mmap */
    typedef struct 
    { short int      i,j,k,l;
      double   value;
    } key_value_type;

    int fd = open(filename, O_RDWR | O_CREAT, (mode_t)0600);
    key_value_type* map;

    size_t offset = 1 * sysconf(_SC_PAGE_SIZE);
    size_t bytes = nao*nao*nao*nao * sizeof(key_value_type);
    cout << bytes << endl;

    if (fd == -1) {
        printf("%s:\n", filename);
        perror("Error opening mmap file for writing");
        exit(EXIT_FAILURE);
    }

    if (lseek(fd, bytes+offset, SEEK_SET) == -1) {
        close(fd);
        printf("%s:\n", filename);
        perror("Error calling lseek() to stretch the file");
        exit(EXIT_FAILURE);
    }
    if (write(fd, "", 1) != 1) {
        close(fd);
        printf("%s:\n", filename);
        perror("Error writing last byte of the file");
        exit(EXIT_FAILURE);
    }

    map = (key_value_type*) mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    int kk;
    for (kk=0 ; kk < nao*nao*nao*nao ; kk++){
      map[kk].i = 0;
    }

    for (auto s1 = 0; s1 != obs.size(); ++s1) {
        std::cout << s1 << "/" << obs.size() << std::endl;
        for (auto s2 = 0; s2 != obs.size(); ++s2)
            for (auto s3 = 0; s3 != obs.size(); ++s3)
                for (auto s4 = 0; s4 != obs.size(); ++s4) {

                    const auto * buf = coulomb_engine.compute(obs[s1], obs[s2], obs[s3], obs[s4]);

                    auto bf1 = shell2bf[s1]+1;  // first basis function in first shell
                    auto n1 = obs[s1].size(); // number of basis functions in first shell
                    auto bf2 = shell2bf[s2]+1;  // first basis function in second shell
                    auto n2 = obs[s2].size(); // number of basis functions in second shell
                    auto bf3 = shell2bf[s3]+1;  // first basis function in third shell
                    auto n3 = obs[s3].size(); // number of basis functions in third shell
                    auto bf4 = shell2bf[s4]+1;  // first basis function in fourth shell
                    auto n4 = obs[s4].size(); // number of basis functions in fourth shell
                
                    // integrals are packed into buf in row-major (C) form
                    // this iterates over integrals in this order
                    int i,j,k,l;
                    for(auto f1=0; f1!=n1; ++f1) {
                        i = bf1+f1;
                        for(auto f2=0; f2!=n2; ++f2) {
                          j = bf2+f2;
                          for(auto f3=0; f3!=n3; ++f3) {
                            k = bf3+f3;
                            for(auto f4=0; f4!=n4; ++f4) {
                                auto f1234 = f1*n2*n3*n4+f2*n3*n4+f3*n4+f4;
                                if ( buf[f1234] != 0. ) {
                                  l = bf4+f4;
                                  kk = bielec_integrals_index(i,k,j,l)-1;
//                                 cout << "  " << kk << "  " << bf1+f1 << " " << bf2+f2 << " " << bf3+f3 << " " << bf4+f4 << " " << buf[f1234] << endl;
                                  map[kk].i = (short int) i;
                                  map[kk].j = (short int) j;
                                  map[kk].k = (short int) k;
                                  map[kk].l = (short int) l;
                                  map[kk].value = buf[f1234];
                                }
                            }
                }
         }
       }
     }
    }

    libint2::finalize(); // do not use libint after this


    int ll = 0;
    for (kk=0 ; kk < nao*nao*nao*nao ; kk++) {
      if (map[kk].i > 0) {
        map[ll] = map[kk];
        ll++;
      }
    }
    map[ll].i = 0;
    map[ll].j = 0;
    map[ll].k = 0;
    map[ll].l = 0;
    map[ll].value = 0.;

 
    if (munmap(map, bytes) == -1) {
        perror("Error un-mmapping the file");
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        close(fd);
        printf("%s:\n", filename);
        perror("Error calling lseek() to stretch the file");
        exit(EXIT_FAILURE);
    }

    write(fd, (void*) &ll, 8);

    ftruncate(fd, offset+(ll)*sizeof(key_value_type));
    close(fd);

    return 0;
}
