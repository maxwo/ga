#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <float.h>
#include <omp.h>
#include <signal.h>
#include <time.h>

#include "graph.c"
#include "ponderation.c"

#ifndef GA_PROBLEM_TYPE
#define GA_PROBLEM_TYPE graph_t
#endif

#ifndef GA_SOLUTION_TYPE
#define GA_SOLUTION_TYPE path_t
#endif

typedef double score_t;

typedef struct {
        score_t score;
        GA_SOLUTION_TYPE* solution;
} individual_t;

typedef struct {
        size_t size;
        individual_t** individuals;
} population_t;

typedef struct {
        GA_PROBLEM_TYPE* graph;

        size_t population_size;
        int elitism;
        probability_t survival_rate;
        probability_t mutation_rate;

        GA_SOLUTION_TYPE* (*generate) (const GA_PROBLEM_TYPE*);
        void (*regularize) (const GA_PROBLEM_TYPE*, GA_SOLUTION_TYPE*);
        int (*compare) (const GA_PROBLEM_TYPE*, const GA_SOLUTION_TYPE*, const GA_SOLUTION_TYPE*);
        score_t (*evaluate) (const GA_PROBLEM_TYPE*, const GA_SOLUTION_TYPE*);
        GA_SOLUTION_TYPE* (*copy) (const GA_PROBLEM_TYPE*, const GA_SOLUTION_TYPE*);
        GA_SOLUTION_TYPE* (*cross) (const GA_PROBLEM_TYPE*, const GA_SOLUTION_TYPE*, const GA_SOLUTION_TYPE*);
        void (*mutate) (const GA_PROBLEM_TYPE*, GA_SOLUTION_TYPE*);
        void (*destroy) (GA_SOLUTION_TYPE*);

} ga_parameters_t;


void ga_regularize_individual(const ga_parameters_t* parameters, const individual_t* individual) {
        parameters->regularize(parameters->graph, individual->solution);
}

score_t ga_evaluate_individual_score(const ga_parameters_t* parameters, individual_t* individual) {
        if (individual->score == -1) {
                individual->score = parameters->evaluate(parameters->graph, individual->solution);
        }
        return individual->score;
}

individual_t* ga_generate_random_individual(const ga_parameters_t* parameters) {
        individual_t* individual = calloc(1, sizeof(individual_t));
        individual->score = -1;
        individual->solution = parameters->generate(parameters->graph);
        ga_regularize_individual(parameters, individual);
        ga_evaluate_individual_score(parameters, individual);
        return individual;
}

size_t ga_individual_index(const ga_parameters_t* parameters, const population_t* population, const individual_t* individual) {
        for (size_t i=0 ; i<population->size ; i++) {
                if (population->individuals[i] == NULL) {
                        return -1;
                } else if (parameters->compare(parameters->graph, population->individuals[i]->solution, individual->solution)==0) {
                        return i;
                }
        }
        return -1;
}

population_t* ga_generate_empty_population(const size_t size) {
        population_t* population = calloc(1, sizeof(population_t));
        population->size = size;
        population->individuals = calloc(population->size, sizeof(individual_t*));
        return population;
}

population_t* ga_generate_random_population(const ga_parameters_t* parameters) {
        population_t* population = ga_generate_empty_population(parameters->population_size);

        size_t i;
        #pragma omp parallel for
        for (i=0 ; i<population->size ; i++) {
                // printf("individual %lu on thread %d\n", i, omp_get_thread_num());
                population->individuals[i] = ga_generate_random_individual(parameters);
                if (ga_individual_index(parameters, population, population->individuals[i]) != i) {
                        i--;
                }
        }
        return population;
}

void ga_destroy_individual(const ga_parameters_t* parameters, individual_t* individual) {
        parameters->destroy(individual->solution);
        free(individual);
}

void ga_destroy_population(const ga_parameters_t* parameters, population_t* population) {
        for (size_t i=0 ; i<population->size ; i++) {
                ga_destroy_individual(parameters, population->individuals[i]);
        }
        free(population->individuals);
        free(population);
}

individual_t* ga_copy_individual(const ga_parameters_t* parameters, const individual_t* individual) {
        individual_t* copy = calloc(1, sizeof(individual_t));
        copy->score = individual->score;
        copy->solution = parameters->copy(parameters->graph, individual->solution);
        return copy;
}

int ga_compare_individuals(const void* a, const void* b) {
        score_t score_a = (*(individual_t**) a)->score;
        score_t score_b = (*(individual_t**) b)->score;
        score_t diff = score_a - score_b;
        if (diff < 0) {
                return -1;
        }
        if (diff > 0) {
                return 1;
        }
        return 0;
}

