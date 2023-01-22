#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Memory{
    long int memoryAddress;
    char operation;
    long int tag;
    long int blockOffset;
    long int setIdentifier;
    int linesPerSet;
    int blockSize;
    int numberOfSets;

}Memory;

typedef struct Cache{
    long int tag;
    int valid;
}Cache;

typedef struct Tracker{
    int memoryReads;
    int memoryWrites;
    int cacheHits;
    int cacheMisses;
}Tracker;


int mathLog(int change, int count);
void read(Memory memory, Cache* cache, char* replacement, Tracker* tracker, int* ageArray, int boolPrefetch);
void write(Memory memory, Cache* cache, char* replacement, Tracker* tracker, int* ageArray, int boolPrefetch);
void fifo(Memory memory, Cache* cache, int* ageArray);
void prefetch(Memory memory, Cache* cache, char* replacement, Tracker* tracker, int* ageArray);
void fifo2(long int tag, long int setIdentifier, Cache* cache, int* ageArray, int linesPerSet);

Cache** cacheArray2;

int main(int argc, char* argv[]) {
    FILE *trace;
    int numberOfSets, linesInSet, n, boolCache, boolBlock, boolN;
    int boolAssocN = 0;
    char *p;
    long cSize = strtol(argv[1], &p, 10);
    int cacheSize = cSize;
    char* associativity = argv[2];
    char* replacement = argv[3];
    long bSize = strtol(argv[4], &p, 10);
    int blockSize = bSize;
    trace = fopen(argv[5], "r");

    // Check if cacheSize and blockSize are powers of 2
    if(cacheSize != 0 && (cacheSize & (cacheSize - 1)) == 0){
        boolCache = 1;
    }
    else{
        boolCache = 0;
    }
    if(blockSize != 0 && (blockSize & (blockSize - 1)) == 0){
        boolBlock = 1;
    }
    else{
        boolBlock = 0;
    }

    // check associativity
    int stringLength = strlen(associativity);
    if(strcmp(associativity, "direct") == 0){
        numberOfSets = cacheSize/blockSize;
        linesInSet = 1;
    }
    else if(strcmp(associativity, "assoc") == 0){
        numberOfSets = 1;
        linesInSet = cacheSize/blockSize;
    }
    else{

        if(stringLength < 5){
            return 1;
        }else{
            char* nums = malloc(sizeof(char)*(stringLength-5));
            for(int i = 0; i < stringLength-6; i++) {
                nums[i] = associativity[i+6];
            }
            n = atoi(nums);
            free(nums);
        }

        numberOfSets = cacheSize/(blockSize*n);
        linesInSet = n;
        boolAssocN = 1;
        // check if n is power of 2
        if(n != 0 && (n & (n - 1)) == 0){
            boolN = 1;
        }
        else{
            return 1;
        }

    }

    if(boolCache != 1 && boolBlock != 1){
        return 1;
    }

    if(boolAssocN == 1 && boolN == 0){
        return 1;
    }


    // getting bits in order to calculate tags
    int blockCount = 0;
    int indexCount = 0;
    int offsetBits = mathLog(blockSize, blockCount);
    int setIndexBits = mathLog(numberOfSets, indexCount);

    long int memoryAddress;
    char operation;
    int count = 0;
    for(int j = 0; fscanf(trace, "%*x: %c %lx", &operation, &memoryAddress) == 2; j++){
        count++;
    }

    Memory* memoryArray = malloc(sizeof(Memory)*count);
    fseek(trace, 0, SEEK_SET);
    for(int i = 0; fscanf(trace, "%*x: %c %lx", &operation, &memoryAddress) == 2; i++){
        long int blockOffset = memoryAddress >> offsetBits;
        long int setIdentifier = blockOffset & ((1 << setIndexBits) - 1);
        long int tag = blockOffset >> setIndexBits;
        Memory memory;
        memory.memoryAddress = memoryAddress;
        memory.setIdentifier = setIdentifier;
        memory.operation = operation;
        memory.blockOffset = blockOffset;
        memory.tag = tag;
        memory.numberOfSets = numberOfSets;
        memory.linesPerSet = linesInSet;
        memory.blockSize = blockSize;
        memoryArray[i] = memory;

    }
    // Both cache arrays
    Cache** cacheArray = malloc(sizeof(Cache)*numberOfSets);
    for(int i = 0; i < numberOfSets; i++){
        cacheArray[i] = malloc(sizeof(Cache)*linesInSet);
    }

    for(int i = 0; i < numberOfSets; i++){
        for(int j = 0; j < linesInSet; j++){
            cacheArray[i][j].valid = 0;
            cacheArray[i][j].tag = -1;
        }
    }

    cacheArray2 = malloc(sizeof(Cache)*numberOfSets);
    for(int i = 0; i < numberOfSets; i++){
        cacheArray2[i] = malloc(sizeof(Cache)*linesInSet);
    }

    for(int i = 0; i < numberOfSets; i++){
        for(int j = 0; j < linesInSet; j++){
            cacheArray2[i][j].valid = 0;
            cacheArray2[i][j].tag = -1;
        }
    }

    // Both age arrays
    int* ageArray = malloc(sizeof(int)*numberOfSets);
    for(int i = 0; i < numberOfSets; i++){
        ageArray[i] = 0;
    }

    int* ageArray2 = malloc(sizeof(int)*numberOfSets);
    for(int i = 0; i < numberOfSets; i++){
        ageArray2[i] = 0;
    }

    Tracker* noPrefetch = malloc(sizeof(Tracker));
    Tracker* prefetch = malloc(sizeof(Tracker));
    noPrefetch->memoryReads = 0;
    noPrefetch->memoryWrites = 0;
    noPrefetch->cacheMisses = 0;
    noPrefetch->cacheHits = 0;
    prefetch->memoryReads = 0;
    prefetch->memoryWrites = 0;
    prefetch->cacheMisses = 0;
    prefetch->cacheHits = 0;
    for(int i = 0; i < count; i++){
        if(memoryArray[i].operation == 'R'){
            read(memoryArray[i], cacheArray[memoryArray[i].setIdentifier], replacement, noPrefetch, ageArray, 0);
            read(memoryArray[i], cacheArray2[memoryArray[i].setIdentifier], replacement, prefetch, ageArray2, 1);


        }
        else if(memoryArray[i].operation == 'W'){
            write(memoryArray[i], cacheArray[memoryArray[i].setIdentifier], replacement, noPrefetch, ageArray, 0);
            write(memoryArray[i], cacheArray2[memoryArray[i].setIdentifier], replacement, prefetch, ageArray2, 1);
        }
    }

    printf("Prefetch 0\n");
    printf("Memory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\n", noPrefetch->memoryReads, noPrefetch->memoryWrites, noPrefetch->cacheHits, noPrefetch->cacheMisses);
    printf("Prefetch 1\n");
    printf("Memory reads: %d\nMemory writes: %d\nCache hits: %d\nCache misses: %d\n", prefetch->memoryReads, prefetch->memoryWrites, prefetch->cacheHits, prefetch->cacheMisses);

    free(ageArray);
    free(ageArray2);
    free(noPrefetch);
    free(prefetch);
    free(memoryArray);

    for(int i = 0; i < numberOfSets; i++){
        free(cacheArray[i]);
    }
    for(int i = 0; i < numberOfSets; i++){
        free(cacheArray2[i]);
    }
    free(cacheArray);
    free(cacheArray2);


    fclose(trace);
    return 0;
}
void prefetch(Memory memory, Cache* cache, char* replacement, Tracker* tracker, int* ageArray){
    long int newMem = memory.memoryAddress+memory.blockSize;
    int blockCount = 0;
    int indexCount = 0;
    int offsetBits = mathLog(memory.blockSize, blockCount);
    int setIndexBits = mathLog(memory.numberOfSets, indexCount);
    long int blockOffset = newMem >> offsetBits;
    long int setIdentifier = blockOffset & ((1 << setIndexBits) - 1);
    long int tag = blockOffset >> setIndexBits;
    int boolFound = 0;

    for(int i = 0; i < memory.linesPerSet; i++){
        if(cacheArray2[setIdentifier][i].valid == 0){
            boolFound = 1;
            tracker->memoryReads++;
            cacheArray2[setIdentifier][i].valid = 1;
            cacheArray2[setIdentifier][i].tag = tag;
            break;
        }
        else if(cacheArray2[setIdentifier][i].tag == tag){
            boolFound = 1;
            break;
        }
    }
    if(boolFound == 0){
        tracker->memoryReads++;
        if(strcmp(replacement, "fifo") == 0){
            fifo2(tag, setIdentifier, cacheArray2[setIdentifier], ageArray, memory.linesPerSet);
        }
    }

}
void read(Memory memory, Cache* cache, char* replacement, Tracker* tracker, int* ageArray, int boolPrefetch){
    int boolFound = 0;

    for(int i = 0; i < memory.linesPerSet; i++){
        if(cache[i].valid == 0){
            boolFound = 1;
            tracker->cacheMisses++;
            tracker->memoryReads++;
            cache[i].valid = 1;
            cache[i].tag = memory.tag;
            if(boolPrefetch == 1){
                prefetch(memory, cache, replacement, tracker, ageArray);
            }
            break;
        }
        else if(cache[i].tag == memory.tag){
            boolFound = 1;
            cache[i].valid = 1;
            tracker->cacheHits++;
            break;
        }

    }
    if(boolFound == 0){
        tracker->cacheMisses++;
        tracker->memoryReads++;
        if(strcmp(replacement, "fifo") == 0){
            fifo(memory, cache, ageArray);
            if(boolPrefetch == 1){
                prefetch(memory, cache, replacement, tracker, ageArray);
            }
        }
    }


}

