#include <stdlib.h>

#define N 100000

int main(int argc, char *argv[]) {
    int *a = malloc(N*sizeof(int));
    int i;
    int b;

    for (i = 0; i < N; i++)
        b = a[i];

    for (i = 0; i < N/10; i++)
        a[i] = 0;

}
