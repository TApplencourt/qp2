#include <libint2.hpp>

#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <getopt.h>
#include <sys/stat.h>

using namespace std;

typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
    Matrix; // import dense, dynamically sized Matrix type from Eigen;
// this is a matrix with row-major storage (http://en.wikipedia.org/wiki/Row-major_order)
// to meet the layout of the integrals returned by the Libint integral library

/***
 *                            
 *     |_|  _   _.  _|  _  ._ 
 *     | | (/_ (_| (_| (/_ |  
 *                            
 */

// BOROWED FROM LIBINT Hartree-Fock
using libint2::Atom;
using libint2::BasisSet;

Matrix compute_schwartz_ints(const BasisSet& bs1,
    const BasisSet& bs2 = BasisSet(),
    bool use_2norm = false // use infty norm by default
    );

// The is the data  structure who will be stored in the memory map
struct IntQuartet {
    short int i, j, k, l;
    double value;
};

/**
    Create the memory map file

    @param filename  the location of the memory map in disk
           bytes     An uperbound of the  size of the memory map
    @return A memore map
*/
IntQuartet* init_mmap(const char* filename, const size_t bytes);

/**
    Close the memory map file

    @param filename  the memory map.
           bytes     An uperbound of the  size of the memory map
*/
void finalize_mmap(IntQuartet* map, const size_t bytes);

/**
    For a (ij|kl) integral quarter (starting with 0) return a unique id.

    @param i,j,k,l  the indice of the basis shell function
    @return A uniq key
*/
long int bielec_integrals_index(const short int i, const short int j, const short int k, const short int l);

/**
    For a (ij|kl) integral quarter key, and this value
    save it in the map at the indice kk

    @param map      the  map array
           kk       the indice in the memory map
           i,j,k,l  the indice of the basis shell function
           value    the value of the integrals
*/
void save_quartet(IntQuartet* map,
    const long int kk,
    const short int i, const short int j, const short int k, const short int l,
    const double value);

/**
    For a (mn|rs) shell integrals stored in a buffer
    save it to the the  map array

     @param map the map array
            n{1,4} the number of AO in (mn,rs) respectively
            bf{1,4}_first the index of the first basis function in thhis shell
*/
void save_buffer(IntQuartet* map,
    const short int n1, const short int n2, const short int n3, const short int n4,
    const short int bf1_first, const short int bf2_first, const short int bf3_first, const short int bf4_first,
    const double* buf_1234);

/***
 *     ___                                                    
 *      |  ._ _  ._  |  _  ._ _   _  ._ _|_  _. _|_ o  _  ._  
 *     _|_ | | | |_) | (/_ | | | (/_ | | |_ (_|  |_ | (_) | | 
 *               |                                            
 */

