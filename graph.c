#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ensemble.c"

#define NODE_DATA_TYPE point_t

typedef double distance_t;

typedef struct {
        distance_t x;
        distance_t y;
} point_t;

typedef struct {
        NODE_DATA_TYPE* metadata;
} node_t;

typedef struct {
        size_t size;
        node_t* nodes;
} graph_t;

typedef struct {
        distance_t distance;
        element_t node;
} neighbor_t;

typedef struct {
        size_t node_count;
        distance_t length;
        neighbor_t* neighbors;
} neighborhood_t;

typedef struct {
        size_t size;
        element_t* node_indices;
        neighborhood_t* neighborhood;
} path_t;

NODE_DATA_TYPE* gra_create_empty_metadata() {
        return calloc(1, sizeof(NODE_DATA_TYPE));
}

point_t* point_of(const distance_t x, const distance_t y) {
        point_t* point = calloc(1, sizeof(point_t));
        point->x = x;
        point->y = y;
        return point;
}

void point_print(const point_t* point) {
        printf("Point %p[x=%f,y=%f]\n", point, point->x, point->y);
}

point_t* point_generate_random() {
        return point_of(rand() % 5000, rand() % 5000);
}

void point_destroy(point_t* point) {
        free(point);
}

void gra_destroy_metadata(NODE_DATA_TYPE* metadata) {
        free(metadata);
}

graph_t* gra_create(const size_t size) {
        graph_t* graph = calloc(1, sizeof(graph_t));
        graph->size = size;
        graph->nodes = calloc(size, sizeof(NODE_DATA_TYPE*));
        return graph;
}

graph_t* gra_of(const size_t node_count, ...) {
        va_list valist;
        graph_t* graph = gra_create(node_count);

        va_start(valist, node_count);
        for (size_t i = 0; i<node_count; i++) {
                NODE_DATA_TYPE* current_node = va_arg(valist, NODE_DATA_TYPE*);
                graph->nodes[i].metadata = current_node;
        }
        va_end(valist);

        return graph;
}

graph_t* gra_generate_random_graph(const size_t size) {
        graph_t* graph = calloc(1, sizeof(graph_t));
        graph->size = size;
        graph->nodes = calloc(size, sizeof(NODE_DATA_TYPE*));
        for (size_t i=0 ; i<size ; i++) {
                graph->nodes[i].metadata = point_generate_random();
        }
        return graph;
}

void gra_destroy_graph(graph_t* graph) {
        for (size_t i=0 ; i<graph->size ; i++) {
                gra_destroy_metadata(graph->nodes[i].metadata);
        }
        free(graph->nodes);
        free(graph);
}

distance_t gra_distance_between_nodes(const graph_t* graph, const element_t node1, const element_t node2) {
        const distance_t dx = graph->nodes[node1].metadata->x - graph->nodes[node2].metadata->x;
        const distance_t dy = graph->nodes[node1].metadata->y - graph->nodes[node2].metadata->y;
        return sqrt(dx*dx + dy*dy);
}

path_t* path_generate_empty(const size_t size) {
        path_t* path = calloc(1, sizeof(path_t));
        path->size = size;
        path->node_indices = calloc(size, sizeof(element_t));
        return path;
}

void path_print(const graph_t* graph, path_t* const path) {
        printf("Path %p[size=%zu,nodes=", path, path->size);
        for (size_t i=0 ; i<path->size ; i++) {
                if (i != 0) {
                        printf("-");
                }
                printf("%u", *(path->node_indices + i));
        }
        printf("]\n");
}

element_t path_node_at(const path_t* path, const size_t index) {
        return *(path->node_indices + index);
}

element_t path_next(const path_t* path, const size_t index) {
        if (index >= path->size - 1) {
                return path_node_at(path, (index + 1) % path->size);
        }
        return path_node_at(path, index + 1);
}

element_t path_previous(const path_t* path, const size_t index) {
        if (index <= 0) {
                return path_node_at(path, (path->size - 1) % path->size);
        }
        return path_node_at(path, index - 1);
}

neighborhood_t* neighborhood_from_path(const graph_t* graph, const path_t* path) {
        neighborhood_t* neighborhood = calloc(1, sizeof(neighborhood_t));
        neighborhood->node_count = path->size;
        neighborhood->neighbors = calloc(path->size * 2, sizeof(neighbor_t));

        element_t previous = path_previous(path, 0);
        for (size_t i = 0 ; i<path->size ; i++) {
                element_t current = path_node_at(path, i);
                distance_t distance = gra_distance_between_nodes(graph, previous, current);

                neighborhood->neighbors[2 * current + 1].distance = distance;
                neighborhood->neighbors[2 * current + 1].node = previous;

                neighborhood->neighbors[2 * previous].distance = distance;
                neighborhood->neighbors[2 * previous].node = current;

                neighborhood->length += distance;

                previous = current;
        }

        return neighborhood;
}

