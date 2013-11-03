#define BENCHMARK "OSU MPI%s Bandwidth Test"
/*
 * Copyright (C) 2002-2013 the Network-Based Computing Laboratory
 * (NBCL), The Ohio State University. 
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */

#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#ifdef _ENABLE_OPENACC_
#include <openacc.h>
#endif

#ifdef _ENABLE_CUDA_
#include <cuda.h>
#include <cuda_runtime.h>
#endif

#ifdef PACKAGE_VERSION
#   define HEADER "# " BENCHMARK " v" PACKAGE_VERSION "\n"
#else
#   define HEADER "# " BENCHMARK "\n"
#endif

#ifndef FIELD_WIDTH
#   define FIELD_WIDTH 20
#endif

#ifndef FLOAT_PRECISION
#   define FLOAT_PRECISION 2
#endif

#define MAX_REQ_NUM 1000

#define MAX_ALIGNMENT 65536
#define MAX_MSG_SIZE (1<<22)
#define MYBUFSIZE (MAX_MSG_SIZE + MAX_ALIGNMENT)

#define LOOP_LARGE  20
#define WINDOW_SIZE_LARGE  64
#define SKIP_LARGE  2
#define LARGE_MESSAGE_SIZE  8192

#ifdef _ENABLE_OPENACC_
#   define OPENACC_ENABLED 1
#else
#   define OPENACC_ENABLED 0
#endif

#ifdef _ENABLE_CUDA_
#   define CUDA_ENABLED 1
#else
#   define CUDA_ENABLED 0
#endif

char s_buf_original[MYBUFSIZE];
char r_buf_original[MYBUFSIZE];

MPI_Request request[MAX_REQ_NUM];
MPI_Status  reqstat[MAX_REQ_NUM];

#ifdef _ENABLE_CUDA_
CUcontext cuContext;
#endif

enum po_ret_type {
    po_cuda_not_avail,
    po_openacc_not_avail,
    po_bad_usage,
    po_help_message,
    po_okay,
};

enum accel_type {
    none,
    cuda,
    openacc
};

struct {
    char src;
    char dst;
    enum accel_type accel;
} options;

void usage (void);
int init_cuda_context (void);
int destroy_cuda_context (void);
int process_options (int argc, char *argv[]);
int allocate_memory (char **sbuf, char **rbuf, int rank);
void print_header (int rank);
void touch_data (void *sbuf, void *rbuf, int rank, size_t size);
void free_memory (void *sbuf, void *rbuf, int rank);

