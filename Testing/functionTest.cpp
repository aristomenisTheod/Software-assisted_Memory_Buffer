#include <pthread.h>

#include "DataCaching.hpp"

void simple_check(){
	// Simple checks for Cache and CacheBlock

	lprintf(0, "\n>>>>>>>>      Creating a cache       <<<<<<<<\n");
	Cache_p cache1 = new Cache(0, 4, 1024*1024);
	cache1->draw_cache(true);

	lprintf(0, "\n>>>>>>>>     Assigning new block     <<<<<<<<\n");
	CBlock_p myBlock = cache1->assign_Cblock(false);
	cache1->draw_cache();
	myBlock->draw_block();
	lprintf(0, "\n>>>>>>>>   Adding reader to block    <<<<<<<<\n");
	myBlock->add_reader();
	myBlock->draw_block();
	lprintf(0, "\n>>>>>>>>   Adding writer to block    <<<<<<<<\n");
	myBlock->add_writer();
	myBlock->draw_block();
	lprintf(0, "\n>>>>>>>>   Updating state of block   <<<<<<<<\n");
	myBlock->update_state();
	myBlock->draw_block();
	lprintf(0, "\n>>>>>>>> Removing writer from block  <<<<<<<<\n");
	myBlock->remove_writer();
	myBlock->draw_block();
	lprintf(0, "\n>>>>>>>>   Updating state of block   <<<<<<<<\n");
	myBlock->update_state();
	myBlock->draw_block();
	lprintf(0, "\n>>>>>>>> Removing reader from block  <<<<<<<<\n");
	myBlock->remove_reader();
	myBlock->draw_block();
	lprintf(0, "\n>>>>>>>>   Updating state of block   <<<<<<<<\n");
	myBlock->update_state();
	myBlock->draw_block();
	lprintf(0, "\n>>>>>>>>       Deleting cache        <<<<<<<<\n");
	delete(cache1);
	lprintf(0, "\n>>>>>>>>           SUCCESS           <<<<<<<<\n");
}

void assign_many_blocks(){
	// Add many blocks to cache to check functionality.
	lprintf(0, "\n>>>>>>>>      Creating a cache        <<<<<<<<\n");
	Cache_p cache1 = new Cache(0, 3, 1024*1024);
	cache1->draw_cache(true);

	lprintf(0, "\n>>>>>>>>     Assigning new block 0    <<<<<<<<\n");
	CBlock_p myBlock1 = cache1->assign_Cblock(false);
	lprintf(0, "\n>>>>>>>>     Assigning new block 1    <<<<<<<<\n");
	CBlock_p myBlock2 = cache1->assign_Cblock(false);
	lprintf(0, "\n>>>>>>>>     Assigning new block 2    <<<<<<<<\n");
	CBlock_p myBlock3 = cache1->assign_Cblock(false);

	cache1->draw_cache();

	lprintf(0, "\n>>>>>>>>   Adding writer to block 0   <<<<<<<<\n");
	myBlock1->add_writer();
	lprintf(0, "\n>>>>>>>>   Adding reader to block 2   <<<<<<<<\n");
	myBlock3->add_reader();

	cache1->draw_cache();

	lprintf(0, "\n>>>>>>>>     Assigning new block 3    <<<<<<<<\n");
	CBlock_p myBlock4 = cache1->assign_Cblock(false);

	cache1->draw_cache();

	lprintf(0, "\n>>>>>>>>       Deleting cache        <<<<<<<<\n");
	delete(cache1);
	lprintf(0, "\n>>>>>>>>           SUCCESS           <<<<<<<<\n");

}
/******* Checks with multiple threads ********/

void *task1(void *ptr){
	Cache_p cache = (Cache_p) ptr;
	CBlock_p block = cache->assign_Cblock();
	block->update_state();
	block->set_state(INVALID);
	// lprintf(0, "finished\n");
	return NULL;
}

void single_cache_multiple_threads(int num_threads, int num_blocks){
	Cache_p cache = new Cache(0, num_blocks, 1024*1024);
	pthread_t threads[num_threads];

	for(int i=0; i<num_threads; i++)
		pthread_create(&threads[i], NULL, task1, (void*) cache);

	for(int j=0; j<num_threads; j++)
		pthread_join(threads[j], NULL);
	cache->draw_cache(true, true, false);
	delete cache;
}

int main(int argc, char **argv){
	// simple_check();
	// assign_many_blocks();
	int num_threads = atoi(argv[1]);
	int num_blocks = atoi(argv[2]);
	lprintf(0, "num_threads: %d, num_blocks: %d\n", num_threads, num_blocks);
	single_cache_multiple_threads(num_threads, num_blocks);

	return 0;
}
