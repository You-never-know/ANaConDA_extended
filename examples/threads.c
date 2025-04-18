#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 5

int shared_counter = 0; // Shared variable

void* increment_counter(void* arg) {
    for (int i = 0; i < 10000; i++) {
        shared_counter++;  // Atomicity violation: Read-Modify-Write is not atomic
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, increment_counter, NULL) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Expected value: NUM_THREADS * 10000, but due to race conditions, it will likely be lower
    printf("Final counter value: %d (Expected: %d)\n", shared_counter, NUM_THREADS * 10000);
    return 0;
}
