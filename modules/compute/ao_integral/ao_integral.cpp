#include <libint2.hpp>
#include "libint_tools.h" //Will include ezfio and zmq
#include "mono.h" //sendMono
#include "bi.h" //sendMono
#include <getopt.h>
#include <cstdio> 
#include <stdio.h> 
#include <string.h>
#include <zmq.h> 

void print_usage()
{
    printf("INTREGRAL_BIELECT : Compute the bielectronique integral.\n");


    printf("\nUSAGE\n");
    printf("  ao_integral monoelec -z <z_endpoint>\n");
    printf("  ao_integral bielec   -z <z_endpoint> -t <t_endpoint> [-g]\n");

    printf("\nOPTION\n");
    printf("    -h                           : Give help\n");
    printf("    -z, --zezfio  <z_endpoint>  : The endpoint of the zezfio server\n");
    printf("    -t, --task    <t_endpoint>  : The endpoint of the task scheduler server\n");
    printf("    -g  --generate_task         : Create the Task\n");
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
        { "zezfio",required_argument,0, 'z'},
        { "task", required_argument, 0, 't' },
        { "generate_task", no_argument, 0, 'g' },
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
        case 'g':
            do_task = 1;
            break;
        case 't':
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
    /*** zezfio  **/
    /*** ======= **/
    void* context = zmq_ctx_new(); // Do not use ZeroMQ before this
    void* zezfio_socket = zmq_socket(context, ZMQ_REQ);

    int rc = zmq_connect(zezfio_socket, zezfio_address);
    if (rc != 0) {
        perror("Error connecting the zezfio_socket");
        return 1;
    }

    Atom_Obs ao = zezfio2libint(zezfio_socket);

    /*** ============ **/
    /*** mono or bi?  **/
    /*** ============ **/

    if (strcmp(mode,"monoelec") == 0) {
        sendMono(zezfio_socket, ao);
    }

    if (strcmp(mode,"bielec") == 0){

        void* task_socket = zmq_socket(context, ZMQ_REQ);    
        rc = zmq_connect(task_socket, task_scheduler_address);
        if (rc != 0) {
            perror("Error connecting the task_scheduler_address");
            return 1;
        }

        if (do_task){
           sendBiTask(task_socket,ao);
        }
        else{

            Collector_Info ci = initializeCollector(context,task_socket);
            sendBiInt(task_socket,ci,ao);
            filinizeCollector(task_socket,ci);
        }

        rc = zmq_close(task_socket);
    }

    rc = zmq_close(zezfio_socket);
    zmq_ctx_destroy (context);

    return 0;
};