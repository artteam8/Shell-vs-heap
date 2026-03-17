#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/// @defgroup storage Storage
/// @brief Global variables for testing arrays
/// @{
int **swapped_shell;
int **compared_shell;

int *swapped_heap;
int *compared_heap;

double **shell_arrays;
double **heap_arrays;
double **copy_arrays;
/// @}

/// @defgroup pointers Counter pointers
/// We are restricted to pass operation counters to sort functions,
/// so we will make global pointer variables that
/// point to the right counter each time.
/// @{
int *swapped;
int *compared;
/// @}

/// @defgroup compare Compare functions
/// @brief Used for sorting
/// @{

/// Tries to compare bits, because EPS in fabs(*a - *b) < EPS cannot be chosen small enough.\n
/// If not equal, just compare them regular way.\n
/// @param a double pointer
/// @param b double pointer
int
compare_double(const void *a, const void *b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    da = fabs(da);
    db = fabs(db);

    uint64_t a_bits, b_bits;
    memcpy(&a_bits, &da, sizeof(uint64_t));
    memcpy(&b_bits, &db, sizeof(uint64_t));

    if (a_bits == b_bits) {
        return 0;
    } else {
        if (da > db) {
            return 1;
        } else {
            return -1;
        }
    }
}

/// Same, but for reverse-sorting
int
compare_double_inverted(const void *a, const void *b) {
    return -1 * compare_double(a, b);
}
/// @}

/// @defgroup random Random functions
/// @brief Using time.h
/// @{

/// Makes 64bit double from two 32bit random outputs.\n
/// Shift left one time is necessary because RAND_MAX=2147483647 and the highest bit is zero.\n
/// \n !!!won't work if random output is not 32bit or RAND_MAX!=2147483647!!!
double
random_double() {
    uint64_t high = ((uint64_t) rand() << 1) | (rand() & 0x1);
    uint64_t low = ((uint64_t) rand() << 1) | (rand() & 0x1);
    uint64_t bits = (high << 32) | low;
    double result;
    memcpy(&result, &bits, sizeof(result));

    if (isnan(result)) {
        return random_double();
    }
    return result;
}

/// Random numbers array of given length.
void
random_arr(double *arr, int len) {
    for (int i = 0; i < len; ++i) {
        arr[i] = random_double();
    }
}

/// We assume that qsort works correct.
void
random_sorted_arr(double *arr, int len) {
    random_arr(arr, len);
    qsort(arr, len, sizeof(arr[0]), compare_double);
}

/// Reverse sorting ¯\\(ツ)/¯
void
random_reverse_sorted_arr(double *arr, int len) {
    random_arr(arr, len);
    qsort(arr, len, sizeof(arr[0]), compare_double_inverted);
}
/// @}

/// Debug print.
void
print_arr(double *arr, int len) {
    printf("\n");
    for (int i = 0; i < len; ++i) {
        printf("%e ", arr[i]);
    }
    printf("\n");
}

/// <em>clz</em> returns number of leading zeros.\n
/// 31 - <em>clz</em> is the highest bit set to 1 in binary representation; that means the closest power of 2.
unsigned int
floor_log2(unsigned int n) {
    return 31 - __builtin_clz(n);
}

/// @defgroup gap Gap functions
/// @brief For the k'th step of sorting, return current gap.
/// @{

/// Base gap function for docs. k is increasing, so we go from huge gaps to 1.
/// @param k current step
/// @param len array length 
/// @return current gap
typedef int
(*gap)(int k, int len);

/// @copydoc gap
/// from https://web.archive.org/web/20170830020037/http://penguin.ewu.edu/cscd300/Topic/AdvSorting/p30-shell.pdf
int
gap_shell(int k, int len) {
    return len / (1 << k);
}

/// @copydoc gap
/// from Knuth's "The Art of Computer Programming" Vol. 3
int
gap_hibbard(int k, int len) {
    return (1 << (floor_log2((unsigned) len) + 1 - k)) - 1;
}

/// Knuth's sequence is recursive, so we need to compute it one time and store
int *knuth_gaps = NULL;
int knuth_size = 0;

/// @copydoc gap
/// from Knuth's "The Art of Computer Programming" Vol. 3
int
gap_knuth(int k, int len) {
    if (knuth_size == 0) {
        knuth_gaps = calloc(len, sizeof(int));
        int cur_gap = 1;
        int i = 0;
        while (cur_gap <= len) {
            knuth_gaps[i] = cur_gap;
            cur_gap = 3 * cur_gap + 1;
            ++i;
        }
        knuth_size = i;
    }
    return knuth_gaps[knuth_size - k];
}

