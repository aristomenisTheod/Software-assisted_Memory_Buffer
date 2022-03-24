#include "DataCaching.hpp"

void simple_check(){
	// Simple checks for Cache and CacheBlock

	lprintf(0, "\n>>>>>>>>      Creating a cache       <<<<<<<<\n");
	Cache_p cache1 = new Cache(0, 4, 1024*1024);
	cache1->draw_cache(true);

	lprintf(0, "\n>>>>>>>>     Assigning new block     <<<<<<<<\n");
	CBlock_p myBlock = cache1->assign_Cblock();
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

	lprintf(0, "\n>>>>>>>>     Assigning new block 1    <<<<<<<<\n");
	CBlock_p myBlock1 = cache1->assign_Cblock();
	lprintf(0, "\n>>>>>>>>     Assigning new block 2    <<<<<<<<\n");
	CBlock_p myBlock2 = cache1->assign_Cblock();
	lprintf(0, "\n>>>>>>>>     Assigning new block 3    <<<<<<<<\n");
	CBlock_p myBlock3 = cache1->assign_Cblock();

	cache1->draw_cache();

	lprintf(0, "\n>>>>>>>>   Adding writer to block 1   <<<<<<<<\n");
	myBlock1->add_writer();
	lprintf(0, "\n>>>>>>>>   Adding reader to block 3   <<<<<<<<\n");
	myBlock3->add_reader();

	cache1->draw_cache();

	lprintf(0, "\n>>>>>>>>     Assigning new block 4    <<<<<<<<\n");
	CBlock_p myBlock4 = cache1->assign_Cblock();

	cache1->draw_cache();

	lprintf(0, "\n>>>>>>>>       Deleting cache        <<<<<<<<\n");
	delete(cache1);
	lprintf(0, "\n>>>>>>>>           SUCCESS           <<<<<<<<\n");

}

int main(){
	simple_check();
	assign_many_blocks();


	return 0;
}
