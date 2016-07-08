#include <zmq.h>
#include <set>
#include <libint2.hpp>
#include <string>
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

//const double precision = 1e-15;
const double precision = -1e-15;

Matrix compute_schwartz_ints(const BasisSet& bs1,
    const BasisSet& bs2 = BasisSet(),
    bool use_2norm = false // use infty norm by default
    );

/**
    For a (ij|kl) integral quarter (starting with 0) return a unique id.

    @param i,j,k,l  the indice of the basis shell function
    @return A uniq key
*/
long int bielec_integrals_index(const int i, const int j, const int k, const int l);

/**
    For a (mn|rs) shell integrals stored in a buffer
    save it to the the  map array

     @param map the map array
            n{1,4} the number of AO in (mn,rs) respectively
            bf{1,4}_first the index of the first basis function in thhis shell
*/
void send_buffer(void* push_socket, int task_id,
    const int n1, const int n2, const int n3, const int n4,
    const int bf1_first, const int bf2_first, const int bf3_first, const int bf4_first,
    const double* buf_1234);

/***
 *     ___                                                    
 *      |  ._ _  ._  |  _  ._ _   _  ._ _|_  _. _|_ o  _  ._  
 *     _|_ | | | |_) | (/_ | | | (/_ | | |_ (_|  |_ | (_) | | 
 *               |                                            
 */

long int
bielec_integrals_index(const int i, const int j, const int k, const int l)
{

    const int p = i < j ? (i + 1) : (j + 1);
    const int r = i < j ? (j + 1) : (i + 1);
    const long int pp = p + ((r * r - r) / 2);

    const int q = k < l ? (k + 1) : (l + 1);
    const int s = k < l ? (l + 1) : (k + 1);
    const long int qq = q + ((s * s - s) / 2);

    const long int i1 = pp < qq ? pp : qq;
    const long int i2 = pp < qq ? qq : pp;
    return i1 + ((i2 * i2 - i2) / 2);
}

void send_buffer(void* push_socket, int task_id,
    const int n1, const int n2, const int n3, const int n4,
    const int bf1_first, const int bf2_first, const int bf3_first, const int bf4_first,
    const double* buf_1234)
{
    int n_integrals = 0;
    const size_t n_integrals_max = n1 * n2 * n3 * n4;
    long int* buffer_i = (long int*)malloc(n_integrals_max * sizeof(long int));
    double* buffer_value = (double*)malloc(n_integrals_max * sizeof(double));
    /**
          loop over all the basis function in the buffer
          If all the shell in the quartet are different,
          we have no permutation to handle.
          Else, you have to take care of the possible duplicate.
          */
    if (n1 != n2 && n1 != n3 && n1 != n4 && n2 != n3 && n2 != n4 && n3 != n4) {

        for (auto f1 = 0, f1234 = 0; f1 != n1; ++f1) {
            const int bf1 = f1 + bf1_first;
            for (int f2 = 0; f2 != n2; ++f2) {
                const int bf2 = f2 + bf2_first;
                for (int f3 = 0; f3 != n3; ++f3) {
                    const int bf3 = f3 + bf3_first;
                    for (int f4 = 0; f4 != n4; ++f4, ++f1234) {
                        const int bf4 = f4 + bf4_first;

                        if (fabs(buf_1234[f1234]) > precision) {
                            buffer_i[n_integrals] = bielec_integrals_index(bf1, bf2, bf3, bf4);
                            buffer_value[n_integrals] = buf_1234[f1234];
                            n_integrals += 1;
                        }
                    }
                }
            }
        }
    }
    else {

        //Vector to store the uniq key of the quartet
        set<long int> uniq;

        for (int f1 = 0, f1234 = 0; f1 != n1; ++f1) {
            const int bf1 = f1 + bf1_first;
            for (int f2 = 0; f2 != n2; ++f2) {
                const int bf2 = f2 + bf2_first;
                for (int f3 = 0; f3 != n3; ++f3) {
                    const int bf3 = f3 + bf3_first;
                    for (int f4 = 0; f4 != n4; ++f4, ++f1234) {
                        if (fabs(buf_1234[f1234]) > precision) {
                            const int bf4 = f4 + bf4_first;
                            const long int key = bielec_integrals_index(bf1, bf2, bf3, bf4);
                            //Check if the quartet has been already computed
                            if (uniq.find(key) == uniq.end()) {
                                uniq.insert(key);
                                buffer_i[n_integrals] = key;
                                buffer_value[n_integrals] = buf_1234[f1234];
                                n_integrals += 1;
                            }
                        }
                    }
                }
            }
        }
    }
    int rc;
    printf("%d\n", n_integrals);
    rc = zmq_send(push_socket, &n_integrals, 4, ZMQ_SNDMORE);
    if (rc != 4) {
        perror("Error pushing n_integrals");
        exit(EXIT_FAILURE);
    }

    if (n_integrals > 0) {
        int msg_len = n_integrals * sizeof(long int);
        rc = zmq_send(push_socket, buffer_i, msg_len, ZMQ_SNDMORE);
        if (rc != msg_len) {
            perror("Error pushing buffer_i");
            exit(EXIT_FAILURE);
        }

        msg_len = n_integrals * sizeof(double);
        rc = zmq_send(push_socket, buffer_value, msg_len, ZMQ_SNDMORE);
        if (rc != msg_len) {
            perror("Error pushing buffer_value");
            exit(EXIT_FAILURE);
        }
    }

    rc = zmq_send(push_socket, &task_id, 4, 0);
    if (rc != 4) {
        perror("Error pushing task_id");
        exit(EXIT_FAILURE);
    }

    free(buffer_i);
    free(buffer_value);
}

