#include "statdata.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

static int compare_by_cost(const void* a, const void* b) {
    float diff = ((const StatData*)a)->cost - ((const StatData*)b)->cost;
    return (diff > 0) - (diff < 0);
}

static int compare_by_id(const void* a, const void* b) {
    const StatData* sa = (const StatData*)a;
    const StatData* sb = (const StatData*)b;
    return (sa->id > sb->id) - (sa->id < sb->id);
}

int StoreDump(const char* filename, const StatData* data, size_t count) {
    if (!filename) return -1;
    
    if (count == 0) {
        // Handle empty file case
        FILE* file = fopen(filename, "wb");
        if (!file) return -1;
        fclose(file);
        return 0;
    }

    if (!data) return -1;
    
    FILE* file = fopen(filename, "wb");
    if (!file) return -1;
    
    size_t written = fwrite(data, sizeof(StatData), count, file);
    fclose(file);
    
    return written == count ? 0 : -1;
}

StatData* LoadDump(const char* filename, size_t* count) {
    if (!filename || !count) return NULL;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    *count = size / sizeof(StatData);
    
    if (size == 0) {
        fclose(file);
        *count = 0;
        return malloc(0);
    }
    
    if (size % sizeof(StatData) != 0) {
        fclose(file);
        return NULL;
    }
    
    StatData* data = malloc(size);
    if (!data) {
        fclose(file);
        return NULL;
    }
    
    if (fread(data, sizeof(StatData), *count, file) != *count) {
        free(data);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    return data;
}

void SortDump(StatData* data, size_t count) {
    if (data && count > 0) {
        qsort(data, count, sizeof(StatData), compare_by_cost);
    }
}

StatData* JoinDump(const StatData* data1, size_t count1,
                  const StatData* data2, size_t count2,
                  size_t* result_count) {
    if (!result_count) return NULL;
    *result_count = 0;
    
    size_t total = count1 + count2;
    if (total == 0) return malloc(0);
    
    StatData* combined = malloc(total * sizeof(StatData));
    if (!combined) return NULL;
    
    // copy w/o sorting
    if (data1 && count1) memcpy(combined, data1, count1 * sizeof(StatData));
    if (data2 && count2) memcpy(combined + count1, data2, count2 * sizeof(StatData));

    // sort by ID for correct join
    qsort(combined, total, sizeof(StatData), compare_by_id);
    
    size_t unique = 0;
    for (size_t i = 0; i < total;) {
        StatData current = combined[i];
        size_t j = i + 1;
        
        while (j < total && combined[j].id == current.id) {
            // add count, check overflow
            if ((combined[j].count > 0 && current.count > INT_MAX - combined[j].count) ||
                (combined[j].count < 0 && current.count < INT_MIN - combined[j].count)) {
                current.count = (combined[j].count > 0) ? INT_MAX : INT_MIN;
            } else {
                current.count += combined[j].count;
            }
            
            current.cost += combined[j].cost;
            current.primary = current.primary && combined[j].primary; // logic and
            current.mode = combined[j].mode > current.mode ? combined[j].mode : current.mode; // MAX
            j++;
        }
        
        combined[unique++] = current;
        i = j;
    }
    
    StatData* result = realloc(combined, unique * sizeof(StatData));//TODO: think it over (maybe remove realloc)
    *result_count = unique;
    return result ? result : combined;
}

void PrintStatData(const StatData* data, size_t count, size_t limit) {
    if (!data || !count) return;
    
    printf("%-10s %-10s %-15s %-8s %-5s\n", "ID", "Count", "Cost", "Primary", "Mode");
    printf("--------------------------------------------\n");
    
    size_t to_print = count < limit ? count : limit;
    long last_id = -1; // Track last printed ID to avoid duplicates
    
    for (size_t i = 0; i < to_print; i++) {
        if (data[i].id == last_id) continue; // Skip duplicates -TODO: remove ??
        
        printf("0x%-8lx %-10d %-15.3e %-8c ",
               data[i].id, data[i].count, data[i].cost,
               data[i].primary ? 'y' : 'n');
        
        for (int j = 2; j >= 0; j--) {
            putchar((data[i].mode >> j) & 1 ? '1' : '0');
        }
        putchar('\n');
        
        last_id = data[i].id;
    }
}



// ---------------------  HASH variant for big data ------------------- TODO: remove ??

#if 0

#include <uthash.h>

typedef struct {
    long id;                // ключ
    StatData data;          // значение
    UT_hash_handle hh;      // служебное поле uthash
} HashItem;

StatData* JoinDump_Optimized(const StatData* data1, size_t count1,
                           const StatData* data2, size_t count2,
                           size_t* result_count) {
    HashItem* hash = NULL;
    size_t total = count1 + count2;
    
    // Добавляем все элементы в хеш-таблицу
    for (size_t i = 0; i < total; i++) {
        const StatData* item = (i < count1) ? &data1[i] : &data2[i - count1];
        HashItem* found = NULL;
        HASH_FIND(hh, hash, &item->id, sizeof(long), found);
        
        if (found) {
            // Объединяем с существующей записью
            if ((item->count > 0 && found->data.count > INT_MAX - item->count) ||
                (item->count < 0 && found->data.count < INT_MIN - item->count)) {
                found->data.count = (item->count > 0) ? INT_MAX : INT_MIN;
            } else {
                found->data.count += item->count;
            }
            found->data.cost += item->cost;
            found->data.primary &= item->primary;
            if (item->mode > found->data.mode) {
                found->data.mode = item->mode;
            }
        } else {
            // Добавляем новую запись
            HashItem* new_item = malloc(sizeof(HashItem));
            new_item->id = item->id;
            new_item->data = *item;
            HASH_ADD(hh, hash, id, sizeof(long), new_item);
        }
    }
    
    // Копируем результаты в выходной массив
    *result_count = HASH_COUNT(hash);
    StatData* result = malloc(*result_count * sizeof(StatData));
    
    size_t idx = 0;
    HashItem *current, *tmp;
    HASH_ITER(hh, hash, current, tmp) {
        result[idx++] = current->data;
        HASH_DEL(hash, current);
        free(current);
    }
    
    return result;
}

#endif