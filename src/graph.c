#include "graph.h"

#define N_COLORING_METHODS 6
typedef enum {
  seq_greedy,
  seq_ldf,
  par_jp,
  par_ldf,
  par_ldf_plus
} coloring_method;
const char *coloring_methods[N_COLORING_METHODS] = {
    "seq_greedy", "seq_ldf", "par_jp", "par_ldf", "par_ldf_plus"};

typedef struct node *link;

struct node {
  int index;
  link next;
};

struct graph {
  unsigned int V, E;
  link *ladj;
  link z;
  unsigned int *degree;
  unsigned int *color;
};

typedef struct param_struct {
  Graph G;
  unsigned int index;
  unsigned int *weights;
  unsigned int *vertexes;
  pthread_mutex_t *lock;
  unsigned int n_threads;
} param_t;

/* UTILITY FUNCTIONS */

static coloring_method method_str_to_enum(char *coloring_method_str) {
  for (unsigned int i = 0; i < N_COLORING_METHODS; i++) {
    if (!strcmp(coloring_method_str, coloring_methods[i])) {
      return i;
    }
  }
  return -1;
}

static link LINK_new(int index, link next) {
  link x = malloc(sizeof *x);
  if (x == NULL) {
    fprintf(stderr, "Error while allocating a link\n");
    return NULL;
  }
  x->index = index;
  x->next = next;
  return x;
}

static link EDGE_insert(Graph G, int from, int to) {
  link new = LINK_new(to, G->ladj[from]);
  if (new == NULL) {
    fprintf(stderr, "Error while inserting an edge\n");
    return NULL;
  }
  G->ladj[from] = new;
  G->degree[from]++;
  G->E++;
  return G->ladj[from];
}

/* COLORING ALGORITHMS */

/* SEQUENTIAL GREEDY */
unsigned int *color_sequential_greedy(Graph G) {
  unsigned int n = G->V;
  unsigned int *random_order = malloc(n * sizeof(unsigned int));
  if (random_order == NULL) {
    printf("Error allocating random order array!\n");
    return NULL;
  }
  for (unsigned int i = 0; i < n; i++) {
    G->color[i] = 0;
    random_order[i] = i;
  }
  UTIL_randomize_array(random_order, n);
  for (int i = 0; i < n; i++) {
    unsigned int ii = random_order[i];

    int *neighbours_colors = malloc(G->degree[ii] * sizeof(int));
    unsigned int j = 0;
    for (link t = G->ladj[ii]; t != G->z; t = t->next) {
      neighbours_colors[j++] = G->color[t->index];
    }

    G->color[ii] = UTIL_smallest_missing_number(neighbours_colors, G->degree[ii]);
    free(neighbours_colors);
  }
  free(random_order);
  return G->color;
}

/* SEQUENTIAL LDF */
unsigned int *color_sequential_ldf(Graph G) {
  unsigned int n = G->V;
  unsigned int *degree = malloc(n * sizeof(unsigned int));
  if (degree == NULL) {
    printf("Error allocating degrees array!\n");
    return NULL;
  }
  unsigned int *vertex = malloc(n * sizeof(unsigned int));
  if (vertex == NULL) {
    printf("Error allocating vertex array!\n");
    return NULL;
  }
  for (unsigned int i = 0; i < n; i++) {
    G->color[i] = 0;
    degree[i] = G->degree[i];
    vertex[i] = i;
  }
  UTIL_heapsort_values_by_keys(n, degree, vertex);
  for (int i = n - 1; i >= 0; i--) {
    unsigned int ii = vertex[i];  // heap sort will sorts degrees in ascending
                                  // order, so we access them backwards
    int *neighbours_colors = malloc(degree[i] * sizeof(int));
    if (neighbours_colors == NULL) {
      printf("Error allocating neighbours_colors array!\n");
    }
    unsigned int j = 0;
    for (link t = G->ladj[ii]; t != G->z; t = t->next) {
      neighbours_colors[j++] = G->color[t->index];
    }

    G->color[ii] = UTIL_smallest_missing_number(neighbours_colors, degree[i]);
    free(neighbours_colors);
  }
  free(vertex);
  free(degree);
  return G->color;
}

