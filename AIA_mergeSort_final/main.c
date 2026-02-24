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

// -------- Merge Sort --------

void merge_into(int src[], int dst[], int left, int mid, int right) {
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right)
        dst[k++] = src[i] <= src[j] ? src[i++] : src[j++];
    while (i <= mid) dst[k++] = src[i++];
    while (j <= right) dst[k++] = src[j++];
}

void merge_sort(int src[], int dst[], int left, int right) {
    if (left >= right) {
        dst[left] = src[left];
        return;
    }
    int mid = (left + right) / 2;
    merge_sort(dst, src, left, mid);
    merge_sort(dst, src, mid + 1, right);
    if (src[mid] <= src[mid + 1]) return;
    merge_into(src, dst, left, mid, right);
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
                    int *original, int *arr, int *temp, int size, int runs) {
    double total_time = 0.0;
    double total_mem = 0.0;

    for (int r = 0; r < runs; r++) {
        generator(original, size);
        memcpy(arr, original, size * sizeof(int));
        memcpy(temp, arr, size * sizeof(int));

#ifdef _WIN32
        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        merge_sort(temp, arr, 0, size - 1);
        QueryPerformanceCounter(&end);
        double time_taken = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
#else
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        merge_sort(temp, arr, 0, size - 1);
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
    int num_sizes = 6;
    int runs = 100;

    // allocate for max size
    int max_size = sizes[num_sizes - 1];
    int *original = malloc(max_size * sizeof(int));
    int *arr      = malloc(max_size * sizeof(int));
    int *temp     = malloc(max_size * sizeof(int));

    if (!original || !arr || !temp) {
        printf("Memory allocation failed\n");
        return 1;
    }

    srand(time(NULL));

    for (int s = 0; s < num_sizes; s++) {
        int size = sizes[s];
        printf("\n=== Size: %d, Runs: %d ===\n", size, runs);
        benchmark_case("Sorted",       gen_sorted,      original, arr, temp, size, runs);
        benchmark_case("Reverse Sorted",gen_reverse,    original, arr, temp, size, runs);
        benchmark_case("Random",       gen_random,      original, arr, temp, size, runs);
        benchmark_case("Half Sorted",  gen_half_sorted, original, arr, temp, size, runs);
    }

    free(original);
    free(arr);
    free(temp);

    return 0;
}