/// \author Anastasiadis Petros (panastas@cslab.ece.ntua.gr)
///
/// \brief The header containing the caching functions for data scheduling and management in heterogeneous multi-device systems.
///

#ifndef DATACACHNING_H
#define DATACACHNING_H

// Protocol for block removal in cache.
// 0. Naive
// 1. FIFO
// 2. MRU
// 3. LRU
// #define CACHE_SCHEDULING_POLICY 3
#define MEM_LIMIT 1024*1024*262

#include<iostream>
#include <string>
#include <mutex>
#include <atomic>

#include "unihelpers.hpp"
#include "Subkernel.hpp"

enum state{
	EMPTY = 0, /// Cache Block is empty.
	FETCHING = 1, /// Is being transfered TO cache
	AVAILABLE = 2, /// exists in location with no (current) operations performed on it.
	R = 3,  /// is being read/used in operation.
	EXCLUSIVE = 4,  /// is being modified or up-to-date only in this cache.
};

const char* print_state(state in_state);

typedef struct Cache_info_wrap{
	short dev_id;
	int BlockIdx;
	int lock_flag;
}* CacheWrap_p;

/* Device-wise software cache struct declaration */
typedef struct DevCache_str{
	short dev_id;
	void * gpu_mem_buf;
	long long mem_buf_sz;
	int BlockNum, serialCtr;
	long long BlockSize;
	int* BlockReaders;
	state* BlockState;
	short* BlockCurrentTileDim;
	void** BlockCurrentTilePtr;
}* DevCachePtr;

#if CACHE_SCHEDULING_POLICY==1 || CACHE_SCHEDULING_POLICY==2 || CACHE_SCHEDULING_POLICY==3
// Node for linked list.
struct Node_LL{
	Node_LL* next;
	Node_LL* previous;
	int idx;
	bool valid;
};

class LinkedList{
private:
	Node_LL* iter;
public:
	Node_LL* start;
	Node_LL* end;
	int length;
	std::mutex lock_ll;

	LinkedList();

	~LinkedList();

	void invalidate(Node_LL* node);

	void push_back(int idx);

	Node_LL* start_iterration();

	Node_LL* next_in_line();

	int remove(Node_LL* node);

	void put_first(Node_LL* node);

	void put_last(Node_LL* node);
};
#endif

long long CoCoPeLiaDevBuffSz(kernel_pthread_wrap_p subkernel_data);
DevCachePtr CoCoPeLiaGlobufInit(short dev_id);

// Return the maximum allowed buffer from i) bufsize_limit and ii) device memory limitations
void CoCoPeLiaRequestMaxBuffer(short dev_id, long long block_num, long long block_size, long long bufsize_limit);

void CoCoPeLiaRequestBuffer(kernel_pthread_wrap_p subkernel_data, long long bufsize_limit, long long block_id);

void* CacheAsignBlock(short dev_id, void* TilePtr, short TileDim);

#if CACHE_SCHEDULING_POLICY==0
int CacheSelectBlockToRemove_naive(short dev_id);
#elif CACHE_SCHEDULING_POLICY==1
Node_LL* CacheSelectBlockToRemove_fifo(short dev_id);
#elif CACHE_SCHEDULING_POLICY==2 || CACHE_SCHEDULING_POLICY==3
Node_LL* CacheSelectBlockToRemove_mru_lru(short dev_id);
#endif

void* CacheUpdateAsignBlock(short dev_id, void* TilePtr, short TileDim);

///Invalidates the GPU-allocated cache buffer metadata at target device
void CoCoPeLiaDevCacheInvalidate(kernel_pthread_wrap_p subkernel_data);

void CacheGetLock(void*);
void CacheReleaseLock(void*);

state CacheUpdateBlockState(short dev_id, int BlockIdx);
state CacheSetBlockState(short dev_id, int BlockIdx, state new_state);
state CacheGetBlockStateNoLock(short dev_id, int BlockIdx);

void CacheInvalidate(void* wrapped_CacheWrap_p);

void CacheStartRead(void* wrapped_CacheWrap_p);
void CacheEndRead(void* wrapped_CacheWrap_p);
//void CacheStartWrite(void* wrapped_CacheWrap_p);
//void CacheEndWrite(void* wrapped_CacheWrap_p);

void CacheStartFetch(void* wrapped_CacheWrap_p);
//void CacheEndFetch(void* wrapped_CacheWrap_p);
void CacheEndFetchStartRead(void* wrapped_CacheWrap_p);
void CacheEndFetchStartWrite(void* wrapped_CacheWrap_p);

void CachePrint(short dev_id);

#ifdef STEST
double CacheGetTimer(short dev_id);
#endif

#endif
