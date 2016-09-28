#include <libint2.hpp>
#include "libint_tools.h"
#include "mono.h" //Atom_Obs 
#include <vector>
#include <zmq.h>

void sendMono(void* zezfio_socket, Atom_Obs AO){


    libint2::init();
    libint2::BasisSet obs = AO.obs;


    libint2::OneBodyEngine overlap_engine(libint2::OneBodyEngine::overlap, // will compute overlap ints
                                          obs.max_nprim(), // max # of primitives in shells this engine will accept
                                          obs.max_l()      // max angular momentum of shells this engine will accept
                                         );

    libint2::OneBodyEngine kinetic_engine(libint2::OneBodyEngine::kinetic,obs.max_nprim(), obs.max_l());
    libint2::OneBodyEngine nuclear_engine(libint2::OneBodyEngine::nuclear,obs.max_nprim(), obs.max_l());
    nuclear_engine.set_params(AO.atoms); // atoms specifies the charge and position of each nucleus


    auto shell2bf = obs.shell2bf(); // maps shell index to basis function index
                                    // shell2bf[0] = index of the first basis function in shell 0
                                    // shell2bf[1] = index of the first basis function in shell 1

    const int nao = (int) obs.nbf();
    const size_t len = (size_t) (nao*nao)*sizeof(double);

    double ** overlap = (double**) malloc(nao*sizeof(double*));
    double ** nuclear = (double**) malloc(nao*sizeof(double*));
    double ** kinetic = (double**) malloc(nao*sizeof(double*));

    overlap[0] = (double*) malloc((nao*nao)*sizeof(double));
    nuclear[0] = (double*) malloc((nao*nao)*sizeof(double));
    kinetic[0] = (double*) malloc((nao*nao)*sizeof(double));
    for (auto i=1 ; i<nao ; i++)
    {
      overlap[i] = overlap[i-1] + nao;
      kinetic[i] = kinetic[i-1] + nao;
      nuclear[i] = nuclear[i-1] + nao;
    }

    std::vector<double> &renorm = AO.renorm;

    for(auto s1=0; s1!=obs.size(); ++s1) {
      for(auto s2=0; s2!=obs.size(); ++s2) {
    
        const double* ints_shellset_overlap = overlap_engine.compute(obs[s1], obs[s2]);
        const double* ints_shellset_nuclear = nuclear_engine.compute(obs[s1], obs[s2]);
        const double* ints_shellset_kinetic = kinetic_engine.compute(obs[s1], obs[s2]);
    
        const auto bf1 = shell2bf[s1];  // first basis function in first shell
        const auto n1 = obs[s1].size(); // number of basis functions in first shell
        const auto bf2 = shell2bf[s2];  // first basis function in second shell
        const auto n2 = obs[s2].size(); // number of basis functions in second shell


        for(auto f1=0; f1!=n1; ++f1)
          for(auto f2=0; f2!=n2; ++f2)
          { 
            const int idx = f1*n2+f2;
            const double norm = renorm[bf1+f1] * renorm[bf2+f2];
            overlap[bf1+f1][bf2+f2] = ints_shellset_overlap[idx] * norm;
            nuclear[bf1+f1][bf2+f2] = ints_shellset_nuclear[idx] * norm;
            kinetic[bf1+f1][bf2+f2] = ints_shellset_kinetic[idx] * norm;
printf("%d %d %lf\n", bf1+f1,bf2+f2,norm);
          }
      }
    }

    int zerrno, rc;
    const char* order_basis = "set.ao_basis.ao_num";
    rc = zmq_send(zezfio_socket,order_basis,strlen(order_basis)*sizeof(char),ZMQ_SNDMORE);
    
    rc = zmq_send(zezfio_socket,&nao,sizeof(int),0);

    rc = zmq_recv(zezfio_socket, &zerrno, sizeof(int), 0);
    if (zerrno < 0 ){
        perror("Cannot set.ao_basis.ao_num");
        exit(EXIT_FAILURE);
    }

    order_basis = "set.ao_basis.integral_overlap";
    rc = zmq_send(zezfio_socket,order_basis,strlen(order_basis)*sizeof(char),ZMQ_SNDMORE);
    rc = zmq_send(zezfio_socket,overlap[0],len,0);

    rc = zmq_recv(zezfio_socket, &zerrno, sizeof(int), 0);
    if (zerrno < 0 ){
        perror("Cannot set.ao_basis.integral_overlap");
        exit(EXIT_FAILURE);
    }


    order_basis = "set.ao_basis.integral_nuclear";
    rc = zmq_send(zezfio_socket,order_basis,strlen(order_basis)*sizeof(char),ZMQ_SNDMORE);
    rc = zmq_send(zezfio_socket,nuclear[0],len,0);

    rc = zmq_recv(zezfio_socket, &zerrno, sizeof(int), 0);
    if (zerrno < 0 ){
        perror("Cannot set.ao_basis.integral_nuclear");
        exit(EXIT_FAILURE);
    }

    order_basis = "set.ao_basis.integral_kinetic";
    rc = zmq_send(zezfio_socket,order_basis,strlen(order_basis)*sizeof(char),ZMQ_SNDMORE);
    rc = zmq_send(zezfio_socket,kinetic[0],len,0);

    rc = zmq_recv(zezfio_socket, &zerrno, sizeof(int), 0);
    if (zerrno < 0 ){
        perror("Cannot set.ao_basis.integral_kinetic");
        exit(EXIT_FAILURE);
    }

   libint2::finalize();

}