/* PARALLEL JP*/
void jp_color_vertex(Graph G, unsigned int index, unsigned int n_threads,
                     unsigned int *weights) {
  unsigned int n = G->V;
  int uncolored = n / n_threads;
  if (n % n_threads && (index < n % n_threads)) {
    uncolored++;  // this is needed to manage graphs with a vertex count that is
                  // not a multiple of the number of threads
  }
  while (uncolored > 0) {
    for (int i = index; i < n; i += n_threads) {
      if (G->color[i] == 0) {
        int *neighbours_colors = malloc(G->degree[i] * sizeof(int));
        unsigned int has_highest_number = 1;
        unsigned int j = 0;
        for (link t = G->ladj[i]; t != G->z; t = t->next) {
          if (G->color[t->index] == 0 &&
              (weights[t->index] > weights[i] ||
               (weights[t->index] == weights[i] && t->index > i))) {
            has_highest_number = 0;
            break;
          } else {
            neighbours_colors[j++] = G->color[t->index];
          }
        }

        if (has_highest_number) {
          G->color[i] =
              UTIL_smallest_missing_number(neighbours_colors, G->degree[i]);
          uncolored--;
        }
        free(neighbours_colors);
      }
    }
  }
}

void jp_color_vertex_wrapper(void *par) {
  param_t *tD = (param_t *)par;
  unsigned int index = tD->index; // read the index
  pthread_mutex_unlock(tD->lock); // then unlock the mutex
  jp_color_vertex(tD->G, index, tD->n_threads, tD->weights);
}

