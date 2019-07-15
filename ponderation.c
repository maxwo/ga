#include <stdlib.h>
#include <stdio.h>

typedef double probability_t;

typedef struct {
        size_t size;
        probability_t sum;
        probability_t* probabilities;
} ponderation_t;

ponderation_t* pond_create(size_t size) {
        ponderation_t* ponderation = calloc(1, sizeof(ponderation_t));
        ponderation->size = size;
        ponderation->sum = 0;
        ponderation->probabilities = calloc(size, sizeof(probability_t));
        return ponderation;
}

void ponderation_reset(ponderation_t* ponderation) {
        ponderation->sum = 0;
        for (size_t i=0 ; i<ponderation->size ; i++) {
                ponderation->probabilities[i] = 0;
        }
}

void pond_set_probability(ponderation_t* ponderation, const size_t index, const probability_t probability) {
        ponderation->sum += probability;
        ponderation->probabilities[index] = probability;
}

probability_t random_probability() {
        return (double) rand() / (double) RAND_MAX;
}

size_t pond_random(const ponderation_t* ponderation) {
        probability_t random = random_probability() * ponderation->sum;
        for (size_t i=0 ; i<ponderation->size ; i++) {
                if (random <= ponderation->probabilities[i]) {
                        return i;
                }
                random -= ponderation->probabilities[i];
        }
        printf("%f out of %f\n", random, ponderation->sum);
        return -1;
}

void pond_destroy(ponderation_t* ponderation) {
        free(ponderation->probabilities);
        free(ponderation);
}
