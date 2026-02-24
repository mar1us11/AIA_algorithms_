#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#elif __APPLE__
    #include <mach/mach.h>
#endif

// -------- Memory Usage --------

double getMemoryUsageMB() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS memInfo;
    GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo));
    return memInfo.WorkingSetSize / (1024.0 * 1024.0);
#elif __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count);
    return info.resident_size / (1024.0 * 1024.0);
#else
    FILE *f = fopen("/proc/self/status", "r");
    char line[128];
    while (fgets(line, 128, f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            long kb;
            sscanf(line + 6, "%ld", &kb);
            fclose(f);
            return kb / 1024.0;
        }
    }
    fclose(f);
    return 0.0;
#endif
}

// -------- Bubble Sort --------

void bubble_sort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

// -------- Data Generators --------

void gen_sorted(int arr[], int size) {
    for (int i = 0; i < size; i++) arr[i] = i;
}

void gen_reverse(int arr[], int size) {
    for (int i = 0; i < size; i++) arr[i] = size - i;
}

void gen_random(int arr[], int size) {
    for (int i = 0; i < size; i++)
        arr[i] = ((unsigned int)rand() << 17) ^ ((unsigned int)rand() << 9) ^ (unsigned int)rand();
}

void gen_half_sorted(int arr[], int size) {
    for (int i = 0; i < size / 2; i++) arr[i] = i;
    for (int i = size / 2; i < size; i++)
        arr[i] = ((unsigned int)rand() << 17) ^ ((unsigned int)rand() << 9) ^ (unsigned int)rand();
}

// -------- Benchmark --------

void benchmark_case(const char *name, void (*generator)(int[], int),
                    int *original, int *arr, int size, int runs) {
    double total_time = 0.0;
    double total_mem = 0.0;

    for (int r = 0; r < runs; r++) {
        generator(original, size);
        memcpy(arr, original, size * sizeof(int));

#ifdef _WIN32
        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        bubble_sort(arr, size);
        QueryPerformanceCounter(&end);
        double time_taken = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
#else
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        bubble_sort(arr, size);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) +
                            (end.tv_nsec - start.tv_nsec) / 1e9;
#endif

        total_time += time_taken;
        total_mem += getMemoryUsageMB();
    }

    printf("    %-20s avg time: %10.6f s  avg mem: %.2f MB\n",
           name, total_time / runs, total_mem / runs);
}

int main() {
    int sizes[] = {10, 100, 1000, 10000, 100000, 1000000};
    int runs[]  = {50,  50,   50,    50,      10,        2};
    int num_sizes = 6;

    int max_size = sizes[num_sizes - 1];
    int *original = malloc(max_size * sizeof(int));
    int *arr      = malloc(max_size * sizeof(int));

    if (!original || !arr) {
        printf("Memory allocation failed\n");
        return 1;
    }

    srand(time(NULL));

    for (int s = 0; s < num_sizes; s++) {
        int size = sizes[s];
        int r    = runs[s];
        printf("\n=== Size: %d, Runs: %d ===\n", size, r);
        benchmark_case("Sorted",        gen_sorted,      original, arr, size, r);
        benchmark_case("Reverse Sorted",gen_reverse,     original, arr, size, r);
        benchmark_case("Random",        gen_random,      original, arr, size, r);
        benchmark_case("Half Sorted",   gen_half_sorted, original, arr, size, r);
    }

    free(original);
    free(arr);

    return 0;
}