#ifndef STATDATA_H
#define STATDATA_H

#include <stddef.h>
#include <limits.h>

#pragma pack(push, 1)
typedef struct StatData {
    long id;
    int count;
    float cost;
    unsigned int primary:1;
    unsigned int mode:3;
} StatData;
#pragma pack(pop)

int StoreDump(const char* filename, const StatData* data, size_t count);
StatData* LoadDump(const char* filename, size_t* count);
StatData* JoinDump(const StatData* data1, size_t count1,
                  const StatData* data2, size_t count2,
                  size_t* result_count);
void SortDump(StatData* data, size_t count);
void PrintStatData(const StatData* data, size_t count, size_t limit);

#endif // STATDATA_H