neighborhood_t* neighborhood_copy(const neighborhood_t* neighborhood) {
        neighborhood_t* copy = calloc(1, sizeof(neighborhood_t));
        copy->node_count = neighborhood->node_count;
        copy->neighbors = calloc(neighborhood->node_count * 2, sizeof(neighborhood_t));
        memcpy(copy->neighbors, neighborhood->neighbors, neighborhood->node_count * 2 * sizeof(element_t));
        return copy;
}

const neighbor_t* neighborhood_neighbors(const neighborhood_t* neighborhood, element_t node, size_t index) {
        return neighborhood->neighbors + (2 * node + index);
}

void neighborhood_destroy(neighborhood_t* neighborhood) {
        free(neighborhood->neighbors);
        free(neighborhood);
}

path_t* path_copy(const path_t* path) {
        path_t* copy = calloc(1, sizeof(path_t));
        copy->size = path->size;
        copy->node_indices = calloc(copy->size, sizeof(element_t));
        memcpy(copy->node_indices, path->node_indices, path->size * sizeof(element_t));
        if (path->neighborhood != NULL) {
                copy->neighborhood = neighborhood_copy(path->neighborhood);
        }
        return copy;
}

path_t* path_extract_path_with_nodes_not_in_ensemble(const path_t* path, const ensemble_t* ensemble, const size_t from, const size_t count) {
        path_t* copy = calloc(1, sizeof(path_t));
        copy->size = 0;
        copy->node_indices = calloc(count, sizeof(element_t));
        for (size_t i=0 ; i<path->size && copy->size < count ; i++) {
                const element_t current = *(path->node_indices + (i + from) % path->size);
                if (!ens_contains(ensemble, current)) {
                        *(copy->node_indices + copy->size) = current;
                        copy->size++;
                }
        }
        return copy;
}

path_t* path_of(const size_t node_count, ...) {
        path_t* path = calloc(1, sizeof(path_t));
        path->size = node_count;
        path->node_indices = calloc(node_count, sizeof(element_t));
        va_list valist;
        va_start(valist, node_count);
        for (size_t i = 0; i<node_count; i++) {
                element_t current_node = va_arg(valist, element_t);
                *(path->node_indices + i) = current_node;
        }
        va_end(valist);
        return path;
}

path_t* path_concat(const size_t path_count, ...) {
        va_list valist;
        va_start(valist, path_count);
        size_t length = 0;
        for (size_t i = 0; i<path_count; i++) {
                path_t* current_path = va_arg(valist, path_t*);
                length += current_path->size;
        }
        va_end(valist);

        path_t* copy = calloc(1, sizeof(path_t));
        copy->size = length;
        copy->node_indices = calloc(length, sizeof(element_t));

        va_start(valist, path_count);
        size_t current_index = 0;
        for (size_t i = 0; i<path_count; i++) {
                path_t* current_path = va_arg(valist, path_t*);
                memcpy(copy->node_indices + current_index, current_path->node_indices, current_path->size * sizeof(element_t));
                current_index += current_path->size;
        }
        va_end(valist);

        return copy;
}

path_t* path_generate_simple(const graph_t* graph) {
        path_t* path = path_generate_empty(graph->size);
        for (element_t i=0 ; i<path->size ; i++) {
                path->node_indices[i] = i;
        }

        return path;
}

void path_swap_nodes(path_t* path, const size_t index1, const size_t index2) {
        const element_t t = path->node_indices[index1];
        path->node_indices[index1] = path->node_indices[index2];
        path->node_indices[index2] = t;
}

void path_shift(path_t* path, const size_t shift) {
        element_t* buffer = calloc(path->size, sizeof(element_t));
        memcpy(buffer, path->node_indices, path->size * sizeof(element_t));
        memcpy(path->node_indices, buffer + shift, (path->size - shift) * sizeof(element_t));
        memcpy(path->node_indices + path->size - shift, buffer, shift * sizeof(element_t));
        free(buffer);
}

size_t path_node_position(const path_t* path, const element_t node) {
        for (size_t i=0 ; i<path->size ; i++) {
                if (*(path->node_indices + i) == node) {
                        return i;
                }
        }
        return -1;
}

void path_set_starting_node(path_t* path, const element_t start_node) {
        const size_t first_element_index = path_node_position(path, start_node);
        if (first_element_index > 0) {
                path_shift(path, first_element_index);
        }
}

void path_randomize(const graph_t* graph, path_t* path) {
        for (size_t i=0 ; i<path->size - 1 ; i++) {
                size_t j = i + rand() / (RAND_MAX / (path->size - i) + 1);
                path_swap_nodes(path, i, j);
        }
}

