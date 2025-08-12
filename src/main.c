#include "statdata.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <file1> <file2> <output>\n", argv[0]);
        return 1;
    }
    
    size_t count1 = 0, count2 = 0;
    StatData* data1 = LoadDump(argv[1], &count1);
    StatData* data2 = LoadDump(argv[2], &count2);
    
    if ((count1 > 0 && !data1) || (count2 > 0 && !data2)) {
        fprintf(stderr, "Failed to load input files\n");
        free(data1);
        free(data2);
        return 1;
    }
    
    size_t result_count = 0;
    StatData* result = JoinDump(data1, count1, data2, count2, &result_count);
    free(data1);
    free(data2);
    
    if (result_count > 0 && !result) {
        fprintf(stderr, "Failed to join data\n");
        return 1;
    }
    
    // sort by cost
    SortDump(result, result_count);
    
    PrintStatData(result, result_count, 10);
    
    if (StoreDump(argv[3], result, result_count) != 0) {
        fprintf(stderr, "Failed to save result\n");
        free(result);
        return 1;
    }
    
    free(result);
    return 0;
}