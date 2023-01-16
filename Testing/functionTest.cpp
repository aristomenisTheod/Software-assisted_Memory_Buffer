#include <pthread.h>
#include <unistd.h>

#include "DataCaching.hpp"

void simple_check(){
	// Simple checks for Cache and CacheBlock

	lprintf(0, "\n>>>>>>>>      Creating a cache       <<<<<<<<\n");
	Cache_p cache1 = new Cache(0, 4, 1024*1024);
	cache1->draw_cache(true);

	lprintf(0, "\n>>>>>>>>     Assigning new block     <<<<<<<<\n");
	CBlock_p myBlock = cache1->assign_Cblock(SHARABLE, false);
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
	CBlock_p myBlock1 = cache1->assign_Cblock(SHARABLE, false);
	lprintf(0, "\n>>>>>>>>     Assigning new block 1    <<<<<<<<\n");
	CBlock_p myBlock2 = cache1->assign_Cblock(SHARABLE, false);
	lprintf(0, "\n>>>>>>>>     Assigning new block 2    <<<<<<<<\n");
	CBlock_p myBlock3 = cache1->assign_Cblock(SHARABLE, false);

	cache1->draw_cache();

	lprintf(0, "\n>>>>>>>>   Adding writer to block 0   <<<<<<<<\n");
	myBlock1->add_writer();
	lprintf(0, "\n>>>>>>>>   Adding reader to block 2   <<<<<<<<\n");
	myBlock3->add_reader();

	cache1->draw_cache();

	lprintf(0, "\n>>>>>>>>     Assigning new block 3    <<<<<<<<\n");
	CBlock_p myBlock4 = cache1->assign_Cblock(SHARABLE, false);

	cache1->draw_cache();

	lprintf(0, "\n>>>>>>>>       Deleting cache        <<<<<<<<\n");
	delete(cache1);
	lprintf(0, "\n>>>>>>>>           SUCCESS           <<<<<<<<\n");

}
/******* Checks with multiple threads ********/

void *task1(void *ptr){
	Cache_p cache = (Cache_p) ptr;
	CBlock_p block1 = cache->assign_Cblock(SHARABLE, false);
	// CBlock_p block1 = cache->assign_Cblock(AVAILABLE, false);

	// sleep(1);
	block1->add_writer();
	block1->remove_reader();
	block1->add_writer();
	block1->add_reader();
	block1->remove_writer();
	block1->remove_writer();
	block1->set_state(SHARABLE);
	block1->remove_reader();
	block1->set_state(INVALID);
	// error("askfjhd");
	CBlock_p block2 = cache->assign_Cblock(SHARABLE, false);
	// error("yeap changing dat sit\n");
	// // sleep(1);
	block2->add_reader();
	block2->add_writer();
	block2->add_writer();
	block2->remove_writer();
	block2->remove_reader();
	block2->remove_reader();
	block2->remove_writer();
	block2->set_state(INVALID);
	lprintf(0, "finished\n");
	return NULL;
}

void *task2(void *ptr){
	Cache_p cache = (Cache_p) ptr;
	CBlock_p block1 = cache->assign_Cblock(EXCLUSIVE, false);
	// sleep(1);
	block1->add_reader();
	block1->add_reader();
	block1->add_writer();
	block1->add_writer();
	block1->remove_writer();
	block1->remove_writer();
	block1->remove_writer();
	block1->set_state(SHARABLE);
	block1->remove_reader();
	block1->remove_reader();
	CBlock_p block2 = cache->assign_Cblock(EXCLUSIVE, false);
	block2->add_reader();

	// sleep(1);
	block2->add_reader();
	block2->add_writer();
	block2->add_writer();
	block2->remove_writer();
	block2->remove_writer();
	block2->remove_writer();
	block2->set_state(SHARABLE);
	block2->remove_reader();
	block2->remove_reader();

	return NULL;
}

void *task3(void *ptr){
	Cache_p cache = (Cache_p) ptr;
	CBlock_p block1 = cache->assign_Cblock(SHARABLE, false);
	// sleep(1);
	block1->add_reader();
	block1->add_reader();
	block1->add_writer();
	block1->add_writer();
	block1->remove_writer();
	block1->add_reader();
	block1->remove_reader();
	block1->remove_writer();
	block1->set_state(SHARABLE);
	block1->remove_reader();
	block1->remove_reader();
	block1->remove_reader();

	CBlock_p block3 = cache->assign_Cblock(AVAILABLE, false);
	block3->set_state(INVALID);
	// sleep(1);
	CBlock_p block2 = cache->assign_Cblock(EXCLUSIVE, false);
	block2->add_reader();
	block2->add_reader();
	block2->add_writer();
	block2->add_writer();
	block2->remove_writer();
	block2->remove_writer();
	block2->remove_writer();
	block2->set_state(SHARABLE);
	block2->remove_reader();
	block2->remove_reader();

	return NULL;
}

void *task1_father(void *ptr){
	char **args = (char**) ptr;
	int num_threads = *((int*) args[0]);
	Cache_p cache = (Cache_p) args[1];
	// Cache_p cache = new Cache(0, num_blocks, 1024*1024);
	pthread_t threads[num_threads];

	for(int i=0; i<num_threads; i++)
		pthread_create(&threads[i], NULL, task1 , (void*) cache);

	for(int j=0; j<num_threads; j++)
		pthread_join(threads[j], NULL);

	return NULL;
}

