#include "bi.h" //Atom_Obs 
#include "libint_tools.h"
#include <zmq.h> 
#include <vector>
#include <unordered_set>

/**
    For a (ij|kl) integral quarter (starting with 0) return a unique id.

    @param i,j,k,l  the indice of the basis shell function
    @return A uniq key
*/
long int
bielec_integrals_index(const int i, const int j, const int k, const int l)
{

    const long int p = i < k ? (i + 1) : (k + 1);
    const long int r = i < k ? (k + 1) : (i + 1);
    const long int pp = p + ((r * r - r) >> 1);

    const long int q = j < l ? (j + 1) : (l + 1);
    const long int s = j < l ? (l + 1) : (j + 1);
    const long int qq = q + ((s * s - s) >> 1);

    const long int i1 = pp < qq ? pp : qq;
    const long int i2 = pp < qq ? qq : pp;
    return i1 + ((i2 * i2 - i2) >> 1);
}

void append_buffer(std::vector<long int>& buffer_i,
                   std::vector<double>& buffer_value,
                   const std::vector<double>& renorm,
                   const double precision,
                   const int n1, const int n2, const int n3, const int n4,
                   const int bf1_first, const int bf2_first, const int bf3_first, const int bf4_first,
                   const double* buf_1234, int& n_integrals)
{
    /**
          loop over all the basis function in the buffer
          If all the shell in the quartet are different,
          we have no permutation to handle.
          Else, you have to take care of the possible duplicate.
          */
    if (n1 != n2 && n1 != n3 && n1 != n4 && n2 != n3 && n2 != n4 && n3 != n4) {

        for (auto f1 = 0, f1234 = 0; f1 < n1; ++f1) {
            const int bf1 = f1 + bf1_first;
            const double fn1 = renorm[bf1];
            for (int f2 = 0; f2 < n2; ++f2) {
                const int bf2 = f2 + bf2_first;
                const double fn2 = renorm[bf2] * fn1;
                for (int f3 = 0; f3 < n3; ++f3) {
                    const int bf3 = f3 + bf3_first;
                    const double fn3 = renorm[bf3] * fn2;
                    for (int f4 = 0; f4 < n4; ++f4, ++f1234) {
                        const int bf4 = f4 + bf4_first;
                        const double fn4 = renorm[bf4] * fn3;

                        if (fabs(buf_1234[f1234]) > precision) {
                            if (n_integrals == buffer_i.size()) {
                              buffer_i.resize(n_integrals+1000000);
                              buffer_value.resize(n_integrals+1000000);
                            }
                            const long int key = bielec_integrals_index(bf1, bf3, bf2, bf4);
                            buffer_i[n_integrals] = key;
                            buffer_value[n_integrals] = buf_1234[f1234] * fn4;
                            n_integrals++;
                        }
                    }
                }
            }
        }
    }
    else {

        //Vector to store the uniq key of the quartet
        std::unordered_set<long int> uniq;

        for (int f1 = 0 ; f1 < n1; ++f1) {
            const int bf1 = f1 + bf1_first;
            const double fn1 = renorm[bf1];
            for (int f2 = 0; f2 < n2; ++f2) {
                const int bf2 = f2 + bf2_first;
                if (bf1 < bf2) break;
                const double fn2 = renorm[bf2] * fn1;
                for (int f3 = 0; f3 < n3; ++f3) {
                    const int bf3 = f3 + bf3_first;
                    const double fn3 = renorm[bf3] * fn2;
                    int f1234 = ( (f1*n2 + f2)*n3 + f3)*n4;
                    for (int f4 = 0; f4 < n4; ++f4, ++f1234) {
                        const int bf4 = f4 + bf4_first;
                        if (bf3 < bf4) break;
                        if (fabs(buf_1234[f1234]) > precision) {
                            const double fn4 = renorm[bf4] * fn3;
                            const long int key = bielec_integrals_index(bf1, bf3, bf2, bf4);
                            //Check if the quartet has been already computed
                            if (uniq.find(key) == uniq.end()) {
                                uniq.insert(key);
                                  if (n_integrals == buffer_i.size()) {
                                    buffer_i.resize(n_integrals+1000000);
                                    buffer_value.resize(n_integrals+1000000);
                                  }
                                  buffer_i[n_integrals] = key;
                                  buffer_value[n_integrals] = buf_1234[f1234] * fn4;
                                  n_integrals++;
                            }
                        }
                    }
                }
            }
        }
    }
}

