#include "ga.c"

int main(int argc, char** argv) {
        srand(time(NULL));
        graph_t* graph = gra_generate_random_graph(16);
        path_t* path1 = path_generate_simple(graph);
        path_t* path2 = path_generate_simple(graph);
        path_t* crossed = cross_paths_naive(graph, path1, path2);
        path_print(NULL, path1);
        path_print(NULL, path2);
        path_print(NULL, crossed);
}