/// @copydoc gap
/// https://web.archive.org/web/20180923235211/http://sun.aei.polsl.pl/~mciura/publikacje/shellsort.pdf\n\n
/// Why not store sequence in memory, like Knuth's? \n
/// Because Ciura's sequence is not computable and made of constants, so I just typed them by hand.
int
gap_ciura(int k, int len) {
    int seq[] = {1, 4, 10, 23, 57, 132, 301, 701, 1750, 3938, 8860};
    int i = 10;
    while (i > 0 && len <= seq[i]) {
        --i;
    }
    return seq[i - k + 1];
}
/// @}

/// Base function for sort.
/// @param arr pointer to array we want to sort
/// @param len length of the array
/// @param compared pointer to compare counter. leave as NULL if you dont need it
/// @param swapped pointer to swap counter. leave as NULL if you dont need it
typedef void
(*sort)(double *arr, int len, int *compared, int *swapped, gap get_gap);

/// https://www.geeksforgeeks.org/dsa/shell-sort/
/// @copydoc sort
/// @param get_gap @ref gap any "gap function" that decreases to 1
void
shell_sort(double *arr, int len, gap get_gap) {
    int k = 1;
    int cur_gap = get_gap(k, len);
    do {
        cur_gap = get_gap(++k, len);
        for (int i = cur_gap; i < len; ++i) {
            double temp = arr[i];
            int j = i;

            while (j >= cur_gap) {
                if (compared) ++*compared;
                if (compare_double(&arr[j - cur_gap], &temp) == 1) {
                    arr[j] = arr[j - cur_gap];
                    j = j - cur_gap;
                    if (swapped) ++*swapped;
                } else {
                    break;
                }
            }

            if (j != i) {
                arr[j] = temp;
                if (swapped) ++*swapped;
            }
        }

    } while (cur_gap > 1);
}

/// @defgroup heap Heap sort
/// @brief https://www.geeksforgeeks.org/c/c-program-for-heap-sort/
/// @{

/// Sifting down.
void
heapify(double* arr, int len, int root_index) {
    int largest = root_index;
    int left = 2 * root_index + 1;
    int right = 2 * root_index + 2;

    if (left < len) {
        if (compared) ++*compared;
        if (compare_double(&arr[left], &arr[largest]) == 1) {
            largest = left;
        }
    }

    if (right < len) {
        if (compared) ++*compared;
        if (compare_double(&arr[right], &arr[largest]) == 1) {
            largest = right;
        }
    }

    if (largest != root_index) {
        double temp = arr[root_index];
        arr[root_index] = arr[largest];
        arr[largest] = temp;
        if (swapped) ++*swapped;

        heapify(arr, len, largest);
    }
}

/// First loop creates the heap, second sifts elements until all are sifted and array is sorted
/// @copydoc sort
void
heap_sort(double *arr, int len) {
    for (int i = len / 2 - 1; i >= 0; --i) {
        heapify(arr, len, i);
    }

    for (int i = len - 1; i > 0; --i) {
        double temp = arr[0];
        arr[0] = arr[i];
        arr[i] = temp;
        if (swapped) ++*swapped;
        heapify(arr, i, 0);
    }
}

/// @}



/// Check that all elements remain in array
/// and it is sorted
/// uses @ref compare_double "compare double function"
int
check_arr(double *arr, double *numbers, int len) {
    int *marks = calloc(len, sizeof(int));

    for (int i = 0; i < len; ++i) {
        int found = 0;

        for (int j = 0; j < len; ++j) {
            if (compare_double(&numbers[i], &arr[j]) == 0 && !marks[j]) {
                marks[j] = 1;
                found = 1;
            }
        }
        if (!found) {
            free(marks);
            return 0;
        }
    }

    free(marks);

    for (int i = 1; i < len; ++i) {
        if (compare_double(&arr[i - 1], &arr[i]) == 1) {
            return 0;
        }
    }

    return 1;
}



