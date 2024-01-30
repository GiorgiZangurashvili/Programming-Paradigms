#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

static const int defaultAllocLen = 4;
void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation)
{
    assert(elemSize > 0 && initialAllocation >= 0);
    v->elemSize = elemSize;
    v->freefn = freeFn;
    if(initialAllocation != 0){
        v->allocLen = initialAllocation;
    }else{
        v->allocLen = defaultAllocLen;
    }
    v->initAlloc = v->allocLen;
    v->logLen = 0;
    v->base = malloc(elemSize * v->allocLen);
    assert(v->base != NULL);    
}

void VectorDispose(vector *v)
{
    if(v->freefn != NULL){
        for(int i = 0; i < v->logLen; i++){
            v->freefn((char*)v->base + i * v->elemSize);
        }
    }
    free(v->base);
}

int VectorLength(const vector *v)
{
    return v->logLen;
}

void *VectorNth(const vector *v, int position)
{
    assert(position <= (v->logLen - 1) && position >= 0);
    return (char*)v->base + position * v->elemSize;
}

void VectorReplace(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0 && position <= (v->logLen - 1));
    if(v->freefn != NULL){
        v->freefn(VectorNth(v, position));
    }
    memcpy((char*)v->base + position * v->elemSize, elemAddr, v->elemSize);
}

static void VectorAllocMore(vector *v){
    v->allocLen += v->initAlloc;
    v->base = realloc(v->base, v->allocLen * v->elemSize);
    assert(v->base != NULL);
}

void VectorInsert(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0 && position <= v->logLen);
    if(v->logLen == v->allocLen){
        VectorAllocMore(v);
    }
    
    memmove((char*)v->base + (position + 1) * v->elemSize, (char*)v->base + position * v->elemSize, (v->logLen - position) * v->elemSize);
    memcpy((char*)v->base + position * v->elemSize, elemAddr, v->elemSize);
    v->logLen++;
}

void VectorAppend(vector *v, const void *elemAddr)
{
    if(v->allocLen == v->logLen){
        VectorAllocMore(v);
    }
    memcpy((char*)v->base + v->logLen * v->elemSize, elemAddr, v->elemSize);
    v->logLen++;
}

void VectorDelete(vector *v, int position)
{
    assert(position >= 0 && position <= (v->logLen - 1));
    if(v->freefn != NULL){
        v->freefn(VectorNth(v, position));
    }
    if(position < v->logLen - 1){
        memmove(VectorNth(v, position), VectorNth(v, position + 1), v->elemSize * (v->logLen - position - 1));
    }
    v->logLen--;
}

void VectorSort(vector *v, VectorCompareFunction compare)
{
    assert(compare != NULL);
    qsort(v->base, v->logLen, v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData)
{
    assert(mapFn != NULL);
    for(int i = 0; i < v->logLen; i++){
        mapFn(VectorNth(v, i), auxData);
    }
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted)
{ 
    assert(startIndex >= 0 && startIndex <= v->logLen && key != NULL && searchFn != NULL);

    if(v->logLen == 0) return kNotFound;
    char* result;
    size_t length = v->logLen - startIndex;
    if(isSorted){
        result = bsearch(key, (const void*)((char*)v->base + startIndex * v->elemSize), length, v->elemSize, searchFn);
    }else{
        result = lfind(key, (const void*)((char*)v->base + startIndex * v->elemSize), &length, v->elemSize, searchFn);
    }
    if(result == NULL){
        return kNotFound;
    }
    return (result - (char*)v->base) / v->elemSize;
}