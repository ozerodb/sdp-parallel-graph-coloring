#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "util.h"

typedef struct edge {
  int from;
  int to;
} Edge;

typedef struct graph *Graph;

unsigned int GRAPH_get_edge_count(Graph G);
unsigned int GRAPH_get_vertex_count(Graph G);
Graph GRAPH_load_from_file(char *filename);
Graph GRAPH_init(unsigned int V);
void GRAPH_free(Graph G);
void GRAPH_ladj_print(Graph G);
void GRAPH_ladj_print_with_colors(Graph G, unsigned int *colors);
unsigned int *GRAPH_color(Graph G, char *coloring_method_str, unsigned int n_threads);
unsigned int *GRAPH_get_degrees(Graph G);
unsigned int GRAPH_check_given_coloring_validity(Graph G, unsigned int *colors);
unsigned int GRAPH_check_current_coloring_validity(Graph G);
unsigned long GRAPH_compute_bytes(Graph G);

#endif