/// \author Anastasiadis Petros (panastas@cslab.ece.ntua.gr)
///
/// \brief The header containing the caching functions for data scheduling and management in heterogeneous multi-device systems.
///

#ifndef DATACACHNING_H
#define DATACACHNING_H

// Protocol for block removal in cache.
 // "N": Naive
 // "F": FIFO
 // "M": MRU
 // "L": LRU

#if CACHE_SCHEDULING_POLICY=='N'
	#define NAIVE
#elif CACHE_SCHEDULING_POLICY=='F'
	#define FIFO
#elif CACHE_SCHEDULING_POLICY=='M'
	#define MRU
#elif CACHE_SCHEDULING_POLICY=='L'
	#define LRU
#endif
#define MEM_LIMIT 1024*1024*262*3

#include<iostream>
#include <string>
#include <mutex>

#include "unihelpers.hpp"
#include "Subkernel.hpp"

enum state{
	EMPTY = 0, /// Cache Block is empty.
	AVAILABLE = 1, /// exists in location with no (current) operations performed on it.
	R = 2,  /// is being read/used in operation.
	W = 3,  /// is being modified (or transefered).
};

const char* print_state(state in_state);

typedef struct pending_action_list{
	Event* event_start, *event_end;
	state effect;
	struct pending_action_list* next;
}* pending_events_p;

int pending_events_free(pending_events_p target);

/* Device-wise software cache struct declaration */
typedef struct DevCache_str{
	short dev_id;
	void * gpu_mem_buf;
	long long mem_buf_sz;
	int BlockNum, serialCtr;
	long long BlockSize;
	state* BlockState;
	short* BlockCurrentTileDim;
	void** BlockCurrentTilePtr;
	pending_events_p *BlockPendingEvents;
}* DevCachePtr;

#if defined(FIFO) || defined(MRU) || defined(LRU)
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

	void invalidate(short dev_id, Node_LL* node);

	void push_back(short dev_id, int idx);

	Node_LL* start_iterration(short dev_id);

	Node_LL* next_in_line(short dev_id);

	int remove(short dev_id, Node_LL* node);

	void put_first(short dev_id, Node_LL* node);

	void put_last(short dev_id, Node_LL* node);
};
#endif

long long CoCoPeLiaDevBuffSz(kernel_pthread_wrap_p subkernel_data);
DevCachePtr CoCoPeLiaGlobufInit(short dev_id);
void CoCoPeLiaRequestBuffer(kernel_pthread_wrap_p subkernel_data, long long bufsize_limit, long long block_id);

void* CoCacheAsignBlock(short dev_id, void* TilePtr, short TileDim);

#if defined(NAIVE)
int CoCacheSelectBlockToRemove_naive(short dev_id);
#elif defined(FIFO)
Node_LL* CoCacheSelectBlockToRemove_fifo(short dev_id);
#elif defined(MRU) || defined(LRU)
Node_LL* CoCacheSelectBlockToRemove_mru_lru(short dev_id);
#endif

void* CoCacheUpdateAsignBlock(short dev_id, void* TilePtr, short TileDim);

void CoCoPeLiaUnlockCache(short dev_id);

state CoCacheUpdateBlockState(short dev_id, int BlockIdx);

void CoCacheAddPendingEvent(short dev_id, Event* e_start, Event* e_end, int BlockIdx, state effect);

long long CoCoGetBlockSize(short dev_id);

///Invalidates the GPU-allocated cache buffer metadata at target device
void CoCoPeLiaDevCacheInvalidate(kernel_pthread_wrap_p subkernel_data);

#endif