// cp from Hartree-Fock libint (remove opemmp stuff)
Matrix compute_schwartz_ints(const BasisSet& bs1,
    const BasisSet& _bs2,
    bool use_2norm)
{
    const BasisSet& bs2 = (_bs2.empty() ? bs1 : _bs2);
    const size_t nsh1 = bs1.size();
    const size_t nsh2 = bs2.size();
    const bool bs1_equiv_bs2 = (&bs1 == &bs2);

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
    printf("    -a, --address <name> : The address of the task server\n");

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
        { "task", no_argument, 0, 't' },
        { "address", required_argument, 0, 'a' },
        { "xyz", required_argument, 0, 'x' },
        { "basis", required_argument, 0, 'b' },
        { 0, 0, 0, 0 }
    };

    string xyz_path;
    string basis_name;
    string qp_run_address;
    int do_task = 0;

    int iarg = 0;
    //switch getopt error message
    opterr = 1;
    while ((iarg = getopt_long(argc, argv, "ha:tx:b:m:", long_options, NULL)) != -1) {

        switch (iarg) {
        case 'h':
            return 0;
        case 't':
            do_task = 1;
            break;
        case 'a':
            qp_run_address = optarg;
            break;
        case 'x':
            xyz_path = optarg;
            break;
        case 'b':
            basis_name = optarg;
            break;
        default:
            print_usage();
            return 1;
        }
    }

    if (xyz_path.empty() || basis_name.empty() || qp_run_address.empty()) {
        print_usage();
        return 1;
    }

    /*** =========================== ***/
    /*** initialize molecule         ***/
    /*** =========================== ***/
    struct stat buffer;
    if (stat(xyz_path.c_str(), &buffer) != 0) {
        printf("%s:\n", xyz_path.c_str());
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

    libint2::init(); // do not use libint before this

    /*** ============================ **/
    /*** Compute schwartz             **/
    /*** ============================ **/

    // WARNING: NOT SUR IF THIS WORK
    // We do not compute the matrix of norms of shell blocks
    const auto Schwartz = compute_schwartz_ints(obs);

    /*** ======= **/
    /*** ZeroMQ  **/
    /*** ======= **/

    int rc;
    void* context = zmq_ctx_new(); // Do not use ZeroMQ before this
    void* qp_run_socket = zmq_socket(context, ZMQ_REQ);
    rc = zmq_connect(qp_run_socket, qp_run_address.c_str());
    if (rc != 0) {
        perror("Error connecting the socket");
        exit(EXIT_FAILURE);
    }

    char msg[512];
    int msg_len;

    if (do_task) {
        /*TODO
        * compute MN shell pair for better load balancing
        */
        for (auto s1 = 0l, s1234 = 0l; s1 != nshell; ++s1) {
            for (auto s2 = 0; s2 <= s1; ++s2) {
                for (auto s3 = 0; s3 <= s1; ++s3) {
                    const auto s4_max = (s1 == s3) ? s2 : s3;
                    for (auto s4 = 0; s4 <= s4_max; ++s4) {
                        if (Schwartz(s1, s2) * Schwartz(s3, s4) < precision)
                            continue;
                        sprintf(msg, "add_task ao_integrals %6d %6d %6d %6d", s1, s2, s3, s4);
                        rc = zmq_send(qp_run_socket, msg, 50, 0);
                        if (rc != 50) {
                            perror("Error sending the task");
                            exit(EXIT_FAILURE);
                        }
                        rc = zmq_recv(qp_run_socket, msg, 510, 0);
                        if (rc != 2) {
                            perror(msg);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }
    }
    else {

        libint2::TwoBodyEngine<libint2::Coulomb> coulomb_engine(obs.max_nprim(), obs.max_l(), 0);

        coulomb_engine.set_precision(std::min(precision, std::numeric_limits<double>::epsilon())); // shellset-dependent precision control will likely break positive definiteness
        // stick with this simple recipe

        auto shell2bf = obs.shell2bf(); // maps shell index to basis function index
        // shell2bf[0] = index of the first basis function in shell 0
        // shell2bf[1] = index of the first basis function in shell 1
        // ...

        rc = zmq_send(qp_run_socket, "connect tcp", 11, 0);
        if (rc != 11) {
            perror("Error connecting to the task server");
            exit(EXIT_FAILURE);
        }

        rc = zmq_recv(qp_run_socket, msg, 510, 0);
        msg[rc] = '\0';
        char reply[32], state[32], push_address[128];
        int worker_id;
        sscanf(msg, "%s %s %d %s", reply, state, &worker_id, push_address);
        if (strcmp(reply, "connect_reply")) {
            perror("Bad reply");
            exit(EXIT_FAILURE);
        }
        if (strcmp(state, "ao_integrals")) {
            perror("Bad state");
            exit(EXIT_FAILURE);
        }

        void* push_socket = zmq_socket(context, ZMQ_PUSH);
        rc = zmq_connect(push_socket, push_address);
        if (rc != 0) {
            perror("Error connecting the push socket");
            exit(EXIT_FAILURE);
        }

        int task_id;
        while (1) {
            sprintf(msg, "get_task ao_integrals %8d", worker_id);
            rc = zmq_send(qp_run_socket, msg, 30, 0);
            if (rc != 30) {
                perror("Error connecting the push socket");
                exit(EXIT_FAILURE);
            }
            rc = zmq_recv(qp_run_socket, msg, 510, 0);
            msg[rc] = '\0';
            int s1, s2, s3, s4;
            sscanf(msg, "%s %d %d %d %d %d", reply, &task_id, &s1, &s2, &s3, &s4);
            if (!strcmp(reply, "terminate"))
                break;

            const auto bf1_first = shell2bf[s1]; // first basis function in this shell
            const auto n1 = obs[s1].size(); // number of basis functions in this shell

            const auto bf2_first = shell2bf[s2];
            const auto n2 = obs[s2].size();

            const auto bf3_first = shell2bf[s3];
            const auto n3 = obs[s3].size();

            const auto bf4_first = shell2bf[s4];
            const auto n4 = obs[s4].size();

            const auto* buf_1234 = coulomb_engine.compute(obs[s1], obs[s2], obs[s3], obs[s4]);

            sprintf(msg, "task_done ao_integrals %d %d", worker_id, task_id);
            msg_len = strlen(msg);
            rc = zmq_send(qp_run_socket, msg, msg_len, 0);
            if (rc != msg_len) {
                perror("Error sending task_done");
                exit(EXIT_FAILURE);
            }
            rc = zmq_recv(qp_run_socket, msg, 510, 0);
            if (rc != 2) {
                perror(msg);
                exit(EXIT_FAILURE);
            }

            send_buffer(push_socket, task_id,
                n1, n2, n3, n4,
                bf1_first, bf2_first, bf3_first, bf4_first,
                buf_1234);
        }

        send_buffer(push_socket, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            NULL);

        sprintf(msg, "disconnect ao_integrals %d", worker_id);
        msg_len = strlen(msg);
        rc = zmq_send(qp_run_socket, msg, msg_len, 0);
        if (rc != msg_len) {
            perror("Error sending disconnect");
            exit(EXIT_FAILURE);
        }
        rc = zmq_recv(qp_run_socket, msg, 510, 0);
        msg[rc] = '\0';
        sscanf(msg, "%s %s", reply, state);
        if (strcmp(reply, "disconnect_reply") || strcmp(state, "ao_integrals")) {
            perror(msg);
            exit(EXIT_FAILURE);
        }

        int value = 0;
        rc = zmq_disconnect(push_socket, push_address);
        rc = zmq_setsockopt(push_socket, ZMQ_LINGER, &value, 4);
        rc = zmq_close(push_socket);
    }

    int value = 0;
    rc = zmq_disconnect(qp_run_socket, qp_run_address.c_str());
    rc = zmq_setsockopt(qp_run_socket, ZMQ_LINGER, &value, 4);
    rc = zmq_close(qp_run_socket);

    libint2::finalize(); // do not use libint after this
    return 0;
}