void write(Memory memory, Cache* cache, char* replacement, Tracker* tracker, int* ageArray, int boolPrefetch){
    int boolFound = 0;
    for(int i = 0; i < memory.linesPerSet; i++){
        if(cache[i].valid == 0){
            boolFound = 1;
            tracker->cacheMisses++;
            tracker->memoryReads++;
            cache[i].valid = 1;
            cache[i].tag = memory.tag;
            tracker->memoryWrites++;
            if(boolPrefetch == 1){
                prefetch(memory, cache, replacement, tracker, ageArray);
            }

            break;
        }
        else if(cache[i].tag == memory.tag){
            boolFound = 1;
            cache[i].valid = 1;
            tracker->cacheHits++;
            tracker->memoryWrites++;
            break;
        }
    }
    if(boolFound == 0){
        if(strcmp(replacement, "fifo") == 0){
            tracker->cacheMisses++;
            tracker->memoryReads++;
            fifo(memory, cache, ageArray);
            tracker->memoryWrites++;
            if(boolPrefetch == 1){
                prefetch(memory, cache, replacement, tracker, ageArray);
            }
        }
    }
}

void fifo(Memory memory, Cache* cache, int* ageArray){

    cache[ageArray[memory.setIdentifier]].valid = 1;
    cache[ageArray[memory.setIdentifier]].tag = memory.tag;
    ageArray[memory.setIdentifier]++;

    if(ageArray[memory.setIdentifier] == memory.linesPerSet){
        ageArray[memory.setIdentifier] = 0;
    }
}
void fifo2(long int tag, long int setIdentifier, Cache* cache, int* ageArray, int linesPerSet){
    cache[ageArray[setIdentifier]].valid = 1;
    cache[ageArray[setIdentifier]].tag = tag;
    ageArray[setIdentifier]++;

    if(ageArray[setIdentifier] == linesPerSet){
        ageArray[setIdentifier] = 0;
    }
}


int mathLog(int change, int count){
    if(change == 1){
        return count;
    }
    else{
        change /= 2;
        count++;
        return mathLog(change, count);
    }
}