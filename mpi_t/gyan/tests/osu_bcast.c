#define BENCHMARK "OSU MPI%s Broadcast Latency Test"
/*
 * Copyright (C) 2002-2013 the Network-Based Computing Laboratory
 * (NBCL), The Ohio State University. 
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */

#include "osu_coll.h"

int main(int argc, char *argv[])
{
    int i = 0, rank, size;
    int skip, numprocs;
    double avg_time = 0.0, max_time = 0.0, min_time = 0.0;
    double latency = 0.0, t_start = 0.0, t_stop = 0.0;
    double timer=0.0;
    char *buffer=NULL;
    int max_msg_size = 1048576, full = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    if (process_args(argc, argv, rank, &max_msg_size, &full)) {
        MPI_Finalize();
        return EXIT_SUCCESS;
    }
    
    if(numprocs < 2) {
        if(rank == 0) {
            fprintf(stderr, "This test requires at least two processes\n");
        }

        MPI_Finalize();

        return EXIT_FAILURE;
    }

    print_header(rank, full);

    buffer = malloc(max_msg_size * sizeof(char));
    if(NULL == buffer) {
        fprintf(stderr, "malloc failed.\n");
        exit(1);
    }
    
    memset(buffer,1, max_msg_size);

    for(size=1; size <=max_msg_size; size *= 2) {
        if(size > LARGE_MESSAGE_SIZE) {
            skip = SKIP_LARGE;
            iterations = iterations_large;
        }
        else {
            skip = SKIP;
        }

        timer=0.0;        
        for(i=0; i < iterations + skip ; i++) {
            t_start = MPI_Wtime();
            MPI_Bcast(buffer, size, MPI_CHAR, 0, MPI_COMM_WORLD);
            t_stop = MPI_Wtime();

            if(i>=skip){
                timer+=t_stop-t_start;
            } 
            MPI_Barrier(MPI_COMM_WORLD);

        }
            
        MPI_Barrier(MPI_COMM_WORLD);

        latency = (timer * 1e6) / iterations;

        MPI_Reduce(&latency, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0,
                MPI_COMM_WORLD);
        MPI_Reduce(&latency, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, 
                MPI_COMM_WORLD);
        MPI_Reduce(&latency, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0,
                MPI_COMM_WORLD);
        avg_time = avg_time/numprocs;

        print_data(rank, full, size, avg_time, min_time, max_time, iterations);
    }

    free(buffer);  
    MPI_Finalize();
   
    return EXIT_SUCCESS;
}

/* vi: set sw=4 sts=4 tw=80: */
