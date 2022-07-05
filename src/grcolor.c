#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>

#include "graph.h"
#include "util.h"

#define N_COLORING_METHODS 5

typedef struct bench_res {
  char *graph_name;
  char *coloring_method;
  unsigned int vertex_count;
  unsigned int colors_used;
  unsigned int n_threads;
  double coloring_time;
} Results;

int main(int argc, char *argv[]) {
  srand(time(NULL));
  char **graphs_filenames = NULL;
  int number_of_graphs = 0;
  int export = 0;
  int par_only = 0;
  int n_threads = get_nprocs();
  int iterations = 1;

  if (argc > 1) {
    graphs_filenames = malloc(
        (argc - 1) * sizeof(char *));  // allocate enough char* for the possible
                                       // graph_filenames passed as arguments
    for (int i = 1; i < argc; i++) {
      /* flag '-t' or '--threads' to specify how many threads to use in parallel
       * algorithms */
      if (!strcmp(argv[i], "--threads") || !strcmp(argv[i], "-t")) {
        if (i + 1 != argc) {
          n_threads = atoi(argv[i + 1]);
          if (n_threads <= 0) {
            printf(
                "Error: '-t|--threads' flag is specified but the number of "
                "threads is invalid! (negative, zero or not numeric)\n");
            return 1;
          }
        } else {
          printf(
              "Error: '-t|--threads' flag is specified without the number of "
              "threads!\n");
          return 1;
        }

        i++;  // Move to the next flag
        continue;
      }

      /* flag '-n' to specify how many times the coloring should be repeated */
      if (!strcmp(argv[i], "-n")) {
        if (i + 1 != argc) {
          iterations = atoi(argv[i + 1]);
          if (iterations <= 0) {
            printf(
                "Error: '-n' flag is specified but the number of "
                "iterations is invalid! (negative, zero or not numeric)\n");
            return 1;
          }
        } else {
          printf(
              "Error: '-n' flag is specified without the number of "
              "iterations!\n");
          return 1;
        }

        i++;  // Move to the next flag
        continue;
      }

      /* flag '--csv' to specify whether or not we want to export results to
       * csv */
      if (!strcmp(argv[i], "--csv")) {
        export = 1;
        continue;
      }

      /* flag '--par' to specify whether or not we want to use parallel
       * algorithms only */
      if (!strcmp(argv[i], "--par")) {
        par_only = 1;
        continue;
      }

      /* if an argument is not a known flag, it's treated as a graph's filename
       */
      graphs_filenames[number_of_graphs++] = argv[i];
    }
  }

  /* if no graphs' filenames were passed as argument */
  if (number_of_graphs == 0) {
    if (graphs_filenames != NULL) {
      free(graphs_filenames);
      graphs_filenames = NULL;
    }

    /* then use the graphs in the /graphs subfolder */
    DIR *d;
    struct dirent *dir;
    d = opendir("graphs");
    if (d) {
      while ((dir = readdir(d)) != NULL) {
        char *dot = strrchr(dir->d_name, '.');  // search for the last dot .
        if (dot && (!strcmp(dot, ".graph") || !strcmp(dot, ".gra"))) {
          /* if the extension is .graph or .gra, add to graphs to be colored */
          graphs_filenames = realloc(
              graphs_filenames,
              (number_of_graphs + 1) *
                  sizeof(char *));  // realloc works as malloc if first init
          graphs_filenames[number_of_graphs] =
              malloc((strlen(dir->d_name) + 8) * sizeof(char));
          strcpy(graphs_filenames[number_of_graphs], "graphs/");
          strcat(graphs_filenames[number_of_graphs], dir->d_name);
          number_of_graphs++;
        }
      }
      closedir(d);
    } else {
      printf("Error opening graphs/ folder\n");
      return 2;
    }
  }

  if (n_threads > get_nprocs()) {
    /* if the number of threads exceeds the number of available logic
     * processors, the coloring is slower and more error prone */
    printf(
        "Lowering the number of threads to %d (number of available logic "
        "processors in the system)\n",
        get_nprocs());
    n_threads = get_nprocs();
  }

  char *coloring_methods[N_COLORING_METHODS] = {
      "seq_greedy", "seq_ldf", "par_jp", "par_ldf", "par_ldf_plus"};

  Results res;  // struct to hold the results of a coloring (in terms of time
                // and colors used, not the coloring itself)
  FILE *csv_file = NULL;
  char csv_filename[128];
  if (export) {
    /* create the csv file where to export results */
    time_t t = time(NULL);
    struct tm now = *localtime(&t);
    sprintf(csv_filename, "results/results_%d-%02d-%02d_%02d-%02d-%02d.csv",
            now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour,
            now.tm_min, now.tm_sec);
    csv_file = fopen(csv_filename, "w");
    if (csv_file == NULL) {
      printf("Error opening %s\n", csv_filename);
      return 3;
    }
    fprintf(csv_file,
            "graph_name,vertex_count,coloring_method,n_threads,coloring_time,"
            "colors_used\n");  // add the header line
    fclose(csv_file);
  }

  res.n_threads = n_threads;

  if (number_of_graphs == 0) {
    printf("No graphs found in the /graphs subfolder!\n");
  }

  for (int i = 0; i < number_of_graphs; i++) {
    /* for each graph */
    double start, finish;

    /* load the graph from file */
    start = UTIL_get_time();
    Graph G = GRAPH_load_from_file(graphs_filenames[i]);
    finish = UTIL_get_time();

    /* take the portion of the filename after the last '/' slash */
    char *s = graphs_filenames[i];
    char *last = strrchr(s, '/');
    if (last != NULL) {
      last++;
    } else {
      last = s;
    }
    res.graph_name = last;

    if (G != NULL) {
      res.vertex_count = GRAPH_get_vertex_count(G);
      printf(
          "         GRAPH NAME | LOADED IN | MAX DEGREE | ESTIMATED MEMORY "
          "FOOTPRINT\n");
      printf("%19s | %09f |     %02d     | %f MB\n", last, finish - start,
             UTIL_max_in_array(GRAPH_get_degrees(G), res.vertex_count),
             ((double)GRAPH_compute_bytes(G)) / 1024 / 1024);

      for (int k = 0; k < iterations; k++) {
        /* for each iteration */
        if (iterations > 1) {
          printf("Iteration %d of %d\n", k + 1, iterations);
        }

        printf("COLOR METHOD | COLORED IN | COLORS USED | VALID?\n");

        for (int method_number = 0; method_number < N_COLORING_METHODS;
             method_number++) {
          /* for each coloring method */
          
          /* skip sequential methods if --par flag had been set */
          if (par_only &&
              (!strcmp(coloring_methods[method_number], "seq_greedy") ||
               !strcmp(coloring_methods[method_number], "seq_ldf"))) {
            continue;
          }

          res.coloring_method = coloring_methods[method_number];
          /* color the graph */
          start = UTIL_get_time();
          unsigned int *colors =
              GRAPH_color(G, res.coloring_method, (unsigned int)n_threads);
          finish = UTIL_get_time();

          /* if the coloring succeeds (i.e: GRAPH_color() returns something !=
           * NULL)*/
          if (colors != NULL) {
            res.colors_used = UTIL_max_in_array(
                colors,
                GRAPH_get_vertex_count(
                    G));  // the number of colors used is the maximum color used
            res.coloring_time = finish - start;
            printf("%12s | %09f  |     %02d      | ", res.coloring_method,
                   res.coloring_time, res.colors_used);

            /* check whether or not the produced coloring is valid */
            if (GRAPH_check_given_coloring_validity(G, colors)) {
              printf("YES \n");

              if (export) {
                // export to csv if flag had been set
                csv_file = fopen(csv_filename, "a");
                if (csv_file == NULL) {
                  printf("Error opening %s in append mode\n", csv_filename);
                } else {
                  fprintf(csv_file, "%s,%d,%s,%d,%09f,%d\n", res.graph_name,
                          res.vertex_count, res.coloring_method, res.n_threads,
                          res.coloring_time, res.colors_used);
                  fclose(csv_file);
                }
              }
            } else {
              printf("NO \n");
            }
          }
        }
      }
      putchar('\n');
      putchar('\n');
      /* after coloring the graph with each method, finally free it */
      GRAPH_free(G);
      sleep(1);
    }
  }

  return 0;
}