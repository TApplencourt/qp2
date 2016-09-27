#include <libint2.hpp>
#include "libint_tools.h"
#include <stdio.h> // fputc
#include <zmq.h> 
#include <vector>

Atom_Obs zezfio2libint(void* zezfio_socket){

    int rc, zerrno, msg_len;

    /*** =========================== ***/
    /*** Create Atmoms               ***/
    /*** =========================== ***/
    const char *order_nuclei = "get.nuclei.xyz";
    rc = zmq_send(zezfio_socket,order_nuclei,strlen(order_nuclei)*sizeof(char),0);

    rc = zmq_recv(zezfio_socket, &zerrno, sizeof(int), 0);
    if (zerrno < 0) {
        perror("get.nuclei.xyz do not exists:");
        std::exit(EXIT_FAILURE);       
    }

    rc = zmq_recv(zezfio_socket, &msg_len, sizeof(int), 0);
    char * xyz_data = (char *)malloc(msg_len * sizeof(char));
    rc = zmq_recv(zezfio_socket, xyz_data, msg_len, 0);

    std::istringstream input_file((std::string(xyz_data)));
    std::vector<libint2::Atom> atoms = libint2::read_dotxyz(input_file);

    /*** =========================== ***/
    /*** Save basis set              ***/
    /*** =========================== ***/
    const char* order_basis = "get.ao_basis.g94";
    rc = zmq_send(zezfio_socket,order_basis,strlen(order_basis)*sizeof(char),0);

    rc = zmq_recv(zezfio_socket, &zerrno, sizeof(int), 0);
    if (zerrno < 0) {
        perror("get.ao_basis.g94 do not exists:");
        std::exit(EXIT_FAILURE);  
    }

    rc = zmq_recv(zezfio_socket, &msg_len, sizeof(int), 0);
    
    char * basis_data = (char *)malloc(msg_len * sizeof(char));
    rc = zmq_recv(zezfio_socket, basis_data, msg_len, 0);

    char shm_pid_path[512];
    sprintf(shm_pid_path, "/dev/shm/%ld", (long) getpid());
    rc = mkdir(shm_pid_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //Maybe we need to check the Error code. If already exist ok, other exit

    char libint_data_path[512];
    sprintf(libint_data_path, "%s/LIBINT_DATA_PATH", shm_pid_path);
    rc = mkdir(libint_data_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //Maybe we need to check the Error code. If already exist ok, other exit

    const char* basis_name = "zezfio";
    char basis_path[512];
    sprintf(basis_path, "%s/%s.g94",libint_data_path,basis_name);

    FILE *fp;
    fp = fopen(basis_path,"w");
    fputs(basis_data,fp);
    fclose(fp);

    /*** =========================== ***/
    /*** create basis set            ***/
    /*** =========================== ***/
    char env[512];
    sprintf(env, "LIBINT_DATA_PATH=%s", libint_data_path);
    putenv(env);

    libint2::Shell::do_enforce_unit_normalization(false);
    libint2::BasisSet obs(basis_name, atoms);
    obs.set_pure(false); // use cartesian gaussians

    //Borowed from https://github.com/evaleev/libint/blob/a1741716a89c96cec6ad064f8bf5fb22f1df606a/tests/hartree-fock/hartree-fock.cc#L120
    std::vector<std::pair<double,std::array<double,3>>> q;
    for(const auto& atom : atoms) { q.push_back( {static_cast<double>(atom.atomic_number), {{atom.x, atom.y, atom.z}}} );}


    libint2::init();

    libint2::OneBodyEngine overlap_engine(libint2::OneBodyEngine::overlap, // will compute overlap ints
                                          obs.max_nprim(), // max # of primitives in shells this engine will accept
                                          obs.max_l()      // max angular momentum of shells this engine will accept
                                         );

    auto shell2bf = obs.shell2bf(); // maps shell index to basis function index
                                    // shell2bf[0] = index of the first basis function in shell 0
                                    // shell2bf[1] = index of the first basis function in shell 1

    /* Compute renormalization factors for AOs due to <xy|xy> != <xx|xx> */
    std::vector<double> renorm(obs.nbf());
    for (int s1 = 0; s1 < obs.size(); s1++) {
        const auto* ints_shellset = overlap_engine.compute(obs[s1], obs[s1]);
        const int n1 = obs[s1].size();
        const int bf1_first = shell2bf[s1];
        for (int bf1 = 0; bf1 < n1; bf1++) {
            const int ao_idx = bf1_first + bf1;
            const int diag_idx = bf1 * n1 + bf1;
            renorm[ao_idx] = sqrt(ints_shellset[0] / ints_shellset[diag_idx]);
        }
    }

    libint2::finalize();

    Atom_Obs ao;
    ao.atoms = q;
    ao.obs = obs;
    ao.renorm = renorm;

    //Cleaning.
    rc = remove(basis_path);
    rc = rmdir(libint_data_path);
    rc = rmdir(shm_pid_path);
    return ao;
}


