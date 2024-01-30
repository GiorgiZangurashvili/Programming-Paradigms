#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const int defaultAllocLen = 10;
void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn)
{
	assert(elemSize > 0); 
	assert( numBuckets > 0 ); 
	assert(hashfn != NULL); 
	assert(comparefn != NULL);
	h->cmp = comparefn;
	h->elemSize = elemSize;
	h->num_buckets = numBuckets;
	h->hash = hashfn;
	h->logLen = 0;
	h->base = malloc(numBuckets * sizeof(vector));
	assert(h->base != NULL);
	for(int i = 0; i < numBuckets; i++){
		VectorNew(&h->base[i], elemSize, freefn, defaultAllocLen);
	}
}

void HashSetDispose(hashset *h)
{
	for(int i = 0; i < h->num_buckets; i++){
		VectorDispose(&h->base[i]);
	}
	free(h->base);
}

int HashSetCount(const hashset *h)
{ 
	return h->logLen;
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData)
{
	assert(mapfn != NULL);
	for(int i = 0; i < h->num_buckets; i++){
		VectorMap(&h->base[i], mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr)
{
	assert(elemAddr != NULL);
	int hash_num = h->hash(elemAddr, h->num_buckets);
	assert(hash_num >= 0 && hash_num < h->num_buckets);
	int position = VectorSearch(&h->base[hash_num], elemAddr, h->cmp, 0, false);
	if(position == -1){
		VectorAppend(&h->base[hash_num], elemAddr);
		h->logLen++;
	}else{
		VectorReplace(&h->base[hash_num], elemAddr, position);
	}
}

void *HashSetLookup(const hashset *h, const void *elemAddr)
{ 
	assert(elemAddr != NULL);
	int hash_num = h->hash(elemAddr, h->num_buckets);
	assert(hash_num >= 0 && hash_num < h->num_buckets);

	int position = VectorSearch(&h->base[hash_num], elemAddr, h->cmp, 0, false);
	if(position == -1){
		return NULL;
	}else{
		return VectorNth(&h->base[hash_num], position);
	}
}