IntQuartet* init_mmap(const string filename, const size_t bytes)
{
    /**
  1- Create the file descriptor
  2- Strech the file
  3- Create the mmap
  4 - Close the file descriptor

  Standard Error:
  EACCES
  A file descriptor refers to a non-regular file.
  Or MAP_PRIVATE was requested, but fd is not open for reading.
  Or MAP_SHARED was requested and PROT_WRITE is set, but fd is not open in read/write (O_RDWR) mode.
  Or PROT_WRITE is set, but the file is append-only.
  */

    // Create file descriptor (chmod 600 file – owner can read and write)
    const int fd = open(filename.c_str(), O_RDWR | O_CREAT, (mode_t)0600);
    if (fd == -1) {
        printf("%s:\n", filename);
        perror("Error opening mmap file for writing only");
        exit(EXIT_FAILURE);
    }

    // Stretch the file
    // The sparse file takes up zero space, since the file system
    //  was smart enough to recognize that it only contains zeroes
    if (lseek(fd, bytes, SEEK_SET) == -1) {
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

    // Create the actual memory map
    IntQuartet* map = (IntQuartet*)mmap(NULL, bytes, PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap failed to be created");
        exit(EXIT_FAILURE);
    }
    // Close the file descriptor. We dont need it anymore
    close(fd);

    return map;
}

void finalize_mmap(IntQuartet* map, const size_t bytes)
{

    if (munmap(map, bytes) == -1) {
        perror("Error un-mmapping the file");
        exit(EXIT_FAILURE);
    }
}

long int
bielec_integrals_index(const short int i, const short int j, const short int k, const short int l)
{

    const short int p = i < j ? (i + 1) : (j + 1);
    const short int r = i < j ? (j + 1) : (i + 1);
    const short int pp = p + ((r * r - r) >> 1);

    const short int q = k < l ? (k + 1) : (l + 1);
    const short int s = k < l ? (l + 1) : (k + 1);
    const short int qq = q + ((s * s - s) >> 1);

    const short int i1 = pp < qq ? pp : qq;
    const short int i2 = pp < qq ? qq : pp;
    return i1 + ((i2 * i2 - i2) >> 1);
}

void save_quartet(IntQuartet* map,
    const long int kk,
    const short int i,
    const short int j,
    const short int k,
    const short int l,
    const double value)
{

    //Create the struct and save it ine the array
    map[kk].i = i;
    map[kk].j = j;
    map[kk].k = k;
    map[kk].l = l;
    map[kk].value = value;

    // Print for debuging
    cout << " " << kk
         << "  " << i << " " << j << " " << k << " " << l
         << " " << value << endl;
}

void save_buffer(IntQuartet* map,
    const short int n1, const short int n2, const short int n3, const short int n4,
    const short int bf1_first, const short int bf2_first, const short int bf3_first, const short int bf4_first,
    const double* buf_1234)
{
    /**
          loop over all the basis function in the buffer
          If all the shell in the quartet are different,
          we have no permutation to handle.
          Else, you have to take care of the possible duplicate.
          */
    if (n1 != n2 && n1 != n3 && n1 != n4 && n2 != n3 && n2 != n4 && n3 != n4) {

        for (auto f1 = 0, f1234 = 0; f1 != n1; ++f1) {
            auto bf1 = f1 + bf1_first;
            for (auto f2 = 0; f2 != n2; ++f2) {
                auto bf2 = f2 + bf2_first;
                for (auto f3 = 0; f3 != n3; ++f3) {
                    auto bf3 = f3 + bf3_first;
                    for (auto f4 = 0; f4 != n4; ++f4, ++f1234) {
                        auto bf4 = f4 + bf4_first;

                        auto key = bielec_integrals_index(bf1, bf2, bf3, bf4);

                        if (buf_1234[f1234] != 0.)
                            // WARNING key cause a lot a random acess
                            save_quartet(map, key, bf1, bf2, bf3, bf4, buf_1234[f1234]);
                    }
                }
            }
        }
    }
    else {

        //Vector to store the uniq key of the quartet
        vector<long int> uniq(n1 * n2 * n3 * n4);

        for (auto f1 = 0, f1234 = 0; f1 != n1; ++f1) {
            auto bf1 = f1 + bf1_first;
            for (auto f2 = 0; f2 != n2; ++f2) {
                auto bf2 = f2 + bf2_first;
                for (auto f3 = 0; f3 != n3; ++f3) {
                    auto bf3 = f3 + bf3_first;
                    for (auto f4 = 0; f4 != n4; ++f4, ++f1234) {
                        auto bf4 = f4 + bf4_first;
                        auto key = bielec_integrals_index(bf1, bf2, bf3, bf4);

                        //Check if the quartet have been already computed
                        if (find(uniq.begin(), uniq.end(), key) == uniq.end() and buf_1234[f1234] != 0.) {
                            // WARNING key cause a lot a random acess
                            save_quartet(map, key, bf1, bf2, bf3, bf4, buf_1234[f1234]);
                            uniq.push_back(key);
                        }
                    }
                }
            }
        }
    }
}

// cp from Hartree-Fock libint (remove opemmp stuff)
Matrix compute_schwartz_ints(const BasisSet& bs1,
    const BasisSet& _bs2,
    bool use_2norm)
{
    const BasisSet& bs2 = (_bs2.empty() ? bs1 : _bs2);
    const auto nsh1 = bs1.size();
    const auto nsh2 = bs2.size();
    const auto bs1_equiv_bs2 = (&bs1 == &bs2);

    Matrix K = Matrix::Zero(nsh1, nsh2);

    // construct the 2-electron repulsion integrals engine
    libint2::TwoBodyEngine<libint2::Coulomb> engines(bs1.max_nprim(), bs2.max_l(), 0);
    engines.set_precision(0.); // !!! very important: cannot screen primitives in Schwartz computation !!!

    // loop over permutationally-unique set of shells
    for (auto s1 = 0l, s12 = 0l; s1 != nsh1; ++s1) {

        auto n1 = bs1[s1].size(); // number of basis functions in this shell

        auto s2_max = bs1_equiv_bs2 ? s1 : nsh2 - 1;
        for (auto s2 = 0; s2 <= s2_max; ++s2, ++s12) {

            auto n2 = bs2[s2].size();
            auto n12 = n1 * n2;

            const auto* buf = engines.compute(bs1[s1], bs2[s2], bs1[s1], bs2[s2]);

            // the diagonal elements are the Schwartz ints ... use Map.diagonal()
            Eigen::Map<const Matrix> buf_mat(buf, n12, n12);
            auto norm2 = use_2norm ? buf_mat.diagonal().norm() : buf_mat.diagonal().lpNorm<Eigen::Infinity>();
            K(s1, s2) = std::sqrt(norm2);
            if (bs1_equiv_bs2)
                K(s2, s1) = K(s1, s2);
        }
    }
    return K;
}
/***
 *                    
 *     |\/|  _. o ._  
 *     |  | (_| | | | 
 *                    
 */

void print_usage()
{
    printf("INTREGRAL_BIELECT : Compute the bielectronique integral.\n");
    printf("   For each possible permutation of i,j,k,l giving the same integral,\n");
    printf("   only one arbitrary version is stored in the memory map\n");

    printf("\nUSAGE\n");
    printf("  integral_bielect [OPTION]\n");

    printf("\nOPTION\n");
    printf("    -x, --xyz   <path>   : The location of the xyz geometry file\n");
    printf("    -b, --basis <name>   : The name of the basis set in 94 format\n");
    printf("    -m, --mmap  <name>   : The location where the memory map will be created\n");

    printf("\nNOTA BENE\n");
    printf("  The basis set need to be present in $LIBINT_DATA_PATH\n");
    printf("  The standard path is $qp_root/usr/share/libint/2.1.0-beta/basis/\n");
    printf("  export LIBINT_DATA_PATH=$qp_root/usr/share/libint/2.1.0-beta/basis/\n");
}

int main(int argc, char* argv[])
{
    /** TODO
    * Check if basis file exists
    */

    /*** =========================== ***/
    /*** Sanitze input               ***/
    /*** =========================== ***/
    static struct option long_options[] = {
        /* These options set a flag. */
        { "help", no_argument, 0, 'h' },
        { "xyz", required_argument, 0, 'x' },
        { "basis", required_argument, 0, 'b' },
        { "mmap", required_argument, 0, 'm' },
        { 0, 0, 0, 0 }
    };
    const double precision = 1e-8;

    string xyz_path;
    string basis_name;
    string mmap_path;

    int iarg = 0;
    //switch getopt error message
    opterr = 1;
    while ((iarg = getopt_long(argc, argv, "hx:b:m:", long_options, &index)) != -1) {

        switch (iarg) {
        case 'h':
            return 0;
        case 'x':
            xyz_path = optarg;
            break;
        case 'b':
            basis_name = optarg;
            break;
        case 'm':
            mmap_path = optarg;
            break;
        default:
            print_usage();
            return 1;
        }
    }

    if (xyz_path.empty() || basis_name.empty() || mmap_path.empty()) {
        print_usage();
        return 1;
    }

    /*** =========================== ***/
    /*** initialize molecule         ***/
    /*** =========================== ***/
    struct stat buffer;
    if (stat(xyz_path.c_str(), &buffer) != 0) {
        printf("%s:\n", xyz_path);
        perror("The xyz file do not exists");
        return 1;
    }

    ifstream input_file(xyz_path);
    vector<Atom> atoms = libint2::read_dotxyz(input_file);

    /*** =========================== ***/
    /*** create basis set            ***/
    /*** =========================== ***/
    // export LIBINT_DATA_PATH="$qp_root"/usr/share/libint/2.1.0-beta/basis/

    BasisSet obs(basis_name, atoms);
    obs.set_pure(false); // use cartesian gaussians

    const auto nshell = obs.size();
    const auto nao = obs.nbf();
    const auto unao = 0.25 * nao * (nao + 1) * (0.5 * nao * (nao + 1) + 1);
    const size_t bytes = unao * sizeof(IntQuartet);

    /*** ============================ **/
    /*** mmap dirname                 **/
    /*** ============================ **/
    IntQuartet* map = init_mmap(mmap_path, bytes); // do not use map before this
    libint2::init(); // do not use libint before this

    /*** ============================ **/
    /*** Compute schwartz             **/
    /*** ============================ **/

    // WARNING: NOT SUR IF THIS WORK
    // We do not compute the matrix of norms of shell blocks
    const auto Schwartz = compute_schwartz_ints(obs);
    const auto do_schwartz_screen = Schwartz.cols() != 0 && Schwartz.rows() != 0;

    /*** ============================ **/
    /*** Compute shell quartet        **/
    /*** ============================ **/
    /*TODO
    * compute MN shell pair for beter load ballensing
    */
    libint2::TwoBodyEngine<libint2::Coulomb> coulomb_engine(obs.max_nprim(),
        obs.max_l(), 0);

    coulomb_engine.set_precision(std::min(precision, std::numeric_limits<double>::epsilon())); // shellset-dependent precision control will likely break positive definiteness
    // stick with this simple recipe

    auto shell2bf = obs.shell2bf(); // maps shell index to basis function index
    // shell2bf[0] = index of the first basis function in shell 0
    // shell2bf[1] = index of the first basis function in shell 1
    // ...

    for (auto s1 = 0l, s1234 = 0l; s1 != nshell; ++s1) {
        auto bf1_first = shell2bf[s1]; // first basis function in this shell
        auto n1 = obs[s1].size(); // number of basis functions in this shell

        for (auto s2 = 0; s2 <= s1; ++s2) {
            auto bf2_first = shell2bf[s2];
            auto n2 = obs[s2].size();

            for (auto s3 = 0; s3 <= s1; ++s3) {
                auto bf3_first = shell2bf[s3];
                auto n3 = obs[s3].size();

                const auto s4_max = (s1 == s3) ? s2 : s3;

                for (auto s4 = 0; s4 <= s4_max; ++s4) {

                    if (Schwartz(s1, s2) * Schwartz(s3, s4) < precision)
                        continue;

                    auto bf4_first = shell2bf[s4];
                    auto n4 = obs[s4].size();

                    const auto* buf_1234 = coulomb_engine.compute(obs[s1], obs[s2], obs[s3], obs[s4]);

                    if (buf_1234 != nullptr)
                        save_buffer(map,
                            n1, n2, n3, n4,
                            bf1_first, bf2_first, bf3_first, bf4_first,
                            buf_1234);
                }
            }
        }
    }

    libint2::finalize(); // do not use libint after this
    finalize_mmap(map, bytes); // do not use map after this
    return 0;
}
