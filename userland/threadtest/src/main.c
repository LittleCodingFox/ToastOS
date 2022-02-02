#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>

void *printfn(void *ptr);

int main(int argc, char **argv)
{
    pthread_t t1, t2;
    char *m1 = "Thread 1";
    char *m2 = "Thread 2";

    int result = pthread_create(&t1, NULL, printfn, (void *)m1);
    int result2 = pthread_create(&t2, NULL, printfn, (void *)m2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("return 1: %d\n", result);
    printf("return 2: %d\n", result2);

    return 0;
}

void *printfn(void *ptr)
{
    char *message = (char *)ptr;

    printf("%s\n", message);

    return NULL;
}