int
main (int argc, char *argv[])
{
    int myid, numprocs, i, j;
    int size;
    char *s_buf, *r_buf;
    double t_start = 0.0, t_end = 0.0, t = 0.0;
    int loop = 100;
    int window_size = 64;
    int skip = 10;
    int po_ret = process_options(argc, argv);

    if (po_okay == po_ret && cuda == options.accel) {
        if (init_cuda_context()) {
            fprintf(stderr, "Error initializing cuda context\n");
            exit(EXIT_FAILURE);
        }
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    if (0 == myid) {
        switch (po_ret) {
            case po_cuda_not_avail:
                fprintf(stderr, "CUDA support not enabled.  Please recompile "
                        "benchmark with CUDA support.\n");
                break;
            case po_openacc_not_avail:
                fprintf(stderr, "OPENACC support not enabled.  Please "
                        "recompile benchmark with OPENACC support.\n");
                break;
            case po_bad_usage:
            case po_help_message:
                usage();
                break;
        }
    }

    switch (po_ret) {
        case po_cuda_not_avail:
        case po_openacc_not_avail:
        case po_bad_usage:
            MPI_Finalize();
            exit(EXIT_FAILURE);
        case po_help_message:
            MPI_Finalize();
            exit(EXIT_SUCCESS);
        case po_okay:
            break;
    }

    if(numprocs != 2) {
        if(myid == 0) {
            fprintf(stderr, "This test requires exactly two processes\n");
        }

        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    if (allocate_memory(&s_buf, &r_buf, myid)) {
        /* Error allocating memory */
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    print_header(myid);

    /* Bandwidth test */
    for(size = 1; size <= MAX_MSG_SIZE; size *= 2) {
        touch_data(s_buf, r_buf, myid, size);

        if(size > LARGE_MESSAGE_SIZE) {
            loop = LOOP_LARGE;
            skip = SKIP_LARGE;
            window_size = WINDOW_SIZE_LARGE;
        }

        if(myid == 0) {
            for(i = 0; i < loop + skip; i++) {
                if(i == skip) {
                    t_start = MPI_Wtime();
                }

                for(j = 0; j < window_size; j++) {
                    MPI_Isend(s_buf, size, MPI_CHAR, 1, 100, MPI_COMM_WORLD,
                            request + j);
                }

                MPI_Waitall(window_size, request, reqstat);
                MPI_Recv(r_buf, 4, MPI_CHAR, 1, 101, MPI_COMM_WORLD,
                        &reqstat[0]);
            }

            t_end = MPI_Wtime();
            t = t_end - t_start;
        }

        else if(myid == 1) {
            for(i = 0; i < loop + skip; i++) {
                for(j = 0; j < window_size; j++) {
                    MPI_Irecv(r_buf, size, MPI_CHAR, 0, 100, MPI_COMM_WORLD,
                            request + j);
                }

                MPI_Waitall(window_size, request, reqstat);
                MPI_Send(s_buf, 4, MPI_CHAR, 0, 101, MPI_COMM_WORLD);
            }
        }

        if(myid == 0) {
            double tmp = size / 1e6 * loop * window_size;

            fprintf(stdout, "%-*d%*.*f\n", 10, size, FIELD_WIDTH,
                    FLOAT_PRECISION, tmp / t);
            fflush(stdout);
        }
    }

    free_memory(s_buf, r_buf, myid);
    MPI_Finalize();

    if (cuda == options.accel) {
        if (destroy_cuda_context()) {
            fprintf(stderr, "Error destroying cuda context\n");
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

void
usage (void)
{
    if (CUDA_ENABLED || OPENACC_ENABLED) {
        printf("Usage: osu_bw [options] [RANK0 RANK1]\n\n");
        printf("RANK0 and RANK1 may be `D' or `H' which specifies whether\n"
               "the buffer is allocated on the accelerator device or host\n"
               "memory for each mpi rank\n\n");
    }

    else {
        printf("Usage: osu_bw [options]\n\n");
    }
    
    printf("options:\n");

    if (CUDA_ENABLED || OPENACC_ENABLED) {
        printf("  -d TYPE       accelerator device buffers can be of TYPE "
                "`cuda' or `openacc'\n");
    }

    printf("  -h            print this help message\n");
    fflush(stdout);
}

int
process_options (int argc, char *argv[])
{
    extern char * optarg;
    extern int optind;

    char const * optstring = (CUDA_ENABLED || OPENACC_ENABLED) ? "+d:h" : "+h";
    int c;

    /*
     * set default options
     */
    options.src = 'H';
    options.dst = 'H';

    if (CUDA_ENABLED) {
        options.accel = cuda;
    }

    else if (OPENACC_ENABLED) {
        options.accel = openacc;
    }

    else {
        options.accel = none;
    }

    while((c = getopt(argc, argv, optstring)) != -1) {
        switch (c) {
            case 'd':
                /* optarg should contain cuda or openacc */
                if (0 == strncasecmp(optarg, "cuda", 10)) {
                    if (!CUDA_ENABLED) {
                        return po_cuda_not_avail;
                    }
                    options.accel = cuda;
                }

                else if (0 == strncasecmp(optarg, "openacc", 10)) {
                    if (!OPENACC_ENABLED) {
                        return po_openacc_not_avail;
                    }
                    options.accel = openacc;
                }

                else {
                    return po_bad_usage;
                }
                break;
            case 'h':
                return po_help_message;
            default:
                return po_bad_usage;
        }
    }

    if (CUDA_ENABLED || OPENACC_ENABLED) {
        if ((optind + 2) == argc) {
            options.src = argv[optind][0];
            options.dst = argv[optind + 1][0];

            switch (options.src) {
                case 'D':
                case 'H':
                    break;
                default:
                    return po_bad_usage;
            }

            switch (options.dst) {
                case 'D':
                case 'H':
                    break;
                default:
                    return po_bad_usage;
            }
        }

        else if (optind != argc) {
            return po_bad_usage;
        }
    }

    return po_okay;
}

int
init_cuda_context (void)
{
#ifdef _ENABLE_CUDA_
    CUresult curesult = CUDA_SUCCESS;
    CUdevice cuDevice;
    int local_rank, dev_count;
    int dev_id = 0;
    char * str;

    if ((str = getenv("LOCAL_RANK")) != NULL) {
        cudaGetDeviceCount(&dev_count);
        local_rank = atoi(str);
        dev_id = local_rank % dev_count;
    }

    curesult = cuInit(0);
    if (curesult != CUDA_SUCCESS) {
        return 1;
    }

    curesult = cuDeviceGet(&cuDevice, dev_id);
    if (curesult != CUDA_SUCCESS) {
        return 1;
    }

    curesult = cuCtxCreate(&cuContext, 0, cuDevice);
    if (curesult != CUDA_SUCCESS) {
        return 1;
    }
#endif
    return 0;
}

int
allocate_device_buffer (char ** buffer)
{
#ifdef _ENABLE_CUDA_
    cudaError_t cuerr = cudaSuccess;
#endif

    switch (options.accel) {
#ifdef _ENABLE_CUDA_
        case cuda:
            cuerr = cudaMalloc((void **)buffer, MYBUFSIZE);

            if (cudaSuccess != cuerr) {
                fprintf(stderr, "Could not allocate device memory\n");
                return 1;
            }
            break;
#endif
#ifdef _ENABLE_OPENACC_
        case openacc:
            *buffer = acc_malloc(MYBUFSIZE);
            if (NULL == *buffer) {
                fprintf(stderr, "Could not allocate device memory\n");
                return 1;
            }
            break;
#endif
        default:
            fprintf(stderr, "Could not allocate device memory\n");
            return 1;
    }

    return 0;
}

void *
align_buffer (void * ptr, unsigned long align_size)
{
    return (void *)(((unsigned long)ptr + (align_size - 1)) / align_size *
            align_size);
}

int
allocate_memory (char ** sbuf, char ** rbuf, int rank)
{
    unsigned long align_size = getpagesize();

    assert(align_size <= MAX_ALIGNMENT);

    switch (rank) {
        case 0:
            if ('D' == options.src) {
                if (allocate_device_buffer(sbuf)) {
                    fprintf(stderr, "Error allocating cuda memory\n");
                    return 1;
                }

                if (allocate_device_buffer(rbuf)) {
                    fprintf(stderr, "Error allocating cuda memory\n");
                    return 1;
                }
            }

            else {
                *sbuf = align_buffer(s_buf_original, align_size);
                *rbuf = align_buffer(r_buf_original, align_size);
            }
            break;
        case 1:
            if ('D' == options.dst) {
                if (allocate_device_buffer(sbuf)) {
                    fprintf(stderr, "Error allocating cuda memory\n");
                    return 1;
                }

                if (allocate_device_buffer(rbuf)) {
                    fprintf(stderr, "Error allocating cuda memory\n");
                    return 1;
                }
            }

            else {
                *sbuf = align_buffer(s_buf_original, align_size);
                *rbuf = align_buffer(r_buf_original, align_size);
            }
            break;
    }

    return 0;
}

void
print_header (int rank)
{
    if (0 == rank) {
        switch (options.accel) {
            case cuda:
                printf(HEADER, "-CUDA");
                break;
            case openacc:
                printf(HEADER, "-OPENACC");
                break;
            default:
                printf(HEADER, "");
                break;
        }

        switch (options.accel) {
            case cuda:
            case openacc:
                printf("# Send Buffer on %s and Receive Buffer on %s\n",
                        'D' == options.src ? "DEVICE (D)" : "HOST (H)",
                        'D' == options.dst ? "DEVICE (D)" : "HOST (H)");
            default:
                printf("%-*s%*s\n", 10, "# Size", FIELD_WIDTH, "Bandwidth (MB/s)");
                fflush(stdout);
        }
    }
}

void
set_device_memory (void * ptr, int data, size_t size)
{
#ifdef _ENABLE_OPENACC_
    size_t i;
    char * p = (char *)ptr;
#endif

    switch (options.accel) {
#ifdef _ENABLE_CUDA_
        case cuda:
            cudaMemset(ptr, data, size);
            break;
#endif
#ifdef _ENABLE_OPENACC_
        case openacc:
#pragma acc parallel loop deviceptr(p)
            for(i = 0; i < size; i++) {
                p[i] = data;
            }
            break;
#endif
        default:
            break;
    }
}

void
touch_data (void * sbuf, void * rbuf, int rank, size_t size)
{
    if ((0 == rank && 'H' == options.src) ||
            (1 == rank && 'H' == options.dst)) {
        memset(sbuf, 'a', size);
        memset(rbuf, 'b', size);
    } else {
        set_device_memory(sbuf, 'a', size);
        set_device_memory(rbuf, 'b', size);
    }
}

int
free_device_buffer (void * buf)
{
    switch (options.accel) {
#ifdef _ENABLE_CUDA_
        case cuda:
            cudaFree(buf);
            break;
#endif
#ifdef _ENABLE_OPENACC_
        case openacc:
            acc_free(buf);
            break;
#endif
        default:
            /* unknown device */
            return 1;
    }

    return 0;
}

int
destroy_cuda_context (void)
{
#ifdef _ENABLE_CUDA_
    CUresult curesult = CUDA_SUCCESS;
    curesult = cuCtxDestroy(cuContext);   

    if (curesult != CUDA_SUCCESS) {
        return 1;
    }  
#endif
    return 0;
}

void
free_memory (void * sbuf, void * rbuf, int rank)
{
    switch (rank) {
        case 0:
            if ('D' == options.src) {
                free_device_buffer(sbuf);
                free_device_buffer(rbuf);
            }
            break;
        case 1:
            if ('D' == options.dst) {
                free_device_buffer(sbuf);
                free_device_buffer(rbuf);
            }
            break;
    }
}

/* vi:set sw=4 sts=4 tw=80: */
