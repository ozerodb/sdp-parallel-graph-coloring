#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>

double UTIL_get_time();
unsigned int UTIL_smallest_missing_number(int *arr, unsigned int size);
void UTIL_randomize_array(unsigned int arr[], unsigned int n);
void UTIL_print_array(unsigned int arr[], int n);
void UTIL_heapsort_values_by_keys(unsigned int n, unsigned int keys[],
                        unsigned int values[]);
unsigned int UTIL_max_in_array(unsigned int arr[], unsigned int size);
void UTIL_stable_qsort_values_by_keys(unsigned int degrees[], unsigned int indexes[],
                             unsigned int n);

#endif