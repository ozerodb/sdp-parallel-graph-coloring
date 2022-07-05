#include "util.h"

#include <stdlib.h>

static void swap_uint(unsigned int *a, unsigned int *b) {
  int temp;
  temp = *a;
  *a = *b;
  *b = temp;
}

static void swap_int(int *a, int *b) {
  int temp;
  temp = *a;
  *a = *b;
  *b = temp;
}

/* Utility function that puts all
non-positive (0 and negative) numbers on left
side of arr[] and return count of such numbers */
static int segregate(int arr[], unsigned int size) {
  int j = 0, i;
  for (i = 0; i < size; i++) {
    if (arr[i] <= 0) {
      swap_int(&arr[i], &arr[j]);
      j++;  // increment count of non-positive integers
    }
  }

  return j;
}

/* Find the smallest positive missing number
in an array that contains all positive integers */
static int findMissingPositive(int arr[], unsigned int size) {
  int i;
  
  // Mark arr[i] as visited by
  // making arr[arr[i] - 1] negative.
  // Note that 1 is subtracted
  // because index start
  // from 0 and positive numbers start from 1
  for (i = 0; i < size; i++) {
    if (abs((int)arr[i]) - 1 < size && arr[abs((int)arr[i]) - 1] > 0)
      arr[abs((int)arr[i]) - 1] = -arr[abs((int)arr[i]) - 1];
  }

  // Return the first index value at which is positive
  for (i = 0; i < size; i++)
    if (arr[i] > 0)
      // 1 is added because indexes start from 0
      return i + 1;

  return size + 1;
}

static void heapify(unsigned int keys[], unsigned int values[],
                    unsigned int n, int i) {
  int max = i;  // Initialize max as root
  int leftChild = 2 * i + 1;
  int rightChild = 2 * i + 2;

  // If left child is greater than root
  if (leftChild < n && keys[leftChild] > keys[max]) max = leftChild;

  // If right child is greater than max
  if (rightChild < n && keys[rightChild] > keys[max]) max = rightChild;

  // If max is not root
  if (max != i) {
    swap_uint(&keys[i], &keys[max]);
    swap_uint(&values[i], &values[max]);
    // heapify the affected sub-tree recursively
    heapify(keys, values, n, max);
  }
}

struct sort_item {
  unsigned int key;
  unsigned int value;
};

static int compare_keys_stable(const void *a, const void *b) {
  struct sort_item *va = (struct sort_item *)a;
  struct sort_item *vb = (struct sort_item *)b;
  int diff = va->key - vb->key;
  return diff != 0 ? diff : va->value - vb->value;
}

/* EXPOSED FUNCTIONS */

unsigned int UTIL_smallest_missing_number(int *arr, unsigned int size) {
  int shift = segregate(arr, size);
  return findMissingPositive(arr + shift, size - shift);
}

double UTIL_get_time() {
  struct timeval t;
  struct timezone tzp;
  gettimeofday(&t, &tzp);
  return t.tv_sec + t.tv_usec * 1e-6;
}

void UTIL_randomize_array(unsigned int arr[], unsigned int n) {
  // Use a different seed value so that we don't get same
  // result each time we run this program
  srand(time(NULL));

  // Start from the last element and swap one by one. We don't
  // need to run for the first element that's why i > 0
  for (int i = n - 1; i > 0; i--) {
    // Pick a random index from 0 to i
    int j = rand() % (i + 1);

    // Swap arr[i] with the element at random index
    swap_uint(&arr[i], &arr[j]);
  }
}

void UTIL_print_array(unsigned int arr[], int n) {
  for (int i = 0; i < n; i++) printf("%d ", arr[i]);
  printf("\n");
}

void UTIL_heapsort_values_by_keys(unsigned int n, unsigned int keys[],
                        unsigned int values[]) {
  // Rearrange array (building heap)
  for (int i = n / 2 - 1; i >= 0; i--) heapify(keys, values, n, i);

  // Extract elements from heap one by one
  for (int i = n - 1; i >= 0; i--) {
    swap_uint(&keys[0], &keys[i]);  // Current root moved to the end
    swap_uint(&values[0], &values[i]);
    heapify(keys, values, i, 0);  // calling max heapify on the heap reduced
  }
}

unsigned int UTIL_max_in_array(unsigned int arr[], unsigned int size) {
  unsigned int max = 0;
  for (unsigned int i = 0; i < size; i++) {
    if (arr[i] > max) {
      max = arr[i];
    }
  }
  return max;
}


void UTIL_stable_qsort_values_by_keys(unsigned int keys[], unsigned int values[],
                             unsigned int n) {
  struct sort_item *a = malloc(n * sizeof(struct sort_item));

  if (a == NULL){
    fprintf(stderr,"Error while allocating sort_item array\n");
    return;
  }

  for (int i = 0; i < n; i++) {
    a[i].key = keys[i];
    a[i].value = values[i];
  }

  qsort(a, n, sizeof(*a), compare_keys_stable);

  for (int i = 0; i < n; i++) {
    keys[i] = a[i].key;
    values[i] = a[i].value;
  }

  free(a);
}