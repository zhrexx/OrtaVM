#ifndef INT_H
#define INT_H
#include <stdlib.h>
#include <time.h>

int random_int(int min, int max) {
    srand(time(NULL));
    srand(rand());

    return min + rand() % (max - min + 1);
}

#endif
