#include <pthread.h>

int sum = 0;
pthread_mutex_t mtx;

void plus() {
   pthread_mutex_lock(&mtx);
   sum++;
   pthread_mutex_unlock(&mtx);
}

int main() {
    pthread_t p1;
    pthread_t p2;
    pthread_create(&p1, NULL, (void* (*)(void*))plus, NULL);
    pthread_create(&p2, NULL, (void* (*)(void*))plus, NULL);
    pthread_detach(p1);
    pthread_join(p2, NULL);
    return 0;
}