unsigned int *color_parallel_jp(Graph G, unsigned int n_threads) {
  unsigned int n = G->V;
  unsigned int *weights = malloc(n * sizeof(unsigned int));
  if (weights == NULL) {
    printf("Error allocating weights array!\n");
    return NULL;
  }
  for (unsigned int i = 0; i < n; i++) {
    G->color[i] = 0;
    weights[i] = rand();
  }
  pthread_mutex_t mutex;

  param_t *par = malloc(sizeof(param_t));
  par->G = G;
  par->n_threads = n_threads;
  par->weights = weights;
  par->lock = &mutex;
  pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
  /* since we use a single param struct for all threads, we need a mutex to
   to make sure the value of par->index is NOT modified until the thread has
   read it
  */
  pthread_mutex_init(&mutex, NULL);

  for (unsigned int i = 0; i < n_threads; i++) {
    pthread_mutex_lock(&mutex); // lock the mutex or wait until it is unlocked
    par->index = i; // assign i
    pthread_create(&threads[i], NULL, (void *)&jp_color_vertex_wrapper,
                   (void *)par);
  }

  for (int i = 0; i < n_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  free(par);
  free(weights);
  free(threads);
  pthread_mutex_destroy(&mutex);
  return G->color;
}

/* PARALLEL LDF */
void ldf_color_vertex(Graph G, unsigned int index, unsigned int n_threads,
                      unsigned int *weights) {
  unsigned int n = G->V;
  int uncolored = n / n_threads;
  if (n % n_threads && (index < n % n_threads)) {
    uncolored++;
  }
  while (uncolored > 0) {
    for (int i = index; i < n; i += n_threads) {
      if (G->color[i] == 0) {
        int *neighbours_colors = malloc(G->degree[i] * sizeof(int));
        unsigned int has_highest_number = 1;
        unsigned int j = 0;
        for (link t = G->ladj[i]; t != G->z; t = t->next) {
          if (G->color[t->index] == 0 &&
              (G->degree[t->index] > G->degree[i] ||
               (G->degree[t->index] == G->degree[i] &&
                weights[t->index] > weights[i]) ||
               (G->degree[t->index] == G->degree[i] &&
                weights[t->index] == weights[i] && t->index > i))) {
            has_highest_number = 0;
            break;
          } else {
            neighbours_colors[j++] = G->color[t->index];
          }
        }

        if (has_highest_number) {
          G->color[i] =
              UTIL_smallest_missing_number(neighbours_colors, G->degree[i]);
          uncolored--;
        }
        free(neighbours_colors);
      }
    }
  }
}

void ldf_color_vertex_wrapper(void *par) {
  param_t *tD = (param_t *)par;
  unsigned int index = tD->index;
  pthread_mutex_unlock(tD->lock);
  ldf_color_vertex(tD->G, index, tD->n_threads, tD->weights);
}

unsigned int *color_parallel_ldf(Graph G, unsigned int n_threads) {
  unsigned int n = G->V;
  unsigned int *weights = malloc(n * sizeof(unsigned int));
  if (weights == NULL) {
    printf("Error allocating weights array!\n");
    return NULL;
  }
  for (unsigned int i = 0; i < n; i++) {
    G->color[i] = 0;
    weights[i] = rand();
  }
  pthread_mutex_t mutex;

  param_t *par = malloc(sizeof(param_t));
  par->G = G;
  par->n_threads = n_threads;
  par->weights = weights;
  par->lock = &mutex;
  pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
  pthread_mutex_init(&mutex, NULL);

  for (unsigned int i = 0; i < n_threads; i++) {
    pthread_mutex_lock(&mutex);
    par->index = i;
    pthread_create(&threads[i], NULL, (void *)&ldf_color_vertex_wrapper,
                   (void *)par);
  }

  for (int i = 0; i < n_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  free(par);
  free(weights);
  free(threads);
  pthread_mutex_destroy(&mutex);
  return G->color;
}

/* PARALLEL LDF PLUS */
void ldf_plus_color_vertex(Graph G, unsigned int index, unsigned int n_threads,
                           unsigned int sorted_vertices[]) {
  unsigned int n = G->V;
  for (int i = n - 1 - index; i >= 0; i -= n_threads) {
    unsigned int ii = sorted_vertices[i];

    int *neighbours_colors = malloc(G->degree[ii] * sizeof(int));
    unsigned int j = 0;
    for (link t = G->ladj[ii]; t != G->z; t = t->next) {
      while (G->color[t->index] == 0 &&
             (G->degree[t->index] > G->degree[ii] ||
              (G->degree[t->index] == G->degree[ii] && t->index > ii))) {
        // wait
      }

      neighbours_colors[j++] = G->color[t->index];
    }
    G->color[ii] = UTIL_smallest_missing_number(neighbours_colors, G->degree[ii]);

    free(neighbours_colors);
  }
}

void ldf_plus_color_vertex_wrapper(void *par) {
  param_t *tD = (param_t *)par;
  unsigned int index = tD->index;
  pthread_mutex_unlock(tD->lock);
  ldf_plus_color_vertex(tD->G, index, tD->n_threads, tD->weights);
}

unsigned int *color_parallel_ldf_plus(Graph G, unsigned int n_threads) {
  unsigned int n = G->V;
  unsigned int *degree = malloc(n * sizeof(unsigned int));
  if (degree == NULL) {
    printf("Error allocating degrees array!\n");
    return NULL;
  }
  unsigned int *vertex = malloc(n * sizeof(unsigned int));
  if (vertex == NULL) {
    printf("Error allocating vertex array!\n");
    return NULL;
  }
  for (unsigned int i = 0; i < n; i++) {
    G->color[i] = 0;
    degree[i] = G->degree[i];
    vertex[i] = i;
  }
  UTIL_stable_qsort_values_by_keys(degree, vertex, n);

  free(degree);
  pthread_mutex_t mutex;

  param_t *par = malloc(sizeof(param_t));
  par->G = G;
  par->n_threads = n_threads;
  par->weights = vertex;
  par->lock = &mutex;
  pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
  pthread_mutex_init(&mutex, NULL);

  for (unsigned int i = 0; i < n_threads; i++) {
    pthread_mutex_lock(&mutex);
    par->index = i;
    pthread_create(&threads[i], NULL, (void *)&ldf_plus_color_vertex_wrapper,
                   (void *)par);
  }

  for (int i = 0; i < n_threads; i++) {
    pthread_join(threads[i], NULL);
  }
  free(par);
  free(vertex);
  free(threads);
  pthread_mutex_destroy(&mutex);
  return G->color;
}

/* EXPOSED FUNCTIONS */

unsigned int GRAPH_check_given_coloring_validity(Graph G,
                                                 unsigned int *colors) {
  for (unsigned int i = 0; i < G->V; i++) {
    for (link t = G->ladj[i]; t != G->z; t = t->next) {
      if (colors[i] == colors[t->index] || colors[i] == 0) {
        return 0;
      }
    }
  }
  return 1;
}

unsigned int GRAPH_check_current_coloring_validity(Graph G) {
  for (unsigned int i = 0; i < G->V; i++) {
    for (link t = G->ladj[i]; t != G->z; t = t->next) {
      if (G->color[i] == G->color[t->index] || G->color[i] == 0) {
        return 0;
      }
    }
  }
  return 1;
}

unsigned int GRAPH_get_edge_count(Graph G) { return G->E; }

unsigned int GRAPH_get_vertex_count(Graph G) { return G->V; }

Graph GRAPH_load_from_file(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Error opening file %s\n", filename);
    return NULL;
  }
  char *dot = strrchr(filename, '.');

  if (!strcmp(dot, ".graph")) {
    unsigned int V, E, fmt = 0, ncon = 0;
    char line[4096], *p, *e;
    fgets(line, sizeof(line), fp);
    sscanf(line, "%d %d %d %d", &V, &E, &fmt, &ncon);

    Graph G = GRAPH_init(V);

    if (G == NULL) {
      fclose(fp);
      return NULL;
    }

    unsigned int from = 1;

    while (fgets(line, sizeof(line), fp)) {
      if (line[0] == '%') {
        continue;
      }
      p = line;
      unsigned int to;
      int alternate = 0;
      if (fmt == 100) {
        if (ncon != 0) {
          fmt = 10;
        } else {
          fmt = 0;
        }
      }
      switch (fmt) {
        case 10:
          for (int i = 0; i < ncon; i++, p = e) {
            to = strtol(p, &e, 10);
          }
        case 0:
          for (;; p = e) {
            to = strtol(p, &e, 10);
            if (p == e) break;
            if (from != to) {
              if (EDGE_insert(G, from - 1, to - 1) == NULL) {
                printf("Couldn't insert edge from %d to %d\n", from, to);
                fclose(fp);
                GRAPH_free(G);
                return NULL;
              }
            }
          }
          break;
        case 11:
          to = strtol(p, &e, 10);
          p = e;
        case 1:
          for (;; p = e) {
            to = strtol(p, &e, 10);
            if (p == e) break;
            if (alternate % 2 == 0 && from != to) {
              if (EDGE_insert(G, from - 1, to - 1) == NULL) {
                printf("Couldn't insert edge from %d to %d\n", from, to);
                fclose(fp);
                GRAPH_free(G);
                return NULL;
              }
            }
            alternate++;
          }
          break;
        default:
          printf("Invalid fmt\n");
          fclose(fp);
          GRAPH_free(G);
          return NULL;
      }
      from++;
    }
    fclose(fp);
    return G;
  } else if (!strcmp(dot, ".gra")) {
    int V = -1;
    char buf[64];
    while (V <= 0) {
      fscanf(fp, "%s", buf);
      if (atoi(buf) > 0) {
        V = atoi(buf);
        // printf("V = %d\n", V);
      } else {
        // printf("Skipped line: %s\n", buf);
      }
    }

    Graph G = GRAPH_init(V);
    if (G == NULL) {
      fclose(fp);
      return NULL;
    }
    for (int i = 0; i < V; i++) {
      fscanf(fp, "%*d:");
      fscanf(fp, " %s ", buf);
      while (strcmp(buf, "#")) {
        int to = atoi(buf);
        if (EDGE_insert(G, i, to) == NULL) {
          printf("Couldn't insert edge from %d to %d\n", i, to);
          fclose(fp);
          GRAPH_free(G);
          return NULL;
        }
        if (EDGE_insert(G, to, i) == NULL) {
          printf("Couldn't insert edge from %d to %d\n", to, i);
          fclose(fp);
          GRAPH_free(G);
          return NULL;
        }
        fscanf(fp, "%s", buf);
      }
    }
    fclose(fp);
    return G;
  } else {
    printf("Invalid extension %s\n", dot);
    fclose(fp);
    return NULL;
  }
}

Graph GRAPH_init(unsigned int V) {
  Graph G = malloc(sizeof *G);
  if (G == NULL) {
    fprintf(stderr, "Error while allocating the graph\n");
    return NULL;
  }
  G->V = V;
  G->E = 0;
  G->z = LINK_new(-1, NULL);
  G->ladj = malloc(V * sizeof(link));
  G->degree = malloc(V * sizeof(unsigned int));
  G->color = malloc(V * sizeof(unsigned int));

  for (unsigned int i = 0; i < V; i++) {
    G->ladj[i] = G->z;
    G->degree[i] = 0;
    G->color[i] = 0;
  }
  return G;
}

void GRAPH_free(Graph G) {
  if (G == NULL) {
    return;
  }
  link next;
  for (int i = 0; i < G->V; i++) {
    for (link t = G->ladj[i]; t != G->z; t = next) {
      next = t->next;
      free(t);
    }
  }
  free(G->ladj);
  free(G->degree);
  free(G->color);
  free(G->z);
  free(G);
}

void GRAPH_ladj_print(Graph G) {
  for (int i = 0; i < G->V; i++) {
    printf("%d -->", i + 1);
    for (link t = G->ladj[i]; t != G->z; t = t->next) {
      printf("%c%d", t == G->ladj[i] ? ' ' : '-', (t->index) + 1);
    }
    putchar('\n');
  }
}
void GRAPH_ladj_print_with_colors(Graph G, unsigned int *colors) {
  for (int i = 0; i < G->V; i++) {
    printf("%d(%d) -->", i + 1, colors[i]);
    for (link t = G->ladj[i]; t != G->z; t = t->next) {
      printf("%c%d(%d)", t == G->ladj[i] ? ' ' : '-', (t->index) + 1,
             colors[t->index]);
    }
    putchar('\n');
  }
}

unsigned int *GRAPH_get_degrees(Graph G) { return G->degree; }

unsigned long GRAPH_compute_bytes(Graph G) {
  unsigned long long bytes = 0;
  bytes += sizeof(G);
  bytes += sizeof(*G);
  bytes += (G->V + G->E + 1) * sizeof(link);
  bytes += (G->V + G->E + 1) * sizeof(struct node);
  bytes += (2 * G->V) * sizeof(unsigned int);
  return bytes;
}

unsigned int *GRAPH_color(Graph G, char *coloring_method_str,
                          unsigned int n_threads) {
  switch (method_str_to_enum(coloring_method_str)) {
    case seq_greedy:
      return color_sequential_greedy(G);
      break;
    case seq_ldf:
      return color_sequential_ldf(G);
      break;
    case par_jp:
      return color_parallel_jp(G, n_threads);
      break;
    case par_ldf:
      return color_parallel_ldf(G, n_threads);
      break;
    case par_ldf_plus:
      return color_parallel_ldf_plus(G, n_threads);
      break;

    default:
      fprintf(stderr, "Passed coloring method '%s' is not valid!\n",
              coloring_method_str);
      return NULL;
  }
}