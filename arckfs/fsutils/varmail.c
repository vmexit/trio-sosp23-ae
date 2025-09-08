#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <stdatomic.h>

#define DIR_PATH "/sufs"
#define NFILES 10000 // 2^n is better for optimize modulo operation..?
#define MAX_THREADS 48
#define IOBUF_SIZE (1024 * 1024)      // 1MB
#define APPEND_SIZE (16 * 1024)       // 16KB

#define CACHE_LINE_SIZE 64

#define FILE_SIZE 16834 
#define PREALLOC 80

#define TOTAL_WORKS 960000

int nthreads = 0;

// fast random

static __thread uint32_t FAST_RANDOM_NEXT = 1;

void fast_random_set_seed(uint32_t seed) {
    FAST_RANDOM_NEXT = seed;
}

uint32_t fast_random() {
    uint32_t new_val = FAST_RANDOM_NEXT * 1103515245 + 12345;
    FAST_RANDOM_NEXT = new_val;
    return (new_val / 65536) % 32768;
}

// fileset


// cache-padded atomic flag
typedef struct {
    atomic_flag flag;
    char padding[CACHE_LINE_SIZE - sizeof(atomic_flag)];
} padded_atomic_flag;

// fine-grained locking for each file in fileset
padded_atomic_flag file_locks[NFILES];

int pick_and_lock() {
    while (1) {
        int idx = fast_random() % NFILES;
        if (!atomic_flag_test_and_set(&file_locks[idx].flag)) {
            return idx;
        }
    }
}



void prework() {
    int num_files = NFILES * PREALLOC / 100;
    char buffer[FILE_SIZE];
    memset(buffer, 0xDEADBEEF, FILE_SIZE);

    for (int i = 0; i < num_files; ++i) {
        char path[256];
        snprintf(path, sizeof(path), "%s/%06d", DIR_PATH, i);

        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open failed");
            exit(EXIT_FAILURE);
        }

        ssize_t written = write(fd, buffer, FILE_SIZE);
        if (written != FILE_SIZE) {
            fprintf(stderr, "write error on %s: %s\n", path, strerror(errno));
            close(fd);
            exit(EXIT_FAILURE);
        }

        close(fd);
    }

    // printf("prework done\n");
}

char iobuf[IOBUF_SIZE];

void work() {
    int file_id= pick_and_lock();
    char path[256];
    snprintf(path, sizeof(path), "%s/%06d", DIR_PATH, file_id);

    // delete
    unlink(path);
    //printf("1");

    // create
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("create");
    }

    // append data
    write(fd, iobuf, APPEND_SIZE);

    // fsync
    fsync(fd);

    // close
    close(fd);

    // open
    fd = open(path, O_RDONLY);

    // read
    read(fd, iobuf, IOBUF_SIZE);

    // append
    write(fd, iobuf, APPEND_SIZE);

    // fsync
    fsync(fd);

    // close
    close(fd);

    // open
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        // read
        read(fd, iobuf, IOBUF_SIZE);

        // close
        close(fd);
    }

    atomic_flag_clear(&file_locks[file_id].flag);
}

void rand_fill(char *buf, size_t size) {
    for (size_t i = 0; i < size; ++i)
        buf[i] = rand() % 256;
}

void* worker_thread(void* arg) {
    fast_random_set_seed(time(NULL) ^ pthread_self());

    int epoch = TOTAL_WORKS / nthreads;
    for(int i=0;i<epoch;i++) {
       work();
    }

    return NULL;
}

double time_diff_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) +
           (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        // printf("사용법: %s <숫자>\n", argv[0]);
        return 1;
    }

    nthreads = atoi(argv[1]);  // 문자열을 정수로 변환
    // printf("Number of threads: %d\n", nthreads);

    pthread_t threads[MAX_THREADS];
    int tids[MAX_THREADS];

    memset(iobuf, 0xDEADBEEF, IOBUF_SIZE);
    prework();


    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < nthreads; ++i) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, worker_thread, &tids[i]);
    }

    for (int i = 0; i < nthreads; ++i) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double duration = time_diff_sec(start, end);
    long total_ops = TOTAL_WORKS / nthreads * nthreads * 13;; //atomic_load(&global_opcount);
    double throughput = total_ops / duration;

    //printf("Finished workload simulation.\n");
    //printf("Time taken: %.3f sec\n", duration);
    //printf("Total ops: %ld\n", total_ops);
    //printf("Throughput: %.2f ops/sec\n", throughput);
    printf("%.2f\n", throughput);

    return 0;
}