Collector_Info initializeCollector(void* context, void * task_socket){

    int rc;
    rc = zmq_send(task_socket, "connect tcp", (size_t)11, 0);
    if (rc != 11) {
        perror("Error connecting to the task server");
        exit(EXIT_FAILURE);
    }

    char msg[512];
    rc = zmq_recv(task_socket, msg, (size_t)510, 0);
    msg[rc] = '\0';

    char reply[32], state[32], collector_address[128];
    int worker_id;
    sscanf(msg, "%s %s %d %s", reply, state, &worker_id, collector_address);
    if (strcmp(reply, "connect_reply")) {
        perror("Bad reply");
        exit(EXIT_FAILURE);
    }
    if (strcmp(state, "ao_integrals")) {
        perror("Bad state");
        exit(EXIT_FAILURE);
    }

    void* collector_socket = zmq_socket(context, ZMQ_PUSH);
    int i;
    i=300000    ; zmq_setsockopt(collector_socket, ZMQ_LINGER,&i,4);
    i=10        ; zmq_setsockopt(collector_socket, ZMQ_SNDHWM,&i,4);
    i=100000000 ; zmq_setsockopt(collector_socket, ZMQ_SNDBUF,&i,4);
    i=1         ; zmq_setsockopt(collector_socket, ZMQ_IMMEDIATE,&i,4);
    i=-1        ; zmq_setsockopt(collector_socket, ZMQ_SNDTIMEO,&i,4);

    rc = zmq_connect(collector_socket, collector_address);
    if (rc != 0) {
        perror("Error connecting the push socket");
        exit(EXIT_FAILURE);
    }

    Collector_Info ci;
    ci.collector_socket = collector_socket;
    ci.worker_id = worker_id;

    return ci;
}

void filinizeCollector(void * task_socket, Collector_Info ci){

    char msg[512], state[32], reply[32];
    int rc, msg_len;

    sprintf(msg, "disconnect ao_integrals %d", ci.worker_id);
    msg_len = strlen(msg);
    rc = zmq_send(task_socket, msg, msg_len, 0);
    if (rc != msg_len) {
        perror("Error sending disconnect");
        exit(EXIT_FAILURE);
    }
    
    rc = zmq_recv(task_socket, msg, (size_t)510, 0);
    msg[rc] = '\0';
    sscanf(msg, "%s %s", reply, state);
    if (strcmp(reply, "disconnect_reply") || strcmp(state, "ao_integrals")) {
        perror(msg);
        exit(EXIT_FAILURE);
    }

    rc = zmq_close(ci.collector_socket);

}

void sendBiTask(void * task_socket, Atom_Obs AO){

    char msg[512];

    /*TODO
    * compute MN shell pair for better load balancing
    */  
    for (auto s1 = 0l; s1 < AO.obs.size(); ++s1) {
        for (auto s2 = 0; s2 <= s1; ++s2) {

            sprintf(msg, "add_task ao_integrals %6d %6d", s1, s2);
            int rc = zmq_send(task_socket, msg, (size_t)36, 0);
            if (rc != 36) {
                perror("Error sending the task");
                exit(EXIT_FAILURE);
            }
            rc = zmq_recv(task_socket, msg, (size_t)510, 0);
            if (rc != 2) {
                perror(msg);
                exit(EXIT_FAILURE);
            }
        }
    }
}

