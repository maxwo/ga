#include "ga.c"

path_t* tsp_generate_random_path(const graph_t* graph) {
        path_t* path = path_generate_simple(graph);
        path_randomize(graph, path);
        return path;
}

path_t* tsp_generate_greedy_path(const graph_t* graph) {
        ensemble_t* visited_nodes = ens_create(graph->size);
        path_t* path = path_generate_empty(graph->size);

        element_t current = random_probability() * graph->size;
        path->node_indices[0] = current;
        ens_add_element(visited_nodes, current);
        for (size_t i=1 ; i<graph->size ; i++) {
                element_t closest_node = -1;
                distance_t closest_distance = -1;

                for (size_t j=0 ; j<graph->size ; j++) {
                        if (!ens_contains(visited_nodes, j)) {
                                const distance_t current_distance = gra_distance_between_nodes(graph, current, j);
                                if (closest_distance == -1 || current_distance < closest_distance) {
                                        closest_node = j;
                                        closest_distance = current_distance;
                                }
                        }
                }

                current = closest_node;
                ens_add_element(visited_nodes, current);
                path->node_indices[i] = current;
        }

        ens_destroy(visited_nodes);

        return path;
}

void tsp_regularize_path(const graph_t* graph, path_t* solution) {
        path_set_starting_node(solution, 0);
        if (path_previous(solution, 0) < path_next(solution, 0)) {
                path_revert_from(solution, 1);
        }
        solution->neighborhood = neighborhood_from_path(graph, solution);
}

int tsp_compare_solutions(const graph_t* graph, const path_t* path1, const path_t* path2) {
        return path_cmp(path1, path2);
}

path_t* tsp_copy_path(const graph_t* graph, const path_t* path) {
        return path_copy(path);
}

score_t tsp_score(const graph_t* graph, const path_t* path) {
        return path_length(graph, path);
}

element_t tsp_node_from_neighbors(const ponderation_t* ponderation, const element_t node, const path_t* path1, const path_t* path2) {
        size_t chosen_one = pond_random(ponderation);
        if (chosen_one < 2) {
                return neighborhood_neighbors(path1->neighborhood, node, chosen_one)->node;
        } else {
                return neighborhood_neighbors(path2->neighborhood, node, chosen_one - 2)->node;
        }
}

element_t tsp_first_unvisited_node(const ensemble_t* ensemble, const element_t from) {
        element_t node = from;
        while (1) {
                if (!ens_contains(ensemble, node)) {
                        return node;
                }
                node++;
        }
        return -1;
}

void tsp_ponderation_from_neighborhoods(ponderation_t* ponderation, const ensemble_t* ensemble, const element_t node, const path_t* path1, const path_t* path2) {
        ponderation_reset(ponderation);

        for (size_t i = 0 ; i<2 ; i++) {
                const neighbor_t* neighbor = neighborhood_neighbors(path1->neighborhood, node, i);
                if (ens_contains(ensemble, neighbor->node)) {
                        pond_set_probability(ponderation, i, 0);
                } else {
                        pond_set_probability(ponderation, i, 1 / neighbor->distance);
                }
        }

        for (size_t i = 0 ; i<2 ; i++) {
                const neighbor_t* neighbor = neighborhood_neighbors(path2->neighborhood, node, i);
                if (ens_contains(ensemble, neighbor->node)) {
                        pond_set_probability(ponderation, i + 2, 0);
                } else {
                        pond_set_probability(ponderation, i + 2, 1 / neighbor->distance);
                }
        }
}

#define TWO_OPT_ITERATIONS 100

void tsp_path_mutate_2_opt(const graph_t* graph, path_t* path) {
        const size_t starting_node = rand() % (path->size);
        for (size_t i=0 ; i<TWO_OPT_ITERATIONS ; i++) {
                path_2_opt_from_to(graph, path, starting_node, path->size);
        }
}


path_t* tsp_cross_paths_neighbors(const graph_t* graph, const path_t* path1, const path_t* path2) {
        path_t* crossed = path_generate_empty(graph->size);

        ponderation_t* ponderation = pond_create(4);
        ensemble_t* ensemble = ens_create(graph->size);

        element_t unvisited_from = 0;
        element_t current = random_probability() * graph->size;

        *(crossed->node_indices) = current;
        ens_add_element(ensemble, current);

        for (size_t i = 1 ; i<crossed->size ; i++) {
                tsp_ponderation_from_neighborhoods(ponderation, ensemble, current, path1, path2);

                if (ponderation->sum==0) {
                        current = tsp_first_unvisited_node(ensemble, unvisited_from);
                        unvisited_from = current;
                } else {
                        current = tsp_node_from_neighbors(ponderation, current, path1, path2);
                }

                *(crossed->node_indices + i) = current;
                ens_add_element(ensemble, current);

        }

        ens_destroy(ensemble);
        pond_destroy(ponderation);

        tsp_path_mutate_2_opt(graph, crossed);

        return crossed;
}

path_t* tsp_cross_paths_naive_cut(const graph_t* graph, const path_t* path1, const path_t* path2) {
        const size_t cut1 = rand() % (path1->size - 2);
        const size_t cut2 = cut1 + 1 + rand() % (path1->size - cut1);

        ensemble_t* ensemble = ens_create(graph->size);

        path_t* part1 = path_extract_path_with_nodes_not_in_ensemble(path1, ensemble, 0, cut1);
        ens_add_elements(ensemble, part1->node_indices, part1->size);

        path_t* part2 = path_extract_path_with_nodes_not_in_ensemble(path2, ensemble, cut1, cut2 - cut1);
        ens_add_elements(ensemble, part2->node_indices, part2->size);

        path_t* part3 = path_extract_path_with_nodes_not_in_ensemble(path1, ensemble, cut2, path1->size - cut2);

        path_t* crossed = path_concat(3, part1, part2, part3);

        path_destroy(part1);
        path_destroy(part2);
        path_destroy(part3);

        ens_destroy(ensemble);

        return crossed;
}

void tsp_path_mutate_random_swap(const graph_t* graph, path_t* solution){
        const size_t swap1 = rand() % (solution->size);
        const size_t swap2 = rand() % (solution->size);

        path_swap_nodes(solution, swap1, swap2);
}

int main(int argc, char** argv) {
        srand(time(NULL));

        // graph_t* graph = gra_read("cities_ready.csv");
        graph_t* graph = gra_generate_random_graph(1024);

        ga_parameters_t parameters;
        parameters.graph = graph;
        parameters.elitism = 1;
        parameters.mutation_rate = 0.02;
        parameters.survival_rate = 0.2;
        parameters.population_size = 200;

        parameters.generate = tsp_generate_random_path;
        parameters.regularize = tsp_regularize_path;
        parameters.compare = tsp_compare_solutions;
        parameters.evaluate = tsp_score;
        parameters.copy = tsp_copy_path;
        parameters.cross = tsp_cross_paths_neighbors;
        parameters.mutate = tsp_path_mutate_2_opt;
        parameters.destroy = path_destroy;

        const path_t* solution = ga_fit(&parameters);

        path_save("solution.txt", graph, solution);

        gra_destroy_graph(graph);
}