void ga_evaluate_population(const ga_parameters_t* parameters, population_t* population) {
        qsort(population->individuals, population->size, sizeof(individual_t*), ga_compare_individuals);
}

individual_t* ga_cross_individuals(const ga_parameters_t* parameters, const individual_t* parent1, const individual_t* parent2) {
        individual_t* child = calloc(1, sizeof(individual_t));
        child->score = -1;
        child->solution = parameters->cross(parameters->graph, parent1->solution, parent2->solution);
        ga_regularize_individual(parameters, child);
        ga_evaluate_individual_score(parameters, child);
        return child;
}

const individual_t* ga_select_individual(const population_t* population, const ponderation_t* ponderation) {
        const size_t random_index = pond_random(ponderation);
        return population->individuals[random_index];
}

void ga_mutate(const ga_parameters_t* parameters, GA_SOLUTION_TYPE* solution) {
        parameters->mutate(parameters->graph, solution);
}

individual_t* ga_generate_individual(const ga_parameters_t* parameters, const population_t* population, const ponderation_t* score_ponderation) {
        if (random_probability() < parameters->survival_rate) {
                return ga_copy_individual(parameters, ga_select_individual(population, score_ponderation));
        }

        const individual_t* parent1 = ga_select_individual(population, score_ponderation);
        const individual_t* parent2 = ga_select_individual(population, score_ponderation);
        individual_t* child = ga_cross_individuals(parameters, parent1, parent2);

        if (random_probability() < parameters->mutation_rate) {
                ga_mutate(parameters, child->solution);
        }

        return child;
}

population_t* ga_generate_next_population(const ga_parameters_t* parameters, const population_t* population) {
        ponderation_t* score_ponderation = pond_create(parameters->population_size);
        for (size_t i=0 ; i<population->size ; i++) {
                pond_set_probability(score_ponderation, i, 1 / population->individuals[i]->score);
        }

        population_t* next_population = ga_generate_empty_population(parameters->population_size);

        if (parameters->elitism) {
                next_population->individuals[0] = ga_copy_individual(parameters, population->individuals[0]);
        }

        size_t i;
        #pragma omp parallel for
        for (i = parameters->elitism ? 1 : 0 ; i<parameters->population_size ; i++) {
                // printf("individual %lu on thread %d\n", i, omp_get_thread_num());
                do {
                        individual_t* individual = ga_generate_individual(parameters, population, score_ponderation);
                        for (size_t j=0 ; j<next_population->size && individual!=NULL ; j++) {
                                if (next_population->individuals[j] != NULL) {
                                        int comparaison = parameters->compare(parameters->graph, next_population->individuals[j]->solution, individual->solution);
                                        if (comparaison == 0) {
                                                ga_destroy_individual(parameters, individual);
                                                individual = NULL;
                                        }
                                }
                        }
                        next_population->individuals[i] = individual;
                } while (next_population->individuals[i] == NULL);
        }

        pond_destroy(score_ponderation);
        return next_population;
}

void ga_print_population(const GA_PROBLEM_TYPE* graph, const population_t* population) {
        printf("Population %p[size=%zu,min=%f,90th=%f,75th=%f,med=%f,25th=%f,max=%f]\n", population, population->size,
                population->individuals[0]->score,
                population->individuals[population->size / 10]->score,
                population->individuals[population->size / 4]->score,
                population->individuals[population->size / 2]->score,
                population->individuals[population->size * 3 / 4]->score,
                population->individuals[population->size - 1]->score);
}

int ga_interrupted = 0;

void ga_interrupt(int sig) {
        ga_interrupted = 1;
}

GA_SOLUTION_TYPE* ga_fit(const ga_parameters_t* parameters) {
        signal(SIGINT, ga_interrupt);

        individual_t* best_fit = NULL;
        population_t* population =  ga_generate_random_population(parameters);

        while (!ga_interrupted) {
                ga_evaluate_population(parameters, population);

                const individual_t* current_best_fit = population->individuals[0];

                if (best_fit==NULL || current_best_fit->score < best_fit->score) {
                        if (best_fit!=NULL) {
                                ga_destroy_individual(parameters, best_fit);
                        }
                        best_fit = ga_copy_individual(parameters, current_best_fit);
                }

                ga_print_population(parameters->graph, population);
                population_t* next_population = ga_generate_next_population(parameters, population);
                ga_destroy_population(parameters, population);
                population = next_population;
        }
        ga_destroy_population(parameters, population);

        GA_SOLUTION_TYPE* solution = parameters->copy(parameters->graph, best_fit->solution);
        ga_destroy_individual(parameters, best_fit);

        return solution;
}
