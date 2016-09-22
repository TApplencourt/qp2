#include "libint_tools.h" //Will include ezfio and zmq
#include <getopt.h>
#include <cstdio> 
#include <stdio.h> 
#include <string.h>
#include <zmq.h> 

void print_usage()
{
    printf("INTREGRAL_BIELECT : Compute the bielectronique integral.\n");


    printf("\nUSAGE\n");
    printf("  ao_integral monoelec -z <z_address>\n");
    printf("  ao_integral bielec   -z <z_address> -s <s_address> [-t]\n");

    printf("\nOPTION\n");
    printf("    -h                                : Give help\n");
    printf("    -z, --zezfio_server  <z_address>  : The address of the zezfio server\n");
    printf("    -t, --task_server    <ts_address> : The address of the task scheduler server\n");
    printf("    -g  --generate_task               : Create the Task\n");
}

void print_help()
{
    printf("\nMORE_INFO\n");

    printf("   For each possible permutation of i,j,k,l giving the same integral,\n");
    printf("   only one arbitrary version is computed (this is a task)\n");
    printf("\n");
    printf(" * monoelec : Will compute the all the monoelec integral\n");
    printf("                 and store to the zezfio server\n");
    printf(" * bielec   : Will compute the all the monoelec integral\n");
    printf("                 and send then to the task scheduler server\n");    
}

int main(int argc, char* argv[])
{


    char* task_scheduler_address;
    char* zezfio_address;
    char* mode;
    int   do_task = 0;

    /*** =========================== ***/
    /*** Sanitze input               ***/
    /*** =========================== ***/
    static struct option long_options[] = {
        /* These options set a flag. */
        { "help", no_argument, 0, 'h' },
        { "zezfio_server",required_argument,0, 'z'},
        { "task_server", required_argument, 0, 't' },
        { "task", no_argument, 0, 't' },
        { 0, 0, 0, 0 }
    };


    if (argc==1){
        print_usage();
        return 1;
    }
    else{
        mode = argv[1];
    }


    int iarg = 0;
    while ((iarg = getopt_long(argc, argv, "hz:a:t", long_options, NULL)) != -1) {

        switch (iarg) {
        case 'h':
            print_usage();
            print_help();
            return 0;
        case 't':
            do_task = 1;
            break;
        case 'a':
            task_scheduler_address = optarg;
            break;
        case 'z':
            zezfio_address = optarg;
            break;
        default:
            print_usage();
            return 1;
        }
    }

    if ( strcmp(mode,"monoelec") !=0 && strcmp(mode,"bielec") !=0 ) {
        print_usage();
        return 1;
    }



    /*** ======= **/
    /*** ZeroMQ  **/
    /*** ======= **/
    void* context = zmq_ctx_new(); // Do not use ZeroMQ before this
    void* zezfio_socket = zmq_socket(context, ZMQ_REQ);

    int rc = zmq_connect(zezfio_socket, zezfio_address);
    if (rc != 0) {
        perror("Error connecting the zezfio_socket");
        return 1;
    }

    libint2::init();


    //TODO:
    // Send these 3 array by zezfio
    // Create a function to do this in libint_tools.cc

    Atom_Obs ao = zezfio2libint(zezfio_socket);
    libint2::BasisSet obs = ao.obs;


    libint2::OneBodyEngine overlap_engine(libint2::OneBodyEngine::overlap, // will compute overlap ints
                                          obs.max_nprim(), // max # of primitives in shells this engine will accept
                                          obs.max_l()      // max angular momentum of shells this engine will accept
                                         );

    libint2::OneBodyEngine kinetic_engine(libint2::OneBodyEngine::kinetic,obs.max_nprim(), obs.max_l());


    //Borowed from https://github.com/evaleev/libint/blob/a1741716a89c96cec6ad064f8bf5fb22f1df606a/tests/hartree-fock/hartree-fock.cc#L120
    libint2::OneBodyEngine nuclear_engine(libint2::OneBodyEngine::nuclear,obs.max_nprim(), obs.max_l());
    std::vector<std::pair<double,std::array<double,3>>> q;
    for(const auto& atom : ao.atoms) { q.push_back( {static_cast<double>(atom.atomic_number), {{atom.x, atom.y, atom.z}}} );}
    nuclear_engine.set_params(q); // atoms specifies the charge and position of each nucleus


    auto shell2bf = obs.shell2bf(); // maps shell index to basis function index
                                    // shell2bf[0] = index of the first basis function in shell 0
                                    // shell2bf[1] = index of the first basis function in shell 1


    for(auto s1=0; s1!=obs.size(); ++s1) {
      for(auto s2=0; s2!=obs.size(); ++s2) {
    
        std::cout << "compute shell set {" << s1 << "," << s2 << "} ... ";
        const auto* ints_shellset_overlap = overlap_engine.compute(obs[s1], obs[s2]);
        const auto* ints_shellset_nuclear = nuclear_engine.compute(obs[s1], obs[s2]);
        const auto* ints_shellset_kinetic = kinetic_engine.compute(obs[s1], obs[s2]);
        std::cout << "done" << std::endl;
    
        auto bf1 = shell2bf[s1];  // first basis function in first shell
        auto n1 = obs[s1].size(); // number of basis functions in first shell
        auto bf2 = shell2bf[s2];  // first basis function in second shell
        auto n2 = obs[s2].size(); // number of basis functions in second shell
    
        // integrals are packed into ints_shellset in row-major (C) form
        // this iterates over integrals in this order
        for(auto f1=0; f1!=n1; ++f1)
          for(auto f2=0; f2!=n2; ++f2)
            std::cout << "  " << bf1+f1 << " " << bf2+f2 << " " << ints_shellset_overlap[f1*n2+f2] << std::endl;
      }
    }

    libint2::finalize();

    return 0;
};