void path_revert_from_to(path_t* path,  size_t from, size_t to) {
        if (from == to) {
                return;
        }
        if (from > to) {
                size_t i = from;
                from = to;
                to = i;
        }
        assert(from < to);
        assert(to > 0);
        assert(from < path->size);
        to--;
        for (; from<to ; from++, to--) {
                path_swap_nodes(path, from, to);
        }
}

void path_revert_from(path_t* path, const size_t from) {
        path_revert_from_to(path, from, path->size);
}

void path_revert(path_t* path) {
        path_revert_from(path, 0);
}

int path_cmp(const path_t* path1, const path_t* path2) {
        if (path1->size != path2->size) {
                return path1->size - path2->size ? -1 : 1;
        }
        return memcmp(path1->node_indices, path2->node_indices, path1->size * sizeof(element_t));
}

element_t path_first_node_not_in_ensemble(const path_t* path, const ensemble_t* ensemble) {
        for (size_t i=0; i < path->size ; i++) {
                element_t current = *(path->node_indices + i);
                if (!ens_contains(ensemble, current)) {
                        return current;
                }
        }
        return -1;
}

element_t path_first_node_not_in_ensemble_from_index(const path_t* path, const size_t from_index, const ensemble_t* ensemble) {
        for (size_t i=0; i < path->size ; i++) {
                const size_t index = (i + from_index) % path->size;
                const element_t current = *(path->node_indices + index);
                if (!ens_contains(ensemble, current)) {
                        return current;
                }
        }
        return -1;
}

distance_t path_length(const graph_t* graph, const path_t* path) {
        if (path->neighborhood == NULL) {
                neighborhood_t* neighborhood = neighborhood_from_path(graph, path);
                distance_t length = neighborhood->length;
                neighborhood_destroy(neighborhood);
                return length;
        }
        return path->neighborhood->length;
}

void path_destroy(path_t* path) {
        if (path->neighborhood != NULL) {
                neighborhood_destroy(path->neighborhood);
        }
        free(path->node_indices);
        free(path);
}

int path_2_opt_iteration(const graph_t* graph, path_t* path, const size_t starting_node, const size_t from, const size_t to) {
        distance_t improvement = 0;
        element_t improvement_node = 0;
        for (size_t j=from ; j<to ; j++) {
                if (j + 1 < starting_node || j > starting_node + 1) {
                        const element_t xi = path_node_at(path, starting_node);
                        const element_t xi1 = path_next(path, starting_node);
                        const element_t xj = path_node_at(path, j);
                        const element_t xj1 = path_next(path, j);
                        const distance_t current_improvement =
                                gra_distance_between_nodes(graph, xi, xi1)
                                + gra_distance_between_nodes(graph, xj, xj1)
                                - gra_distance_between_nodes(graph, xi, xj)
                                - gra_distance_between_nodes(graph, xi1, xj1);
                        if (current_improvement > improvement) {
                                improvement = current_improvement;
                                improvement_node = j;
                        }
                }
        }
        if (improvement > 0) {
                path_revert_from_to(path, starting_node + 1, improvement_node + 1);
                return 1;
        }
        return 0;
}

int path_2_opt_from_to(const graph_t* graph, path_t* path, const size_t from, const size_t to) {
        for (size_t i=from ; i<to ; i++) {
                int is_better = path_2_opt_iteration(graph, path, i, from, path->size);
                if (is_better) {
                        return 1;
                }
        }
        return 0;
}

void path_2_opt(const graph_t* graph, path_t* path) {
        path_2_opt_from_to(graph, path, 0, path->size);
}

void path_2_opt_iterate(const graph_t* graph, path_t* path, const size_t from, const size_t to) {
        while (path_2_opt_from_to(graph, path, from, to)) {}
}

graph_t* gra_read(const char* filename) {
        FILE *file = fopen(filename, "r");
        if (file == NULL) {
                printf("Error reading file!\n");
                exit(1);
        }
        size_t size;
        double x;
        double y;
        fscanf(file, "%zu", &size);
        graph_t* graph = gra_create(size);
        for (size_t i=0 ; i<size ; i++) {
                if (fscanf(file, "%lf,%lf", &x, &y) == 0) {
                        printf("Error reading graph!\n");
                        exit(1);
                }
                graph->nodes[i].metadata = point_of(x, y);
        }
        fclose(file);
        return graph;
}

void path_save(const char* filename, const graph_t* graph, const path_t* path) {
        FILE *file = fopen(filename, "w");
        if (file == NULL) {
                printf("Error opening file!\n");
                exit(1);
        }
        for (size_t i=0 ; i<path->size ; i++) {
                const size_t current_node = path->node_indices[i];
                fprintf(file, "%zu,%f,%f\n", current_node, graph->nodes[current_node].metadata->x, graph->nodes[current_node].metadata->y);
        }
        fclose(file);
}