void sendBiInt(void* task_socket, Collector_Info ci, Atom_Obs AO){
    libint2::init();

    const double precision = 1e-15;

    const libint2::BasisSet obs = AO.obs;
    const std::vector<double>& renorm = AO.renorm;

    const auto shell2bf = obs.shell2bf(); 
    const size_t nao = obs.nbf();
    const int n_integral_max = nao * nao;

    std::vector<long int> buffer_i(n_integral_max);
    std::vector<double> buffer_value(n_integral_max);


    libint2::TwoBodyEngine<libint2::Coulomb> coulomb_engine(obs.max_nprim(), obs.max_l(), 0);
    coulomb_engine.set_precision(std::min(precision, std::numeric_limits<double>::epsilon())); // shellset-dependent precision control will likely break positive definiteness
    // stick with this simple recipe

    char msg[512], reply[32];
    int rc, msg_len, task_id, s1, s2;
    const int worker_id = ci.worker_id;
    void * collector_socket = ci.collector_socket;

    while (1) {

        /* ***********
        Get the task
        ************ */
        sprintf(msg, "get_task ao_integrals %8d", worker_id);
        rc = zmq_send(task_socket, msg, (size_t)30, 0);
        if (rc != 30) {
            perror("Error connecting the push socket");
            exit(EXIT_FAILURE);
        }

        rc = zmq_recv(task_socket, msg, (size_t)510, 0);
        msg[rc] = '\0';

        sscanf(msg, "%s %d %d %d", reply, &task_id, &s1, &s2);
        if (task_id == 0)
            break;

        const auto bf1_first = shell2bf[s1]; // first basis function in this shell
        const auto n1 = obs[s1].size(); // number of basis functions in this shell

        const auto bf2_first = shell2bf[s2];
        const auto n2 = obs[s2].size();

        /* ***********
        Compute the task
        ************ */
        int n_integrals = 0;
        // Loop over s3 and s4
        // All the permutation are handle. (coherency with the generator task)
        for (auto s3 = 0; s3 <= s1; ++s3) {
            const auto bf3_first = shell2bf[s3];
            const auto n3 = obs[s3].size();

            const auto s4_max = (s1 == s3) ? s2 : s3;
            for (auto s4 = 0; s4 <= s4_max; ++s4) {
//                if (Schwartz(s1, s2) * Schwartz(s3, s4) < precision) continue;

                const auto bf4_first = shell2bf[s4];
                const auto n4 = obs[s4].size();
                const auto* buf_1234 = coulomb_engine.compute(obs[s1], obs[s2], obs[s3], obs[s4]);

                append_buffer(buffer_i, buffer_value, 
                              renorm, precision,
                              n1, n2, n3, n4,
                              bf1_first, bf2_first, bf3_first, bf4_first,
                              buf_1234, n_integrals);
            }
        }

        /* ***********
        Send the task
        ************ */

        // Send it to the collector_socket
        msg_len = 4;
        rc = zmq_send(collector_socket, &n_integrals, msg_len, ZMQ_SNDMORE);
        if (rc != 4) {
            perror("Error pushing n_integrals");
            exit(EXIT_FAILURE);
        }

        msg_len = n_integrals * sizeof(long int);
        rc = zmq_send(collector_socket, &buffer_i[0], msg_len, ZMQ_SNDMORE);
        if (rc != msg_len) {
            perror("Error pushing buffer_i");
            exit(EXIT_FAILURE);
        }

        msg_len = n_integrals * sizeof(double);
        rc = zmq_send(collector_socket, &buffer_value[0], msg_len, ZMQ_SNDMORE);
        if (rc != msg_len) {
            perror("Error pushing buffer_value");
            exit(EXIT_FAILURE);
        }

        msg_len = 4;
        rc = zmq_send(collector_socket, &task_id, msg_len, 0);
        if (rc != 4) {
            perror("Error pushing task_id");
            exit(EXIT_FAILURE);
        }
    }

    libint2::finalize();
}