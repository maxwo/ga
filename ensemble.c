#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

typedef unsigned int element_t;

typedef unsigned short int bucket_t;

typedef struct {
        size_t size;
        bucket_t* elements;
} ensemble_t;

#define ENS_ELEMENT_INDEX(element) element / (8 * sizeof(bucket_t))
#define ENS_ELEMENT_SHIFT(element) 8 * sizeof(bucket_t) * (ENS_ELEMENT_INDEX(element))

ensemble_t* ens_create(const size_t size) {
        ensemble_t* ensemble = calloc(1, sizeof(ensemble_t));
        ensemble->size = size;
        ensemble->elements = calloc(1 + ENS_ELEMENT_INDEX(size), sizeof(bucket_t));
        return ensemble;
}

int ens_contains(const ensemble_t* ensemble, const element_t element) {
        const bucket_t element_test = element - ENS_ELEMENT_SHIFT(element);
        const size_t bucket_test = ENS_ELEMENT_INDEX(element);
        const bucket_t mask_to_test = ((bucket_t) 1) << element_test;
        // printf("    Element: %u\n", element);
        // printf("Bit to test: %hu\n", element_test);
        // printf("       Mask: %hu\n", mask_to_test);
        // printf("  Bucket ID: %zu\n", bucket_test);
        // printf("      Value: %hu\n", *(ensemble->elements + bucket_test));
        return (*(ensemble->elements + bucket_test) & mask_to_test) > 0;
}

void ens_print(const ensemble_t* ensemble) {
        printf("Ensemble %p[", ensemble);
        unsigned int first = 1;
        for (element_t current=0 ; current<ensemble->size ; current++) {
                if (ens_contains(ensemble, current)) {
                        if (first != 1) {
                                printf("-");
                                first = 0;
                        }
                        printf("%u", current);
                }
        }
        printf("]\n");
}

void ens_add_element(ensemble_t* ensemble, const element_t element) {
        const bucket_t element_test = element - ENS_ELEMENT_SHIFT(element);
        const size_t bucket_test = ENS_ELEMENT_INDEX(element);
        const bucket_t mask_to_add = ((bucket_t) 1) << element_test;
        // printf("    Element: %u\n", element);
        // printf("Bit to test: %hu\n", element_test);
        // printf("       Mask: %hu\n", mask_to_add);
        // printf("  Bucket ID: %zu\n", bucket_test);
        // printf("      Value: %hu\n", *(ensemble->elements + bucket_test));
        *(ensemble->elements + bucket_test) |= mask_to_add;
        // printf("      After: %hu\n", *(ensemble->elements + bucket_test));
}

void ens_add_elements(ensemble_t* ensemble, const element_t* elements, const size_t size) {
        for (element_t i=0 ; i<size ; i++) {
                ens_add_element(ensemble, *(elements + i));
        }
}

void ens_remove_element(ensemble_t* ensemble, const element_t element) {
        const bucket_t element_test = element - ENS_ELEMENT_SHIFT(element);
        const size_t bucket_test = ENS_ELEMENT_INDEX(element);
        const bucket_t mask_to_remove = !(((bucket_t) 1) << element_test);
        *(ensemble->elements + bucket_test) |= mask_to_remove;
}

void ens_remove_elements(ensemble_t* ensemble, const element_t* elements, const size_t size) {
        for (size_t i=0 ; i<size ; i++) {
                ens_remove_element(ensemble, *(elements + i));
        }
}

ensemble_t* ens_of(const size_t element_count, ...) {
        va_list valist;
        va_start(valist, element_count);
        size_t max_element = 0;
        for (size_t i = 0; i<element_count; i++) {
                element_t current_element = va_arg(valist, element_t);
                if (current_element > max_element) {
                        max_element = current_element;
                }
        }
        va_end(valist);

        ensemble_t* ensemble = ens_create(max_element);
        va_start(valist, element_count);
        for (size_t i = 0; i<element_count; i++) {
                element_t current_element = va_arg(valist, element_t);
                ens_add_element(ensemble, current_element);
        }

        return ensemble;
}

void ens_destroy(ensemble_t* ensemble) {
        free(ensemble->elements);
        free(ensemble);
}