/// Tests sort functions for fixed n on the same arrays\n
/// \n swapped_shell and compared_shell contain num of swaps and compares for each GAP FUNCTION and then for each ARRAY. swapped_heap and compared_heap contain swaps and compares for each ARRAY.
int
test(int n) {
    const int num_arrays = 4;

    char *func_names[] = {"Shell", "Hibbard", "Knuth", "Ciura"};
    int (*gap_funcs[])(int, int) = {gap_shell, gap_hibbard, gap_knuth, gap_ciura};
    const int num_gaps = sizeof(gap_funcs) / sizeof(gap_funcs[0]);

    swapped_shell = calloc(num_gaps, sizeof(int*));
    compared_shell = calloc(num_gaps, sizeof(int*));

    for (int g = 0; g < num_gaps; ++g) {
        swapped_shell[g] = calloc(num_arrays, sizeof(int));
        compared_shell[g] = calloc(num_arrays, sizeof(int));
        for (int j = 0; j < num_arrays; ++j) {
            swapped_shell[g][j] = 0;
            compared_shell[g][j] = 0;
        }
    }

    swapped_heap = calloc(num_arrays, sizeof(int));
    compared_heap = calloc(num_arrays, sizeof(int));

    for (int j = 0; j < num_arrays; ++j) {
        swapped_heap[j] = 0;
        compared_heap[j] = 0;
    }

    shell_arrays = calloc(num_arrays, sizeof(double*));
    heap_arrays = calloc(num_arrays, sizeof(double*));
    copy_arrays = calloc(num_arrays, sizeof(double*));


    for (int i = 0; i < num_arrays; ++i) {
        shell_arrays[i] = calloc(n, sizeof(double));
        heap_arrays[i] = calloc(n, sizeof(double));
        copy_arrays[i] = calloc(n, sizeof(double));
    }

    random_sorted_arr(shell_arrays[0], n);
    random_reverse_sorted_arr(shell_arrays[1], n);
    random_arr(shell_arrays[2], n);
    random_arr(shell_arrays[3], n);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < num_arrays; ++j) {
            copy_arrays[j][i] = shell_arrays[j][i];
            heap_arrays[j][i] = shell_arrays[j][i];
        }
    }

    for (int g = 0; g < num_gaps; ++g) {
        for (int j = 0; j < num_arrays; ++j) {
            for (int i = 0; i < n; ++i) {
                shell_arrays[j][i] = copy_arrays[j][i];
            }
        }

        for (int j = 0; j < num_arrays; ++j) {
            swapped = &swapped_shell[g][j];
            compared = &compared_shell[g][j];
            shell_sort(shell_arrays[j], n, gap_funcs[g]);

            if (!check_arr(shell_arrays[j], copy_arrays[j], n)) {
                printf("ERROR!!! %s %d\n", func_names[g], j);
                print_arr(copy_arrays[j], n);
                print_arr(shell_arrays[j], n);
                for (int i = 0; i < num_arrays; ++i) {
                    free(shell_arrays[i]);
                    free(heap_arrays[i]);
                    free(copy_arrays[i]);
                }
                return 1;
            }
        }
    }

    for (int j = 0; j < num_arrays; ++j) {
        swapped = &swapped_heap[j];
        compared = &compared_heap[j];
        heap_sort(heap_arrays[j], n);

        if (!check_arr(heap_arrays[j], copy_arrays[j], n)) {
            printf("ERROR!!! %d\n", j);
            for (int i = 0; i < num_arrays; ++i) {
                free(shell_arrays[i]);
                free(heap_arrays[i]);
                free(copy_arrays[i]);
            }
            return 1;
        }
    }

    printf("\nShell sort:");
    for (int g = 0; g < num_gaps; ++g) {
        printf("\nGap function %s:\n", func_names[g]);
        printf("\\multirow{2}{*}{10} & Сравнения ");
        double avg_comp = 0;
        for (int j = 0; j < num_arrays; ++j) {
            printf("& %d ", compared_shell[g][j]);
            avg_comp += compared_shell[g][j];
        }
        printf("& %lf \\\\\n", avg_comp / num_arrays);

        printf("\\cline{2-7}\n& Перемещения ");
        double avg_swap = 0;
        for (int j = 0; j < num_arrays; ++j) {
            printf("& %d ", swapped_shell[g][j]);
            avg_swap += swapped_shell[g][j];
        }
        printf("& %lf \\\\\n", avg_swap / num_arrays);
    }

    printf("\nHeap sort: \n");
    printf("\\multirow{2}{*}{10} & Сравнения ");
    double avg_comp = 0;
    for (int j = 0; j < num_arrays; ++j) {
        printf("& %d ", compared_heap[j]);
        avg_comp += compared_heap[j];
    }
    printf("& %lf \\\\\n", avg_comp / num_arrays);

    printf("\\cline{2-7}\n& Перемещения ");
    double avg_swap = 0;
    for (int j = 0; j < num_arrays; ++j) {
        printf("& %d ", swapped_heap[j]);
        avg_swap += swapped_heap[j];
    }
    printf("& %lf \\\\\n", avg_swap / num_arrays);

    for (int i = 0; i < num_arrays; ++i) {
        free(shell_arrays[i]);
        free(heap_arrays[i]);
        free(copy_arrays[i]);
    }
    free(shell_arrays);
    free(heap_arrays);
    free(copy_arrays);
    free(swapped_heap);
    free(compared_heap);

    for (int i = 0; i < num_gaps; ++i) {
        free(swapped_shell[i]);
        free(compared_shell[i]);
    }
    free(swapped_shell);
    free(compared_shell);

    free(knuth_gaps);
    knuth_gaps = NULL;
    knuth_size = 0;

    return 0;
}

/// Testing with different n's, each time multiplying by 10\n
/// Set random seed once for all tests
int
main(void) {
    srand(time(NULL));
    int n = 10;

    for (int i = 0; i < 4; ++i) {
        printf("n = %d\n", n);
        test(n);
        printf("%d\n", n);
        printf("\n\n\n");
        n = n * 10;
    }

    return 0;
}
