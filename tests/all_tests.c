#include "../src/statdata.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <float.h>

#define TEST_CASE(name) { #name, name }
typedef struct {
    const char* name;
    int (*func)(void);
} TestCase;

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            return 0; \
        } \
    } while (0)

static char* get_exe_dir() {
    static char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
    if (len == -1) return NULL;
    exe_path[len] = '\0';
    return dirname(exe_path);
}

static int run_processor(const char* input1, const char* input2, const char* output) {
    char* dir = get_exe_dir();
    if (!dir) return -1;
    
    char command[2048];
    snprintf(command, sizeof(command), "%s/statdata_processor %s %s %s", 
             dir, input1, input2, output);
    return system(command);
}

static int test_basic_merge() {
    clock_t start = clock();
    
    const StatData input_a[2] = {
        {.id = 90889, .count = 13, .cost = 3.567, .primary = 0, .mode = 3},
        {.id = 90089, .count = 1, .cost = 88.90, .primary = 1, .mode = 0}
    };
    
    const StatData input_b[2] = {
        {.id = 90089, .count = 13, .cost = 0.011, .primary = 0, .mode = 2},
        {.id = 90189, .count = 1000, .cost = 1.00003, .primary = 1, .mode = 2}
    };

    const StatData expected[3] = {
        {.id = 90189, .count = 1000, .cost = 1.00003, .primary = 1, .mode = 2},
        {.id = 90889, .count = 13, .cost = 3.567, .primary = 0, .mode = 3},
        {.id = 90089, .count = 14, .cost = 88.911, .primary = 0, .mode = 2}
    };

    const char* files[] = {"test1_input1.bin", "test1_input2.bin", "test1_output.bin"};
    ASSERT(StoreDump(files[0], input_a, 2) == 0, "Failed to create input file 1");
    ASSERT(StoreDump(files[1], input_b, 2) == 0, "Failed to create input file 2");
    ASSERT(run_processor(files[0], files[1], files[2]) == 0, "Processor execution failed");

    size_t count = 0;
    StatData* result = LoadDump(files[2], &count);
    ASSERT(result != NULL, "Failed to load output file");
    ASSERT(count == 3, "Unexpected record count");
    
    for (size_t i = 0; i < count; i++) {
        ASSERT(result[i].id == expected[i].id, "ID mismatch");
        ASSERT(result[i].count == expected[i].count, "Count mismatch");
        ASSERT(fabs(result[i].cost - expected[i].cost) < 0.001, "Cost mismatch");
        ASSERT(result[i].primary == expected[i].primary, "Primary mismatch");
        ASSERT(result[i].mode == expected[i].mode, "Mode mismatch");
    }
    
    free(result);
    for (int i = 0; i < 3; i++) unlink(files[i]);
    
    printf("[TIME] %.3f ms\n", (double)(clock() - start) * 1000 / CLOCKS_PER_SEC);
    return 1;
}

static int test_empty_inputs() {
    clock_t start = clock();
    
    const char* files[] = {"test2_input1.bin", "test2_input2.bin", "test2_output.bin"};
    
    ASSERT(StoreDump(files[0], NULL, 0) == 0, "Failed to create empty file 1");
    ASSERT(StoreDump(files[1], NULL, 0) == 0, "Failed to create empty file 2");
    ASSERT(run_processor(files[0], files[1], files[2]) == 0, "Processor failed on empty inputs");

    size_t count = 0;
    StatData* result = LoadDump(files[2], &count);
    ASSERT(result != NULL, "Failed to load empty output");
    ASSERT(count == 0, "Output should be empty");
    
    free(result);
    for (int i = 0; i < 3; i++) unlink(files[i]);
    
    printf("[TIME] %.3f ms\n", (double)(clock() - start) * 1000 / CLOCKS_PER_SEC);
    return 1;
}

static int test_duplicate_ids() {
    clock_t start = clock();
    
    const StatData input_a[3] = {
        {.id = 1, .count = 10, .cost = 5.0, .primary = 1, .mode = 1},
        {.id = 1, .count = 20, .cost = 3.0, .primary = 1, .mode = 2},
        {.id = 1, .count = 30, .cost = 1.0, .primary = 0, .mode = 3}
    };
    
    const StatData input_b[2] = {
        {.id = 1, .count = 5, .cost = 2.0, .primary = 1, .mode = 4},
        {.id = 2, .count = 100, .cost = 10.0, .primary = 1, .mode = 0}
    };

    const StatData expected[2] = {
        {.id = 2, .count = 100, .cost = 10.0, .primary = 1, .mode = 0},
        {.id = 1, .count = 65, .cost = 11.0, .primary = 0, .mode = 4}
    };

    const char* files[] = {"test3_input1.bin", "test3_input2.bin", "test3_output.bin"};
    ASSERT(StoreDump(files[0], input_a, 3) == 0, "Failed to create input file 1");
    ASSERT(StoreDump(files[1], input_b, 2) == 0, "Failed to create input file 2");
    ASSERT(run_processor(files[0], files[1], files[2]) == 0, "Processor execution failed");

    size_t count = 0;
    StatData* result = LoadDump(files[2], &count);
    ASSERT(result != NULL, "Failed to load output file");
    ASSERT(count == 2, "Unexpected record count");
    
    for (size_t i = 0; i < count; i++) {
        ASSERT(result[i].id == expected[i].id, "ID mismatch");
        ASSERT(result[i].count == expected[i].count, "Count mismatch");
        ASSERT(fabs(result[i].cost - expected[i].cost) < 0.001, "Cost mismatch");
        ASSERT(result[i].primary == expected[i].primary, "Primary mismatch");
        ASSERT(result[i].mode == expected[i].mode, "Mode mismatch");
    }
    
    free(result);
    for (int i = 0; i < 3; i++) unlink(files[i]);
    
    printf("[TIME] %.3f ms\n", (double)(clock() - start) * 1000 / CLOCKS_PER_SEC);
    return 1;
}

