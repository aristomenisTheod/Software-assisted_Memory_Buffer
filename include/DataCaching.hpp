///
/// \author Anastasiadis Petros (panastas@cslab.ece.ntua.gr)
/// \author Theodoridis Aristomenis (theodoridisaristomenis@gmail.com)
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
#define MEM_LIMIT 1024*1024*262

#include<iostream>
#include <string>
#include <mutex>
#include <atomic>

#include "unihelpers.hpp"

enum state{
	INVALID = 0, /// Cache Block is not valid.
	EXCLUSIVE = 1,  /// is being modified locally.
	SHARABLE = 2,  /// is available for sharing only but cannot be modified.
	AVAILABLE = 3, /// Is available with no operations performed or waiting on it.
};
const char* print_state(state in_state);

typedef class Cache* Cache_p;

// A class for each cache block.
typedef class CacheBlock{
	private:
	public:
		int id; // A unique per DevCache id for each block
		std::string Name; // Including it in all classes for potential debugging
		Cache_p Parent;		// Is this needed?
		void** Owner_p; // A pointer to the pointer that is used externally to associate with this block.
		long long Size; // Included here but should be available at parent DevCache (?)

		// Current reads/writes + read/write requests waiting for access.
		std::atomic<int> PendingReaders, PendingWriters;  //Atomic for no lock
		// int PendingReaders, PendingWriters; if std::atomic becomes too annoying, must have block lock to change these
		void* Adrs;
		state State; // The (lazy) current state of the block. Must ALWAYS be <= the actual state
		Event_p Available;
		// int Lock; // I like integers, but maybe consider using a more sophisticated/faster/friendly lock.
		std::mutex Lock;
		//Constructor
		CacheBlock(int id, Cache_p Parent, long long Size);
		//Destructor
		~CacheBlock();

		// Functions
		void draw_block();
		void allocate();
		void add_reader(); // These might or might not be too much since DevCache will have to take part in calling them anyway.
		void add_writer(); // All add/remove should either use atomics or ensure the block is locked.
		void remove_reader();
		void remove_writer();
		void set_owner(void** owner_adrs);
		void reset(bool lockfree);  // Cleans a block to be given to someone else
		state get_state();
		state set_state(state new_state, bool lockfree); // Return prev state
		int update_state(bool lockfree); // Force state check for Cblock, return 1 if state was changed, 0 if same old.
		void lock();
		void unlock();
		bool is_locked();

}* CBlock_p;

/// Device-wise software cache class declaration
typedef class Cache{
	private:
	public:
		int id; // A unique id per cache
		int dev_id; /// Pressumably this should be sufficient for current use cases instead of id, since we use only 1 cache/dev
		std::string Name; // Including it in all classes for potential debugging
		long long Size; // The sum of a cache's CBlock_sizes.
		// int Lock; // I like integers, but maybe consider using a more sophisticated/faster/friendly lock.
		std::mutex Lock;

		int SerialCtr; // Number of blocks currently in cache.
		int BlockNum; // Number of Blocks the cache holds
		long long BlockSize; // Size allocated for each block - in reality it can hold less data
		CBlock_p* Blocks;

		//Constructor
		Cache(int dev_id, long long block_num, long long block_size);
		//Destructor
		~Cache();

		// Functions
		void draw_cache(bool print_blocks);
		CBlock_p assign_Cblock();

		void lock();
		void unlock();
		bool is_locked();

#ifdef STEST
		double timer; // Keeps total time spend in cache operations-code
#endif

}* Cache_p;

typedef struct Cache_info_wrap{
	short dev_id;
	int BlockIdx;
	int lock_flag;
}* CacheWrap_p;


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

	// Constructor
	LinkedList();
	// Destructor
	~LinkedList();

	// Functions
	void invalidate(Cache_p cache, Node_LL* node);
	void push_back(Cache_p cache, int idx);
	Node_LL* start_iterration(Cache_p cache);
	Node_LL* next_in_line(Cache_p cache);
	int remove(Cache_p cache, Node_LL* node);
	void put_first(Cache_p cache, Node_LL* node);
	void put_last(Cache_p cache, Node_LL* node);
	void lock();
	void unlock();
	bool is_locked();
};
#endif


// FIXME: void CoCoPeLiaRequestMaxBuffer(short dev_id, long long block_num, long long block_size, long long bufsize_limit); -> Cache constructor + Cache.allocate()

// FIXME: void* CacheAsignBlock(short dev_id, void* TilePtr, short TileDim); -> Cache.assignCblock()
// FIXME: void* CacheUpdateAsignBlock(short dev_id, void* TilePtr, short TileDim); merge

/// FIXME: All these should be transfered in the corresponding functions of Cblock
// state CacheUpdateBlockState(short dev_id, int BlockIdx);
// state CacheSetBlockState(short dev_id, int BlockIdx, state new_state);
// state CacheGetBlockStateNoLock(short dev_id, int BlockIdx);
//
// void CacheInvalidate(void* wrapped_CacheWrap_p);
//
// void CacheStartRead(void* wrapped_CacheWrap_p);
// void CacheEndRead(void* wrapped_CacheWrap_p);
// //void CacheStartWrite(void* wrapped_CacheWrap_p);
// //void CacheEndWrite(void* wrapped_CacheWrap_p);
//
// void CacheStartFetch(void* wrapped_CacheWrap_p);
// //void CacheEndFetch(void* wrapped_CacheWrap_p);
// void CacheEndFetchStartRead(void* wrapped_CacheWrap_p);
// void CacheEndFetchStartWrite(void* wrapped_CacheWrap_p);
// FIXME: void CacheGetLock(void*);
// FIXME: void CacheReleaseLock(void*);

#if defined(NAIVE)
int CacheSelectBlockToRemove_naive(Cache_p cache);
#elif defined(FIFO)
Node_LL* CacheSelectBlockToRemove_fifo(Cache_p cache);
#elif defined(MRU) || defined(LRU)
Node_LL* CacheSelectBlockToRemove_mru_lru(Cache_p cache);
#endif

// FIXME: void CachePrint(short dev_id); -> Cache.print();

#endif