void *task2_father(void *ptr){
	char **args = (char**) ptr;
	int num_threads = *((int*) args[0]);
	Cache_p cache = (Cache_p) args[1];
	// Cache_p cache = new Cache(0, num_blocks, 1024*1024);
	pthread_t threads[num_threads];

	for(int i=0; i<num_threads; i++)
		pthread_create(&threads[i], NULL, task2 , (void*) cache);

	for(int j=0; j<num_threads; j++)
		pthread_join(threads[j], NULL);

	return NULL;
}

void *task3_father(void *ptr){
	char **args = (char**) ptr;
	int num_threads = *((int*) args[0]);
	Cache_p cache = (Cache_p) args[1];
	// Cache_p cache = new Cache(0, num_blocks, 1024*1024);
	pthread_t threads[num_threads];

	for(int i=0; i<num_threads; i++)
		pthread_create(&threads[i], NULL, task3 , (void*) cache);

	for(int j=0; j<num_threads; j++)
		pthread_join(threads[j], NULL);

	return NULL;
}

void *multiple_assigns_sh(void *ptr){
	Cache_p cache = (Cache_p) ptr;

	CBlock_p block;

	for(int i = 0; i < 100; i++) {
		block = cache->assign_Cblock(SHARABLE, false);
		block->remove_reader();
	}

	return NULL;
}

void *multiple_assigns_ex(void *ptr){
	Cache_p cache = (Cache_p) ptr;

	CBlock_p block;

	for(int i = 0; i < 100; i++) {
		block = cache->assign_Cblock(EXCLUSIVE, false);
		block->remove_writer();
		block->set_state(INVALID);
	}

	return NULL;
}

void *single_cache_multiple_threads(void *ptr){
	char **argum = (char**) ptr;
	int num_threads = *((int*) argum[0]);
	int num_blocks = *((int*) argum[1]);
	Cache_p cache = new Cache(0, num_blocks, 1024*1024);
	pthread_t threads[5];

	char *args[2];
	args[0] = (char*) &num_threads;
	args[1] = (char*) cache;

	pthread_create(&threads[0], NULL, task1_father, (void*) args);
	pthread_create(&threads[1], NULL, task2_father, (void*) args);
	pthread_create(&threads[2], NULL, task3_father, (void*) args);
	pthread_create(&threads[3], NULL, multiple_assigns_sh, (void*) cache);
	pthread_create(&threads[4], NULL, multiple_assigns_ex, (void*) cache);

	for(int i=0; i<5; i++)
		pthread_join(threads[i], NULL);

	cache->reset();
	cache->draw_cache(true, true, false);
	delete cache;

	return NULL;
}

void multiple_caches(int num_caches, int num_threads, int num_blocks){
	pthread_t caches[num_caches];
	char *args[2];
	args[0] = (char*) &num_threads;
	args[1] = (char*) &num_blocks;

	for(int i=0; i<num_caches; i++)
		pthread_create(&caches[i], NULL, single_cache_multiple_threads, (void*) args);

	for(int j=0; j<num_caches; j++)
		pthread_join(caches[j], NULL);


}

void *get_block(void *ptr){
	Cache_p cache = (Cache_p) ptr;
	CBlock_p block;

	block = cache->assign_Cblock(SHARABLE, false);
	block->remove_reader();
	block = cache->assign_Cblock(SHARABLE, false);
	block->remove_reader();
	block = cache->assign_Cblock(SHARABLE, false);
	block->remove_reader();

	return NULL;

}

void last_block_check(){
	Cache_p cache = new Cache(0, 4, 1024);
	CBlock_p block1, block2, block3, block4;

	block1 = cache->assign_Cblock(EXCLUSIVE, false);
	block2 = cache->assign_Cblock(EXCLUSIVE, false);
	block3 = cache->assign_Cblock(EXCLUSIVE, false);
	block3->remove_writer();
	block4 = cache->assign_Cblock(EXCLUSIVE, false);
 	block4->remove_writer();
	// cache->draw_cache(true, true, false);
	pthread_t threads[5];

	pthread_create(&threads[0], NULL, get_block, (void*) cache);
	pthread_create(&threads[1], NULL, get_block, (void*) cache);
	pthread_create(&threads[2], NULL, get_block, (void*) cache);
	pthread_create(&threads[3], NULL, get_block, (void*) cache);
	pthread_create(&threads[4], NULL, get_block, (void*) cache);

	for(int i=0; i<5; i++)
		pthread_join(threads[i], NULL);


}

int main(int argc, char **argv){
	// simple_check();
	// assign_many_blocks();
	// if(argc != 4){
	// 	printf("Usage: functionTest <num_caches> <task_threads> <num_blocks>\n");
	// 	exit(0);
	// }
	// int num_caches = atoi(argv[1]);
	// int num_threads = atoi(argv[2]);
	// int num_blocks = atoi(argv[3]);
	// lprintf(0, "num_caches: %d, num_threads1: %d, num_blocks: %d\n", num_caches, num_threads, num_blocks);
	// multiple_caches(num_caches, num_threads, num_blocks);
	last_block_check();
	return 0;
}