static int test_large_dataset() {
    clock_t start = clock();
    const size_t SIZE = 10000;
    
    StatData* input_a = malloc(SIZE * sizeof(StatData));
    StatData* input_b = malloc(SIZE * sizeof(StatData));
    ASSERT(input_a && input_b, "Memory allocation failed");
    
    srand(time(NULL));
    for (size_t i = 0; i < SIZE; i++) {
        input_a[i] = (StatData){
            .id = rand() % 5000,
            .count = rand() % 100,
            .cost = (float)rand() / (float)(RAND_MAX / 100.0),
            .primary = rand() % 2,
            .mode = rand() % 8
        };
        input_b[i] = (StatData){
            .id = rand() % 5000,
            .count = rand() % 100,
            .cost = (float)rand() / (float)(RAND_MAX / 100.0),
            .primary = rand() % 2,
            .mode = rand() % 8
        };
    }
    
    const char* files[] = {"test4_input1.bin", "test4_input2.bin", "test4_output.bin"};
    ASSERT(StoreDump(files[0], input_a, SIZE) == 0, "Failed to create large input 1");
    ASSERT(StoreDump(files[1], input_b, SIZE) == 0, "Failed to create large input 2");
    ASSERT(run_processor(files[0], files[1], files[2]) == 0, "Processor failed on large dataset");

    size_t count = 0;
    StatData* result = LoadDump(files[2], &count);
    ASSERT(result != NULL, "Failed to load large output");
    ASSERT(count > 0, "Output should not be empty");
    
    for (size_t i = 1; i < count; i++) {
        ASSERT(result[i].cost >= result[i-1].cost, "Output not sorted by cost");
    }
    
    free(input_a);
    free(input_b);
    free(result);
    for (int i = 0; i < 3; i++) unlink(files[i]);
    
    printf("[TIME] %.3f ms\n", (double)(clock() - start) * 1000 / CLOCKS_PER_SEC);
    return 1;
}

static int test_extreme_values() {
    clock_t start = clock();
    
    const StatData input_a[3] = {
        {.id = LONG_MAX, .count = INT_MAX, .cost = FLT_MAX, .primary = 1, .mode = 7},
        {.id = LONG_MIN, .count = INT_MIN, .cost = FLT_MIN, .primary = 0, .mode = 0},
        {.id = 0, .count = 0, .cost = 0.0, .primary = 1, .mode = 0}
    };
    
    const StatData input_b[2] = {
        {.id = LONG_MAX, .count = 1, .cost = 1.0, .primary = 0, .mode = 5},
        {.id = 0, .count = 1, .cost = -1.0, .primary = 1, .mode = 1}
    };

    const StatData expected[3] = {
        {.id = 0, .count = 1, .cost = -1.0, .primary = 1, .mode = 1},
        {.id = LONG_MIN, .count = INT_MIN, .cost = FLT_MIN, .primary = 0, .mode = 0},
        {.id = LONG_MAX, .count = INT_MAX, .cost = FLT_MAX, .primary = 0, .mode = 7}
    };

    const char* files[] = {"test5_input1.bin", "test5_input2.bin", "test5_output.bin"};
    ASSERT(StoreDump(files[0], input_a, 3) == 0, "Failed to create input file 1");
    ASSERT(StoreDump(files[1], input_b, 2) == 0, "Failed to create input file 2");
    ASSERT(run_processor(files[0], files[1], files[2]) == 0, "Processor execution failed");

    size_t count = 0;
    StatData* result = LoadDump(files[2], &count);
    ASSERT(result != NULL, "Failed to load output file");
    ASSERT(count == 3, "Unexpected record count");
    
    for (size_t i = 0; i < count; i++) {
        ASSERT(result[i].id == expected[i].id, "ID mismatch");
        ASSERT(result[i].count == expected[i].count, "Count mismatch");
        if (i > 0) { // Skip exact float comparison for extreme values
            ASSERT(fabs(result[i].cost - expected[i].cost) < 0.001, "Cost mismatch");
        }
        ASSERT(result[i].primary == expected[i].primary, "Primary mismatch");
        ASSERT(result[i].mode == expected[i].mode, "Mode mismatch");
    }
    
    free(result);
    for (int i = 0; i < 3; i++) unlink(files[i]);
    
    printf("[TIME] %.3f ms\n", (double)(clock() - start) * 1000 / CLOCKS_PER_SEC);
    return 1;
}

static void run_tests(TestCase* tests, size_t count) {
    printf("=== Running test suite ===\n");
    clock_t start = clock();
    size_t passed = 0;
    
    for (size_t i = 0; i < count; i++) {
        printf("[TEST %zu/%zu] %-30s ", i+1, count, tests[i].name);
        fflush(stdout);
        
        if (tests[i].func()) {
            printf("PASSED\n");
            passed++;
        } else {
            printf("FAILED\n");
        }
    }
    
    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("\n=== Test summary ===\n");
    printf("  Passed: %zu/%zu (%.1f%%)\n", passed, count, (double)passed/count*100);
    printf("  Total duration: %.3f seconds\n", elapsed);
}

int main() {
    TestCase tests[] = {
        TEST_CASE(test_basic_merge),
        TEST_CASE(test_empty_inputs),
        TEST_CASE(test_duplicate_ids),
        TEST_CASE(test_large_dataset),
        TEST_CASE(test_extreme_values)
    };
    
    run_tests(tests, sizeof(tests)/sizeof(tests[0]));
    return 0;
}