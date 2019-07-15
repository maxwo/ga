#include <assert.h>

#include "graph.c"

void testPathShift(size_t size) {
        graph_t* graph = gra_generate_random_graph(size);
        path_t* path = path_generate_simple(graph);
        path_print(graph, path);
        path_shift(path, 10);
        path_print(graph, path);
        path_destroy(path);
        gra_destroy_graph(graph);
}

void testPathRevert(size_t size) {
        graph_t* graph = gra_generate_random_graph(size);
        path_t* path = path_generate_simple(graph);
        path_print(graph, path);
        path_revert(path);
        path_print(graph, path);
        path_destroy(path);
        gra_destroy_graph(graph);
}

void testNeighborhood(const graph_t* graph) {
        path_t* path1 = path_of(4, 0, 1, 2, 3);
        path_t* path2 = path_of(4, 0, 2, 3, 1);
        path_print(NULL, path1);
        path_print(NULL, path2);

        neighborhood_t* neighborhood = neighborhood_from_path(graph, path1);
        for (size_t i=0 ; i<path1->size ; i++) {
                printf("- Node %zu neighbors:\n", i);
                for (size_t j=0 ; j<2 ; j++) {
                        printf("  - %u by %f\n", neighborhood_neighbors(neighborhood, i, j)->node, neighborhood_neighbors(neighborhood, i, j)->distance);
                }
        }
}

void test_path_length() {
        graph_t* graph_length_8 = gra_of(8,
                point_of(0, 0),
                point_of(0, 1),
                point_of(0, 2),
                point_of(1, 2),
                point_of(2, 2),
                point_of(2, 1),
                point_of(2, 0),
                point_of(1, 0)
        );

        path_t* path_length_8 = path_generate_simple(graph_length_8);
        path_print(graph_length_8, path_length_8);
        distance_t length = path_length(graph_length_8, path_length_8);

        printf("Actual length: %f\n", length);

        assert(length == 8);

        gra_destroy_graph(graph_length_8);
}

void test_path_2_opt() {
        graph_t* graph = gra_read("graph_test.csv");
        path_t* path = path_generate_simple(graph);
        path_t* path_test = path_generate_simple(graph);

        path_revert_from_to(path_test, 3, 6);

        path_print(graph, path);
        printf("length: %lf\n", path_length(graph, path));

        path_print(graph, path_test);
        printf("length: %lf\n", path_length(graph, path_test));

        path_2_opt_iterate(graph, path_test, 0, path_test->size);

        path_print(graph, path_test);
        printf("length: %lf\n", path_length(graph, path_test));

        assert(path_cmp(path, path_test) == 0);

        path_destroy(path_test);
        path_destroy(path);
        gra_destroy_graph(graph);
}

int main(int argc, char** argv) {
        graph_t* graph_length_8 = gra_of(8,
                point_of(0, 0),
                point_of(0, 1),
                point_of(0, 2),
                point_of(1, 2),
                point_of(2, 2),
                point_of(2, 1),
                point_of(2, 0),
                point_of(1, 0)
        );

        test_path_2_opt();
        test_path_length();
        testPathShift(16);
        testPathRevert(16);
        testNeighborhood(graph_length_8);
}
