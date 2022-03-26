///
/// \author Anastasiadis Petros (panastas@cslab.ece.ntua.gr)
/// \author Theodoridis Aristomenis (theodoridisaristomenis@gmail.com)
///
/// \brief The CoCopeLia caching functions.
///

//#include <cassert>
//#include <atomic>

#include "unihelpers.hpp"
#include "DataCaching.hpp"
//#include "backend_wrappers.hpp"

Cache_p DevCache[LOC_NUM] = {NULL};
int CBlock_ctr[LOC_NUM] = {0};
int DevCache_ctr = 0;

int globalock = 0;

#if defined(FIFO) || defined(MRU) || defined(LRU)

/**************************
 ** LinkedList Functions **
 **************************/

LinkedList::LinkedList(Cache_p cache){
	short lvl = 2;
	if(cache==NULL)
		error("LinkedList::LinkedList(): Creating cache that doesn't belong to a cache.\n");
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::LinkedList():\n", cache->dev_id);
#endif
	Parent = cache;
	start = NULL;
	end = NULL;
	length = 0;
	iter = NULL;
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::LinkedList()\n", Parent->dev_id);
#endif
}

LinkedList::~LinkedList(){
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::~LinkedList()\n", Parent->dev_id);
#endif
	Node_LL_p tmp;
	while(start != NULL){
		tmp = start;
		start = tmp->next;
		delete(tmp);
	}
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::~LinkedList()\n", Parent->dev_id);
#endif
}

void LinkedList::draw_queue(bool lockfree){
	short lvl = 1;
	if(!lockfree)
		lock();
	int count = 1;
#if defined(FIFO)
	lprintf(lvl-1, " FIFO");
#elif defined(MRU)
	lprintf(lvl-1, " MRU");
#elif defined(LRU)
	lprintf(lvl-1, " LRU");
#endif
	lprintf(lvl-1, " Queue:\
						\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\
						\n||      Cache Id       | %d\
						\n|| - - - - - - - - - - - - - - - - - - - -\
						\n||      Device Id      | %d\
						\n|| - - - - - - - - - - - - - - - - - - - -\
						\n||       Length        | %d\
						\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", Parent->id, Parent->dev_id, length);
	if(length>0){
	iter = start;
	lprintf(lvl-1, " Position: %d\
						\n_________________________________________\
						\n|       Idx       | %d\
						\n| - - - - - - - - - - - - - - - - - - - -\
						\n|      Valid      | %s\
						\n|________________________________________\
						\n", count, iter->idx, iter->valid ? "True" : "False");
		iter = iter->next;
		count++;
		while(iter!=NULL && count<=length){
			lprintf(lvl-1, " Position: %d\
								\n_________________________________________\
								\n|       Idx       | %d\
								\n| - - - - - - - - - - - - - - - - - - - -\
								\n|      Valid      | %s\
								\n|________________________________________\
								\n", count, iter->idx, iter->valid ? "True" : "False");
			iter = iter->next;
			count++;
		}
	}
	if(!lockfree)
		unlock();
}

void LinkedList::invalidate(Node_LL_p node, bool lockfree){
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::invalidate(node_id=%d)\n", Parent->dev_id, node->idx);
#endif
	if(node == NULL)
		error("[dev_id=%d] LinkedList::invalidate: Node not found.\n", Parent->dev_id);
	else if(node->idx>=Parent->BlockNum)
		error("[dev_id=%d] LinkedList::invalidate: Node idx (%d) is larger than the BlockNum(%d).\n",
				Parent->dev_id,  node->idx, Parent->BlockNum);
	else{
		if(!lockfree)
			lock();
		node->valid = false;
		if(!lockfree)
			unlock();
		}
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::invalidate(node_id=%d)\n", Parent->dev_id, node->idx);
#endif
}

void LinkedList::push_back(int idx, bool lockfree){
	// Pushes an element in the back of the queue.
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::push_back(idx=%d)\n", Parent->dev_id, idx);
#endif
	if(!lockfree)
		lock();
	if(idx >= Parent->BlockNum)
		error("[dev_id=%d] LinkedList::push_back(): Index given(%d) is larger than the number of blocks(%d).\n", Parent->dev_id, idx, Parent->BlockNum);
	else if(length > Parent->BlockNum)
		error("[dev_id=%d] LinkedList::push_back(): Called to put another element but max length is reached with BlockNum=%d and length=%d\n", Parent->dev_id, Parent->BlockNum, length);
	Node_LL_p tmp = new Node_LL();
	tmp->next = NULL;
	tmp->previous = NULL;
	tmp->idx = idx;
	tmp->valid = true;
	if(start == NULL){ // Queue is empty.
		start = tmp;
		end = tmp;

	}
	else{ // Queue not empty.
		end->next = tmp;
		tmp->previous = end;
		end = tmp;
	}
	length++;
	if(!lockfree)
		unlock();
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::push_back(idx=%d)\n", Parent->dev_id, idx);
#endif

}


Node_LL_p LinkedList::start_iterration(){
	// Returns the first valid element without removing it.
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::start_iterration()\n", Parent->dev_id);
#endif
	if(start == NULL) // Queue is empty.
		error("[dev_id=%d] LinkedList::start_iterration(): Fifo queue is empty. Can't pop element.\n", Parent->dev_id);
	else{ // Queue not empty.
		iter = start;
		while(iter != NULL && !iter->valid)
			iter = iter->next;
		if(iter == NULL){
			Node_LL_p tmp_node = new Node_LL();
			tmp_node->idx = -1;
		#ifdef CDEBUG
			lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::start_iterration()\n", Parent->dev_id);
		#endif
			return tmp_node;
		}
		else{
		#ifdef CDEBUG
			lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::start_iterration()\n", Parent->dev_id);
		#endif
			return iter;
		}
	}
}

Node_LL_p LinkedList::next_in_line(){
	// Returns next element in iterration.
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::next_in_line()\n", Parent->dev_id);
#endif
	if(iter == NULL)
		error("[dev_id=%d] LinkedList::next_in_line(): Iterration not started. Call check_first.\n", Parent->dev_id);
	else{
		iter = iter->next;
		while(iter != NULL && !iter->valid)
			iter = iter->next;
		if(iter == NULL){
			Node_LL_p tmp_node = new Node_LL();
			tmp_node->idx = -1;
			return tmp_node;
		#ifdef CDEBUG
			lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::next_in_line()\n", Parent->dev_id);
		#endif
		}
		else{
		#ifdef CDEBUG
			lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::next_in_line()\n", Parent->dev_id);
		#endif
			return iter;
		}
	}
}

int LinkedList::remove(Node_LL_p node, bool lockfree){
	// Removes first element.
	short lvl = 2;

#ifdef CDEBUG
	if(node!=NULL)
		lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::remove(idx=%d)\n", Parent->dev_id, node->idx);
#endif

	int tmp_idx;
	Node_LL_p tmp_node;

	if(node == NULL) // Node not found.
		error("[dev_id=%d] LinkedList::remove(): Input node not found. Can't pop element.\n", Parent->dev_id);
	else if(node->idx >= Parent->BlockNum)
		error("[dev_id=%d] LinkedList::remove(): Index of given node(%d) is larger than the number of blocks(%d).\n", Parent->dev_id, node->idx, Parent->BlockNum);
	else{
		if(!lockfree)
			lock();
		tmp_idx = node->idx;
		tmp_node = node;
		if(start!=tmp_node)
			(tmp_node->previous)->next = tmp_node->next;
		if(end!=tmp_node)
			(tmp_node->next)->previous = tmp_node->previous;
		free(tmp_node);
		length--;
		if(!lockfree)
			unlock();
#ifdef CDEBUG
		lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::remove(idx=%d)\n", Parent->dev_id, node->idx);
#endif
		return tmp_idx;
	}
}

void LinkedList::put_first(Node_LL* node, bool lockfree){
	// Puts the element first on the list.
	short lvl = 2;

#ifdef CDEBUG
	if(node != NULL)
		lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::put_first(idx=%d)\n", Parent->dev_id, node->idx);
#endif
	if(node == NULL)
		error("[dev_id=%d] LinkedList::put_first(): Input node not found. Can't move element.\n", Parent->dev_id);
	else if(node->idx<0)
		error("[dev_id=%d] LinkedList::put_first(): Input node is invalid. Can't move element.\n", Parent->dev_id);
	else if(node->idx>=Parent->BlockNum)
		error("[dev_id=%d] LinkedList::put_first(): Index of given node(%d) is larger than the number of blocks(%d).\n", Parent->dev_id, node->idx, Parent->BlockNum);
	else if(node->previous==NULL){
		if(!lockfree)
			lock();
		// Node is the first on the list.
		if(start==node){
			node->valid = true;
			if(!lockfree)
				unlock();
		}
		// Node is not on the list.
		else{
			if(!lockfree)
				unlock();
			error("[dev_id=%d] LinkedList::put_first(): Node is not in the list.\n", Parent->dev_id);
		}
	}// Node is somewhere inside the list but not first.
	else{
		if(!lockfree)
			lock();
		if(end==node)
			end=node->previous;

		(node->previous)->next = node->next;
		if(node->next != NULL)
			(node->next)->previous = node->previous;
		start->previous = node;
		node->next = start;
		node->previous = NULL;
		node->valid = true;
		start = node;
		if(!lockfree)
			unlock();
	}
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::put_first(idx=%d)\n", Parent->dev_id, node->idx);
#endif
}

void LinkedList::put_last(Node_LL_p node, bool lockfree){
	// Takes a node and puts it in the end of the list.
	short lvl = 2;

#ifdef CDEBUG
	if(node != NULL)
		lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::put_last(idx=%d)\n", Parent->dev_id, node->idx);
#endif
	if(node == NULL)
		error("[dev_id=%d] LinkedList::put_last(): Input node not found. Can't move element.\n", Parent->dev_id);
	else if(node->idx<0)
		error("[dev_id=%d] LinkedList::put_last(): Input node is invalid. Can't move element.\n", Parent->dev_id);
	else if(node->idx>=Parent->BlockNum)
		error("[dev_id=%d] LinkedList::put_first(): Index of given node(%d) is larger than the number of blocks(%d).\n", Parent->dev_id, node->idx, Parent->BlockNum);
	else if(node->next==NULL){
		if(!lockfree)
			lock();
		// Node is the last on the list.
		if(end==node){
			node->valid = true;
		// Node is not on the list.
			if(!lockfree)
				unlock();
		}
		else{
			if(!lockfree)
				unlock();
			error("[dev_id=%d] LinkedList::put_last: Node is not in the list.\n", Parent->dev_id);
		}
	} // Node is somewhere inside the list but not last.
	else{
		if(!lockfree)
			lock();
		if(node==start)
			start = node->next;

		(node->next)->previous = node->previous;
		if(node->previous != NULL)
			(node->previous)->next = node->next;
		end->next = node;
		node->previous = end;
		node->next = NULL;
		node->valid = true;
		end = node;
		if(!lockfree)
			unlock();
	}
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::put_last(idx=%d)\n", Parent->dev_id, node->idx);
#endif
}

void LinkedList::lock(){
	while(is_locked());
	lock_ll++;
	// lock_ll.lock();
}

void LinkedList::unlock(){
	lock_ll--;
	// lock_ll.unlock();
}

bool LinkedList::is_locked(){
	if(lock_ll==0)
		return false;
	return true;
}

#endif

// #if defined(FIFO)
// LinkedList* fifo_queues[128] = {NULL};
// #elif defined(MRU) || defined(LRU)
// Node_LL** mru_lru_hash[128];
// LinkedList* mru_lru_queues[128] = {NULL};
// #endif


const char* print_state(state in_state){
	switch(in_state){
		case(INVALID):
			return "INVALID";
		case(EXCLUSIVE):
			return "EXCLUSIVE";
		case(SHARABLE):
			return "SHARABLE";
		case(AVAILABLE):
			return "AVAILABLE";
		default:
			error("print_state: Unknown state\n");
	}
}

/***************************
 ** Cache Block Functions **
 ***************************/

CacheBlock::CacheBlock(int block_id, Cache_p block_parent, long long block_size){
	// Constructor for Blocks.
	// Args:
	// - block_id: Id of the block.
	// - block_parent: Cache that the block belongs to.
	// - block_size: Size of usable memory in the block.

	short lvl=3;

	if(block_parent!=NULL && block_id>=0 && block_id<block_parent->BlockNum && block_size>0){
	#ifdef CDEBUG
		lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::CacheBlock(id=%d, Parent_id=%d, Size=%llu)\n", block_parent->dev_id, block_id, block_parent->id, block_size);
	#endif
		id = block_id;
		// TODO: Name?
		Owner_p = NULL;
		Parent = block_parent;
		Size = block_size;
		PendingReaders = 0;
		PendingWriters = 0;

		Adrs = NULL; // Will be set by cache allocate.
		State = INVALID; 	//	Technically no data in, so INVALID?
												//	But AVAILABLE correct for scheduling out as well...
		Event_p Available = new Event(Parent->dev_id);
	#ifdef CDEBUG
		lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::CacheBlock()\n", Parent->dev_id);
	#endif
	}
	else{
		if(block_parent==NULL)
			error("CacheBlock::CacheBlock(): Constructor called with no cache to belong.\n");
		else if(block_id<0 && block_id>=block_parent->BlockNum)
			error("[dev_id=%d] CacheBlock::CacheBlock(): Constructor called with invalid id=%d.\n", block_parent->dev_id, block_id);
		else
			error("[dev_id=%d] CacheBlock::CacheBlock(): Constructor called with invalid mem size=%llu.\n", block_parent->dev_id, block_size);
	}
}

CacheBlock::~CacheBlock(){
	short lvl =3;
	// Destructor of the block.
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::~CacheBlock()\n", Parent->dev_id);
#endif
	lock();
	reset(true, true);
	free(Adrs);
	delete Available;
	unlock();
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::~CacheBlock()\n", Parent->dev_id);
#endif
}

void CacheBlock::draw_block(bool lockfree){
	// Draws the block for debugging purposes.
	short lvl=0;

	if(!lockfree)
		lock();
	lprintf(lvl-1, " Block:   \
						\n_________________________________________\
						\n|       Id        | %d\
						\n| - - - - - - - - - - - - - - - - - - - -\
						\n|      Name       | %s\
						\n| - - - - - - - - - - - - - - - - - - - -\
						\n|      Size       | %llu\
						\n| - - - - - - - - - - - - - - - - - - - -\
						\n|      State      | %s \
						\n| - - - - - - - - - - - - - - - - - - - -\
						\n| Pending Readers | %d\
						\n| - - - - - - - - - - - - - - - - - - - -\
						\n| Pending Writers | %d\
						\n|________________________________________\
						\n", id, Name.c_str(), Size, print_state(State), PendingReaders.load(), PendingWriters.load());
	if(!lockfree)
		unlock();
}

void CacheBlock::add_reader(bool lockfree){
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::add_reader(block_id=%d)\n", Parent->dev_id, id);
#endif
	if(!lockfree)
		lock();
	if(State==SHARABLE || State==EXCLUSIVE)
		PendingReaders++;
	else if(State==AVAILABLE){
		PendingReaders++;
		set_state(SHARABLE, true);
	}
	else
		error("[dev_id=%d] CacheBlock::add_reader(): Can't add reader. Block has State=%s\n", Parent->dev_id, print_state(State));
#if defined(MRU)
	Parent->Queue->put_first(Parent->Hash[id]);
#elif defined(LRU)
	Parent->Queue->put_last(Parent->Hash[id]);
#endif
	if(!lockfree)
		unlock();
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::add_reader(block_id=%d)\n", Parent->dev_id, id);
#endif
}

void CacheBlock::add_writer(bool lockfree){
	short lvl = 2;
	if(!lockfree)
		lock();
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::add_writer(block_id=%d)\n", Parent->dev_id, id);
#endif
	if(State==EXCLUSIVE)
		PendingWriters++;
	else if(State==AVAILABLE || State==SHARABLE){
		PendingWriters++;
		set_state(EXCLUSIVE, true);
	}
	else
		error("[dev_id=%d] CacheBlock::add_reader(): Can't add reader. Block has State=%s\n", Parent->dev_id, print_state(State));
#if defined(MRU)
	Parent->Queue->put_first(Parent->Hash[id]);
#elif defined(LRU)
	Parent->Queue->put_last(Parent->Hash[id]);
#endif
	if(!lockfree)
		unlock();
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::add_writer(block_id=%d)\n", Parent->dev_id, id);
#endif
}

void CacheBlock::remove_reader(){
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::remove_reader(block_id=%d)\n", Parent->dev_id, id);
#endif
	if(PendingReaders.load()>0)
		PendingReaders--;
	else
		error("[dev_id=%d] CacheBlock::remove_reader(): Can't remove reader. There are none.\n", Parent->dev_id);
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::remove_reader(block_id=%d)\n", Parent->dev_id, id);
#endif
}

void CacheBlock::remove_writer(){
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::remove_writer(block_id=%d)\n", Parent->dev_id, id);
#endif
	if(PendingWriters.load()>0)
		PendingWriters--;
	else
		error("[dev_id=%d] CacheBlock::remove_writer(): Can't remove writer. There are none.\n", Parent->dev_id);
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::remove_writer(block_id=%d)\n", Parent->dev_id, id);
#endif
}

void* CBlock_RR_wrap(void* CBlock_wraped){
	//TODO: include lock flag
	CBlock_wrap_p CBlock_unwraped = (CBlock_wrap_p) CBlock_wraped;
	CBlock_unwraped->CBlock->remove_reader();
	return NULL;
}

void* CBlock_RW_wrap(void* CBlock_wraped){
	//TODO: include lock flag
	CBlock_wrap_p CBlock_unwraped = (CBlock_wrap_p) CBlock_wraped;
	CBlock_unwraped->CBlock->remove_writer();
	return NULL;
}

void CacheBlock::set_owner(void** owner_adrs, bool lockfree){
	short lvl = 2;
	if(!lockfree)
		lock();
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> CacheBlock::set_owner(owner_adrs=%p)\n", owner_adrs);
#endif
	Owner_p = owner_adrs;
	if(!lockfree)
		unlock();
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| CacheBlock::set_owner()\n");
#endif
}

void CacheBlock::reset(bool lockfree, bool forceReset){
	// Resets block attibutes if it's AVAILABLE to be used again.
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::reset(block_id=%d)\n", Parent->dev_id, id);
#endif
	if(State==INVALID || forceReset){
		if(!lockfree)
			lock();
		PendingReaders = 0;
		PendingWriters = 0;
		set_state(SHARABLE, true);
		// Available->reset();
		if(Owner_p){
			*Owner_p = NULL;
			Owner_p = NULL;
		}
		if(!lockfree)
			unlock();
	#ifdef CDEBUG
		if(forceReset)
			lprintf(lvl-1, "------- [dev_id=%d] CacheBlock::reset(): Block with id=%d forced to be reseted.\n", Parent->dev_id, id);
		else
			lprintf(lvl-1, "------- [dev_id=%d] CacheBlock::reset(): Block with id=%d reseted.\n", Parent->dev_id, id);
	#endif
	}
	else
		error("[dev_id=%d] CacheBlock::reset(): Reset was called on a %s block.\n", Parent->dev_id, print_state(State));
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::reset(block_id=%d)\n", Parent->dev_id, id);
#endif
}

state CacheBlock::get_state(){
	if(id < 0 || id >= Parent->BlockNum)
		error("[dev_id=%d] CacheBlock::get_state(): Invalid block id=%d\n", Parent->dev_id, id);
	return State;
}

state CacheBlock::set_state(state new_state, bool lockfree){
	// Forces a new state.
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::set_state(block_id=%d, new_state=%s)\n", Parent->dev_id, id, print_state(State));
#endif

	if(id < 0 || id >= Parent->BlockNum)
		error("[dev_id=%d] CacheBlock::set_state(): Invalid block id=%d\n", Parent->dev_id, id);
	if(!lockfree)
		lock();
	state old_state = State;
	State = new_state;
	if(!lockfree)
		unlock();

#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::set_state(block_id=%d, new_state=%s)\n", Parent->dev_id, id, print_state(State));
#endif
	return old_state;
}

int CacheBlock::update_state(bool lockfree){
	// Updates the state of the block. It cannot raise the state but only lower.
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::update_state(block_id=%d)\n", Parent->dev_id, id);
#endif
	int ret = 0;
	if(!lockfree)
		lock();
	state prev_state = State;
	if(PendingWriters > 0){
		if(State!=EXCLUSIVE)
			error("[dev_id=%d] CacheBlock::update_state(): Block has writers but state was %s.\n", Parent->dev_id, print_state(State));
	}
	else if(PendingReaders > 0){
		if(State==EXCLUSIVE){
			set_state(SHARABLE, true);
			ret = 1;
		}
		else if(State!=SHARABLE)
			error("[dev_id=%d] CacheBlock::update_state(): Block has readers but state was %s.\n", Parent->dev_id, print_state(State));
	}
	else if(State == EXCLUSIVE || State == SHARABLE){
		set_state(AVAILABLE, true);
		ret = 1;
	}
#ifdef CDEBUG
	if(ret==1)
		lprintf(lvl-1, "------- [dev_id=%d] CacheBlock::update_state(block_id=%d): Block state was changed %s -> %s \n", Parent->dev_id, id, print_state(prev_state), print_state(State));
	else
		lprintf(lvl-1, "------- [dev_id=%d] CacheBlock::update_state(block_id=%d): Block state is still %s \n", Parent->dev_id, id, print_state(State));
#endif
	if(!lockfree)
		unlock();
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::update_state(block_id=%d)\n", Parent->dev_id, id);
#endif
	return ret;
}

void CacheBlock::lock(){
	while(is_locked());
	Lock++;
}

void CacheBlock::unlock(){
	Lock--;
}

bool CacheBlock::is_locked(){
	if(Lock==0)
		return false;
	return true;
}

/*********************
 ** Cache Functions **
 *********************/

Cache::Cache(int dev_id_in, long long block_num, long long block_size){
	// Constructor for caches
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] Cache::Cache(block_num = %lld, block_size = %lld)\n", dev_id_in, block_num, block_size);
#endif
	id = DevCache_ctr++;
	dev_id = dev_id_in;
	BlockSize = block_size;
	SerialCtr = 0;
	BlockNum = block_num;
	Size = BlockSize*BlockNum;
	Blocks =  (CBlock_p*) malloc (BlockNum * sizeof(CBlock_p));
	for (int idx = 0; idx < BlockNum; idx++) Blocks[idx] = new CacheBlock(idx, this, BlockSize); // Or NULL here and initialize when requested? not sure
#if defined(FIFO)
	Queue = new LinkedList(this);
#elif defined(MRU) || defined(LRU)
	Hash = (Node_LL_p*) malloc(BlockNum * sizeof(Node_LL_p));
	Queue = new LinkedList(this);
#endif
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] Cache::Cache()\n", dev_id_in);
#endif
	return ;
}

Cache::~Cache(){
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] Cache::~Cache()\n", dev_id);
#endif
	lock();
	DevCache_ctr--;
	for (int idx = 0; idx < BlockNum; idx++) delete Blocks[idx];
	free(Blocks);
#if defined(FIFO)
	delete Queue;
#elif defined(MRU) || defined(LRU)
	free(Hash);
	delete(Queue);
#endif
	unlock();
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] Cache::~Cache()\n", dev_id);
#endif
	return ;
}

void Cache::draw_cache(bool print_blocks, bool print_queue, bool lockfree){
	short lvl = 0;

	if(!lockfree)
		lock();
	lprintf(lvl-1, " Cache:\
						\n==========================================\
						\n||      Cache Id       | %d\
						\n|| - - - - - - - - - - - - - - - - - - - -\
						\n||      Device Id      | %d\
						\n|| - - - - - - - - - - - - - - - - - - - -\
						\n||        Name         | %s\
						\n|| - - - - - - - - - - - - - - - - - - - -\
						\n||        Size         | %llu\
						\n|| - - - - - - - - - - - - - - - - - - - -\
						\n||  Number of blocks   | %d\
						\n|| - - - - - - - - - - - - - - - - - - - -\
						\n||   Size of blocks    | %llu\
						\n|| - - - - - - - - - - - - - - - - - - - -", id, dev_id, Name.c_str(), Size, BlockNum, BlockSize);
#if defined(NAIVE)
	lprintf(lvl-1,"\n||  Scheduling policy  | NAIVE");
#elif defined(FIFO)
	lprintf(lvl-1,"\n||  Scheduling policy  | FIFO");
#elif defined(MRU)
	lprintf(lvl-1,"\n||  Scheduling policy  | MRU");
#elif defined(LRU)
	lprintf(lvl-1,"\n||  Scheduling policy  | LRU");
#endif

	lprintf(lvl-1, "\n==========================================");

	if(print_blocks){
		lprintf(lvl-1, "======================================\
							\n|| Start of Blocks in Cache ||\
							\n==============================\n");
		for(int i=0; i<BlockNum; i++)
			if(Blocks[i]!=NULL)
				Blocks[i]->draw_block(true);
		lprintf(lvl-1, "============================\
							\n|| End of Blocks in Cache ||\
							\n================================================================================\n");

	}
	if(print_queue){
		#if defined(NAIVE)
		lprintf(lvl-1, "There is no Queue.\n");
		#elif defined(FIFO) || defined(MRU) || defined(LRU)
		lprintf(lvl-1, "|| Start of Blocks in Cache ||\
							\n==============================\n");
		Queue->draw_queue();
		lprintf(lvl-1, "============================\
							\n|| End of Blocks in Cache ||\
							\n================================================================================\n");
		#endif
	}
	lprintf(lvl-1, "\n");
	if(!lockfree)
		unlock();
}

CBlock_p Cache::assign_Cblock(){
	// Assigns a block from cache to be used for memory.

	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] Cache::assign_Cblock()\n", dev_id);
#endif

	CBlock_p result = NULL;
	lock(); // Lock cache
	if (SerialCtr >= BlockNum){
		int remove_block_idx = -42;
	#if defined(NAIVE)
		remove_block_idx = CacheSelectBlockToRemove_naive(this);
	#elif defined(FIFO)
		Node_LL_p remove_block;
		remove_block = CacheSelectBlockToRemove_fifo(this);
		remove_block_idx = remove_block->idx;
	#elif defined(MRU) || defined(LRU)
		Node_LL_p remove_block;
		remove_block = CacheSelectBlockToRemove_mru_lru(this);
		remove_block_idx = remove_block->idx;
	#endif
		if(remove_block_idx >= 0){
	#if defined(FIFO)
			Queue->lock();
			int remove_block_idx = Queue->remove(remove_block, true);
			Queue->push_back(remove_block_idx, true);
			Queue->unlock();
	#elif defined(MRU) || defined(LRU)
			int remove_block_idx = remove_block->idx;
		#if defined(MRU)
			Queue->put_first(remove_block);
		#elif defined(LRU)
			Queue->put_last(remove_block);
		#endif
	#endif
			Blocks[remove_block_idx]->reset(true);
			result = Blocks[remove_block_idx];
	#ifdef CDUBUG
		lprintf(lvl-1,"------ [dev_id=%d] Cache::assign_Cblock(): Block with id=%d reseted.\n", dev_id, remove_block_idx);
	#endif
			unlock(); // Unlock cache
		}
		else{
	#if defined(FIFO) || defined(MRU) || defined(LRU)
			delete(remove_block);
	#endif
			error("[dev_id=%d] Cache::assign_Cblock()-Rec: entry\n", dev_id);
			unlock(); // Unlock cache
			result = assign_Cblock();
		}
	}
	else{
		result = Blocks[SerialCtr];
		result->reset(true);
#if defined(FIFO)
		Queue->push_back(SerialCtr);
#elif defined(MRU)
		Queue->lock();
		Queue->push_back(SerialCtr, true);
		Hash[SerialCtr] = Queue->end;
		Queue->put_first(Hash[SerialCtr], true);
		Queue->unlock();
#elif defined(LRU)
		Queue->lock();
		Queue->push_back(SerialCtr, true);
		Hash[SerialCtr] = Queue->end;
		Queue->unlock();
#endif
		SerialCtr++;
		unlock(); // Unlock cache
	}
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] Cache::assign_Cblock()\n", dev_id);
#endif
  return result;
}

void Cache::lock(){
	while(is_locked());
	Lock++;
	// Lock.lock();
}

void Cache::unlock(){
	Lock--;
}

bool Cache::is_locked(){
	if(Lock==0)
		return false;
	return true;

}

/*********************
 ** Other Functions **
 *********************/

#if defined(NAIVE)
int CacheSelectBlockToRemove_naive(Cache_p cache){
	short lvl = 2;

	if (cache == NULL)
		error("CacheSelectBlockToRemove_naive(): Called on empty buffer\n");

#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheSelectBlockToRemove_naive()\n",cache->dev_id);
#endif

	int result_idx = -1;

	for (int idx = 0; idx < cache->BlockNum; idx++){ // Iterate through cache serially.
		cache->Blocks[idx]->lock();
		cache->Blocks[idx]->update_state(true);
		state tmp_state = cache->Blocks[idx]->get_state(); // Update all events etc for idx.
		if(tmp_state == INVALID || tmp_state == AVAILABLE){ // Indx can be removed if there are no pending events.
			result_idx =  idx;
			cache->Blocks[idx]->set_state(INVALID, true);
			cache->Blocks[idx]->unlock();
		#ifdef CDEBUG
			lprintf(lvl-1, "------- [dev_id=%d] CacheSelectBlockToRemove_naive(): Found available block. Invalidated.\n",cache->dev_id);
		#endif
			break;
		}
		cache->Blocks[idx]->unlock();
	}
	#ifdef CDEBUG
		lprintf(lvl-1, "<-----| [dev_id=%d] CacheSelectBlockToRemove_naive()\n",cache->dev_id);
	#endif
	return result_idx;
}

#elif defined(FIFO)
Node_LL_p CacheSelectBlockToRemove_fifo(Cache_p cache){
	short lvl = 2;
	if (cache == NULL)
		error("CacheSelectBlockToRemove_fifo(): Called on empty buffer\n");

#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheSelectBlockToRemove_fifo()\n", cache->dev_id);
#endif
	Node_LL_p result_node = new Node_LL();

	result_node->idx = -1;
	state tmp_state = INVALID;
	cache->Queue->lock();
	Node_LL_p node = cache->Queue->start_iterration();
	if(node->idx >= 0){
		cache->Blocks[node->idx]->lock();
		cache->Blocks[node->idx]->update_state(true);
		tmp_state = cache->Blocks[node->idx]->get_state(); // Update all events etc for idx.
	}
	while(tmp_state != AVAILABLE){
		cache->Blocks[node->idx]->unlock();
		node = cache->Queue->next_in_line();
		if(node->idx >= 0){
			cache->Blocks[node->idx]->lock();
			cache->Blocks[node->idx]->update_state(true);
			tmp_state = cache->Blocks[node->idx]->get_state(); // Update all events etc for idx.
		}
		else
			break;
	}
	if(node->idx >=0){
		// cache->Blocks[idx]->unlock();
		if(tmp_state == AVAILABLE){
			cache->Queue->invalidate(node, true);
			delete(result_node);
			result_node = node;
			cache->Blocks[result_node->idx]->set_state(INVALID, true);
			// cache->Blocks[idx]->unlock();
		#ifdef CDEBUG
			lprintf(lvl-1, "------- [dev_id=%d] CacheSelectBlockToRemove_fifo(): Found available block. Invalidated.\n",cache->dev_id);
		#endif
		}
		cache->Blocks[result_node->idx]->unlock();
	}
	cache->Queue->unlock();

#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheSelectBlockToRemove_fifo()\n",cache->dev_id);
#endif
	return result_node;
}

#elif defined(MRU) || defined(LRU)
Node_LL_p CacheSelectBlockToRemove_mru_lru(Cache_p cache){
short lvl = 2;

	if (cache == NULL)
		error("CacheSelectBlockToRemove_mru_lru(): Called on empty buffer\n");

#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheSelectBlockToRemove_mru_lru()\n",cache->dev_id);
#endif

	Node_LL_p result_node = new Node_LL();

	result_node->idx = -1;
	state tmp_state = INVALID;
	cache->Queue->lock();
	Node_LL_p node = cache->Queue->start_iterration();
	if(node->idx >= 0){
		cache->Blocks[node->idx]->lock();
		cache->Blocks[node->idx]->update_state(true);
		tmp_state = cache->Blocks[node->idx]->get_state(); // Update all events etc for idx.
	}
	while(tmp_state != AVAILABLE){
		cache->Blocks[node->idx]->unlock();
		node = cache->Queue->next_in_line();
		if(node->idx >= 0){
			cache->Blocks[node->idx]->lock();
			cache->Blocks[node->idx]->update_state(true);
			tmp_state = cache->Blocks[node->idx]->get_state(); // Update all events etc for idx.
		}
		else
			break;
	}
	if(node->idx >=0){
		// cache->Blocks[idx]->unlock();
		if(tmp_state == AVAILABLE){
			cache->Queue->invalidate(node, true);
			delete(result_node);
			result_node = node;
			cache->Blocks[result_node->idx]->set_state(INVALID, true);
			// cache->Blocks[idx]->unlock();
		#ifdef CDEBUG
			lprintf(lvl-1, "------- [dev_id=%d] CacheSelectBlockToRemove_mru_lru(): Found available block. Invalidated.\n",cache->dev_id);
		#endif
		}
		cache->Blocks[result_node->idx]->unlock();
	}
	cache->Queue->unlock();

#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheSelectBlockToRemove_mru_lru()\n",cache->dev_id);
#endif
	return result_node;
}
#endif

/*
void CoCopeLiaDevCacheFree(short dev_id)
{
	short lvl = 3;
#ifdef DEBUG
			lprintf(lvl-1, "|-----> CoCopeLiaDevCacheFree(dev_id=%d)\n", dev_id);
#endif
  short dev_id_idx = (dev_id >= 0) ? dev_id : LOC_NUM - 1;
#ifdef TEST
	double cpu_timer = csecond();
#endif
	if (DevCache[dev_id_idx]){
#ifdef DEBUG
		lprintf(lvl, "Clearing (presumably) %zu MB\n\n", (size_t) DevCache[dev_id_idx]->mem_buf_sz/1024/1024);
#endif
		CoCoFree(DevCache[dev_id_idx]->gpu_mem_buf, dev_id);
		free(DevCache[dev_id_idx]->BlockState);
		free(DevCache[dev_id_idx]->BlockReaders);
		free(DevCache[dev_id_idx]->BlockCurrentTileDim);
		free(DevCache[dev_id_idx]->BlockCurrentTilePtr);
		free(DevCache[dev_id_idx]);
		DevCache[dev_id_idx] = NULL;
    recursion_depth[dev_id_idx] = 0;
		CoCoSyncCheckErr();
	}
	else{
#ifdef DEBUG
		lprintf(lvl, "Target buffer already empty\n");
#endif
		;
	}
#ifdef TEST
	cpu_timer = csecond() - cpu_timer;
	lprintf(lvl, "Buffer de-allocation(%d): t_free = %lf ms\n" , dev_id, cpu_timer*1000);
#endif
#ifdef DEBUG
	lprintf(lvl-1, "<-----|\n");
#endif
}

void CoCoPeLiaDevCacheInvalidate(kernel_pthread_wrap_p subkernel_data){
	short lvl = 3;
#ifdef DEBUG
	lprintf(lvl-1, "|-----> CoCoPeLiaDevCacheInvalidate(subkernel_list(len=%d)\n", subkernel_data->SubkernelNumDev);
#endif
#ifdef TEST
	double cpu_timer = csecond();
#endif
	for (int i = 0; i < subkernel_data->SubkernelNumDev; i++){
		Subkernel* curr = subkernel_data->SubkernelListDev[i];
		short dev_id_idx = idxize(curr->run_dev_id);
		for (int j = 0; j < curr->TileNum; j++){
			if (curr->TileDimlist[j] == 1) {
					Tile1D<VALUE_TYPE>* tmp = (Tile1D<VALUE_TYPE>*) curr->TileList[j];
					if (tmp->CacheLocId[dev_id_idx] != -1) tmp->CacheLocId[dev_id_idx] = -42;
					tmp->available[dev_id_idx]->reset();
			}
			else if (curr->TileDimlist[j] == 2){
					Tile2D<VALUE_TYPE>* tmp = (Tile2D<VALUE_TYPE>*) curr->TileList[j];
					if (tmp->CacheLocId[dev_id_idx] != -1) tmp->CacheLocId[dev_id_idx] = -42;
					tmp->available[dev_id_idx]->reset();
			}
			else error("CoCoPeLiaDevCacheInvalidate: Not implemented for TileDim=%d\n", curr->TileDimlist[j]);
		}
		 if (DevCache[dev_id_idx]!= NULL && DevCache[dev_id_idx]->serialCtr){
			 DevCache[dev_id_idx]->serialCtr = 0;
			 recursion_depth[dev_id_idx] = 0;
			 for (int idx = 0; idx < DevCache[dev_id_idx]->BlockNum; idx++){
				 DevCache[dev_id_idx]->BlockReaders[idx] = 0;
				 DevCache[dev_id_idx]->BlockState[idx] = EMPTY;
				 DevCache[dev_id_idx]->BlockCurrentTilePtr[idx] = NULL;
		 	}
		}
	}
#ifdef TEST
	cpu_timer = csecond() - cpu_timer;
	lprintf(lvl, "Cache for %d Subkernels invalidated: t_nv = %lf ms\n" , subkernel_data->SubkernelNumDev, cpu_timer*1000);
#endif
#ifdef DEBUG
	lprintf(lvl-1, "<-----|\n");
#endif
}

void* CacheAsignBlock(short dev_id, void* TilePtr, short TileDim){

  short lvl = 5;
	int* block_id_ptr;
	short dev_id_idx = (dev_id == -1) ? LOC_NUM - 1: dev_id;
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	if (TileDim == 1){
		Tile1D<VALUE_TYPE>* tmp = (Tile1D<VALUE_TYPE>*) TilePtr;
		block_id_ptr = &tmp->CacheLocId[dev_id_idx];
#ifdef DEBUG
  	lprintf(lvl-1, "|-----> CacheAsignBlock(dev= %d, T = %d.[%d] )\n",
			dev_id, tmp->id, tmp->GridId);
#endif
	}
	else if(TileDim == 2){
		Tile2D<VALUE_TYPE>* tmp = (Tile2D<VALUE_TYPE>*) TilePtr;
		block_id_ptr = &tmp->CacheLocId[dev_id_idx];
#ifdef DEBUG
  	lprintf(lvl-1, "|-----> CacheAsignBlock(dev= %d, T = %d.[%d,%d] )\n",
			dev_id, tmp->id, tmp->GridId1, tmp->GridId2);
#endif
	}
	else error("CacheAsignBlock(%d): Tile%dD not implemented\n", dev_id, TileDim);
	if (DevCache[dev_id_idx] == NULL)
		error("CacheAsignBlock(%d): Called on empty buffer\n", dev_id);
	void* result = NULL;
  if (DevCache[dev_id_idx]->serialCtr >= DevCache[dev_id_idx]->BlockNum){
#ifdef STEST
		total_cache_timer[dev_id_idx]+= csecond();
#endif
    return CacheUpdateAsignBlock(dev_id, TilePtr, TileDim);
	}
	else{
 #if CACHE_SCHEDULING_POLICY==1
		fifo_queues[dev_id_idx].lock_ll.lock();
#elif CACHE_SCHEDULING_POLICY==2 || CACHE_SCHEDULING_POLICY==3
		mru_lru_queues[dev_id_idx].lock_ll.lock();
#endif
  	result = DevCache[dev_id_idx]->gpu_mem_buf + DevCache[dev_id_idx]->serialCtr*DevCache[dev_id_idx]->BlockSize;
		*block_id_ptr = DevCache[dev_id_idx]->serialCtr;
		DevCache[dev_id_idx]->BlockCurrentTileDim[DevCache[dev_id_idx]->serialCtr] = TileDim;
		DevCache[dev_id_idx]->BlockCurrentTilePtr[DevCache[dev_id_idx]->serialCtr] = TilePtr;
#if CACHE_SCHEDULING_POLICY==1
		fifo_queues[dev_id_idx].push_back(DevCache[dev_id_idx]->serialCtr);
		fifo_queues[dev_id_idx].lock_ll.unlock();
#elif CACHE_SCHEDULING_POLICY==2
		mru_lru_queues[dev_id_idx].push_back(DevCache[dev_id_idx]->serialCtr);
		mru_lru_hash[dev_id_idx][DevCache[dev_id_idx]->serialCtr] = mru_lru_queues[dev_id_idx].end;
		mru_lru_queues[dev_id_idx].put_first(mru_lru_hash[dev_id_idx][DevCache[dev_id_idx]->serialCtr]);
		mru_lru_queues[dev_id_idx].lock_ll.unlock();
#elif CACHE_SCHEDULING_POLICY==3
		mru_lru_queues[dev_id_idx].push_back(DevCache[dev_id_idx]->serialCtr);
		mru_lru_hash[dev_id_idx][DevCache[dev_id_idx]->serialCtr] = mru_lru_queues[dev_id_idx].end;
		mru_lru_queues[dev_id_idx].lock_ll.unlock();
#endif
		DevCache[dev_id_idx]->serialCtr++;
	}
#ifdef DEBUG
	lprintf(lvl-1, "<-----|\n");
#endif
#ifdef STEST
	total_cache_timer[dev_id_idx]+= csecond();
#endif
  return result;
}

void* CacheUpdateAsignBlock(short dev_id, void* TilePtr, short TileDim){
	short lvl = 5;
	int* block_id_ptr;
	short dev_id_idx = (dev_id == -1) ? LOC_NUM - 1: dev_id;
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	if (TileDim == 1){
		Tile1D<VALUE_TYPE>* tmp = (Tile1D<VALUE_TYPE>*) TilePtr;
		block_id_ptr = &tmp->CacheLocId[dev_id_idx];
#ifdef DEBUG
  	lprintf(lvl-1, "|-----> CacheUpdateAsignBlock(dev= %d, T = %d.[%d] )\n",
			dev_id, tmp->id, tmp->GridId);
#endif
	}
	else if(TileDim == 2){
		Tile2D<VALUE_TYPE>* tmp = (Tile2D<VALUE_TYPE>*) TilePtr;
		block_id_ptr = &tmp->CacheLocId[dev_id_idx];
#ifdef DEBUG
  	lprintf(lvl-1, "|-----> CacheUpdateAsignBlock(dev= %d, T = %d.[%d,%d] )\n",
			dev_id, tmp->id, tmp->GridId1, tmp->GridId2);
#endif
	}
	else error("CacheUpdateAsignBlock(%d): Tile%dD not implemented\n", dev_id, TileDim);
	if (DevCache[dev_id_idx] == NULL)
		error("CacheUpdateAsignBlock(%d): Called on empty buffer\n", dev_id);
	while(__sync_lock_test_and_set (&globalock, 1));
	void* result = NULL;
#if CACHE_SCHEDULING_POLICY==0
	int remove_block_idx = CacheSelectBlockToRemove_naive(dev_id);
	if(remove_block_idx >= 0){
		result = DevCache[dev_id_idx]->gpu_mem_buf + remove_block_idx*DevCache[dev_id_idx]->BlockSize;
		DevCache[dev_id_idx]->BlockState[remove_block_idx] = EMPTY;

#elif CACHE_SCHEDULING_POLICY==1
	Node_LL* remove_block;
	remove_block = CacheSelectBlockToRemove_fifo(dev_id);
	if(remove_block->idx >= 0){
		fifo_queues[dev_id_idx].lock_ll.lock();
		int remove_block_idx = fifo_queues[dev_id_idx].remove(remove_block);
		fifo_queues[dev_id_idx].push_back(remove_block_idx);
		fifo_queues[dev_id_idx].lock_ll.unlock();
		result = DevCache[dev_id_idx]->gpu_mem_buf + remove_block_idx*DevCache[dev_id_idx]->BlockSize;
		DevCache[dev_id_idx]->BlockState[remove_block_idx] = EMPTY;

#elif CACHE_SCHEDULING_POLICY==2 || CACHE_SCHEDULING_POLICY==3
	Node_LL* remove_block;
	remove_block = CacheSelectBlockToRemove_mru_lru(dev_id);
	if(remove_block->idx >= 0){
		mru_lru_queues[dev_id_idx].lock_ll.lock();
		int remove_block_idx = remove_block->idx;
	#if CACHE_SCHEDULING_POLICY==2
		mru_lru_queues[dev_id_idx].put_first(remove_block);
	#elif CACHE_SCHEDULING_POLICY==3
		mru_lru_queues[dev_id_idx].put_last(remove_block);
	#endif
		mru_lru_queues[dev_id_idx].lock_ll.unlock();
		result = DevCache[dev_id_idx]->gpu_mem_buf + remove_block_idx*DevCache[dev_id_idx]->BlockSize;
		DevCache[dev_id_idx]->BlockState[remove_block_idx] = EMPTY;
#endif
		if (TileDim == 1){
			Tile1D<VALUE_TYPE>* prev_tile = (Tile1D<VALUE_TYPE>*)
				DevCache[dev_id_idx]->BlockCurrentTilePtr[remove_block_idx];
			prev_tile->CacheLocId[dev_id_idx] = -42;
			prev_tile->available[dev_id_idx]->reset();
		#ifdef CDEBUG
				lprintf(lvl, "CacheSelectBlockToRemove(%d): Replacing Block(idx=%d) - T(%d.[%d])...\n",
				dev_id, remove_block_idx, prev_tile->id, prev_tile->GridId);
		#endif
		}
		else if(TileDim == 2){
			Tile2D<VALUE_TYPE>* prev_tile = (Tile2D<VALUE_TYPE>*)
				DevCache[dev_id_idx]->BlockCurrentTilePtr[remove_block_idx];
			prev_tile->CacheLocId[dev_id_idx] = -42;
			prev_tile->available[dev_id_idx]->reset();
		#ifdef CDEBUG
				lprintf(lvl, "CacheSelectBlockToRemove(%d): Replacing Block(idx=%d)- T(%d.[%d,%d])...\n",
				dev_id, remove_block_idx, prev_tile->id, prev_tile->GridId1, prev_tile->GridId2);
		#endif
		}
		else error("CacheUpdateAsignBlock(%d): Tile%dD not implemented\n", dev_id, TileDim);
		*block_id_ptr = remove_block_idx;
		DevCache[dev_id_idx]->BlockCurrentTileDim[remove_block_idx] = TileDim;
		DevCache[dev_id_idx]->BlockCurrentTilePtr[remove_block_idx] = TilePtr;
	}
	else{
#if CACHE_SCHEDULING_POLICY==1 || CACHE_SCHEDULING_POLICY==2 || CACHE_SCHEDULING_POLICY==3
		free(remove_block);
#endif

		error("CacheUpdateAsignBlock(dev= %d)-Rec: entry\n",dev_id);
		__sync_lock_release(&globalock);
#ifdef STEST
		total_cache_timer[dev_id_idx]+= csecond();
#endif
		return CacheUpdateAsignBlock(dev_id, TilePtr, TileDim);
	}
#ifdef DEBUG
	lprintf(lvl-1, "<-----|\n");
#endif
		__sync_lock_release(&globalock);
#ifdef STEST
		total_cache_timer[dev_id_idx]+= csecond();
#endif
  return result;
}

#if CACHE_SCHEDULING_POLICY==0
int CacheSelectBlockToRemove_naive(short dev_id){
	short lvl = 6;
#ifdef DEBUG
	lprintf(lvl-1, "|-----> CacheSelectBlockToRemove_naive(dev= %d)\n",dev_id);
#endif
#ifdef CTEST
	cpu_timer = csecond();
#endif
	short dev_id_idx = (dev_id == -1) ? LOC_NUM - 1: dev_id;
	int result_idx = -1;
	if (DevCache[dev_id_idx] == NULL)
		error("CacheSelectBlockToRemove_naive(%d): Called on empty buffer\n", dev_id);
	for (int idx = 0; idx < DevCache[dev_id_idx]->BlockNum; idx++){ // Iterate through cache serially.
		if(DevCache[dev_id_idx]->BlockState[idx] == AVAILABLE){ 		// Index can be removed if block is available
			result_idx =  idx;
			break;
		}
	}
#ifdef CTEST
	cpu_timer = csecond() - cpu_timer;
	lprintf(lvl-1, "CacheSelectBlockToRemove_naive(%d): t_select = %lf\n",dev_id, cpu_timer);
	cpu_timer = csecond();
#endif
#ifdef DEBUG
	lprintf(lvl-1, "<-----|\n");
#endif
	return result_idx;
}

#elif CACHE_SCHEDULING_POLICY==1
Node_LL* CacheSelectBlockToRemove_fifo(short dev_id){
	short lvl = 6;
#ifdef DEBUG
	lprintf(lvl-1, "|-----> CacheSelectBlockToRemove_fifo(dev= %d)\n",dev_id);
#endif
#ifdef CTEST
	cpu_timer = csecond();
#endif
	Node_LL* result_node = new Node_LL();

  short dev_id_idx = (dev_id >= 0) ? dev_id : LOC_NUM - 1;
	result_node->idx = -1;
	if (DevCache[dev_id_idx] == NULL)
		error("CacheSelectBlockToRemove_fifo(%d): Called on empty buffer\n", dev_id);
	// while(__sync_lock_test_and_set (&globalock, 1));
	fifo_queues[dev_id_idx].lock_ll.lock();
	Node_LL* node = fifo_queues[dev_id_idx].start_iterration();
	while(DevCache[dev_id_idx]->BlockState[node->idx] != AVAILABLE){
		node = fifo_queues[dev_id_idx].next_in_line();
		if(node->idx < 0) break;
	}
	if(node->idx >=0 && DevCache[dev_id_idx]->BlockState[node->idx] == AVAILABLE){
		lprintf(lvl-1, "|-----> CacheSelectBlockToRemove_fifo: Invalidating\n");
		fifo_queues[dev_id_idx].invalidate(node);
		free(result_node);
		result_node = node;
	}
	fifo_queues[dev_id_idx].lock_ll.unlock();
	// __sync_lock_release(&globalock);

#ifdef CTEST
	cpu_timer = csecond() - cpu_timer;
	lprintf(lvl-1, "CacheSelectBlockToRemove_fifo(%d): t_select = %lf\n",dev_id, cpu_timer);
	cpu_timer = csecond();
#endif
#ifdef DEBUG
	lprintf(lvl-1, "<-----|\n");
#endif
	return result_node;
}

#elif CACHE_SCHEDULING_POLICY==2 || CACHE_SCHEDULING_POLICY==3
Node_LL* CacheSelectBlockToRemove_mru_lru(short dev_id){
short lvl = 6;
#ifdef DEBUG
	lprintf(lvl-1, "|-----> CacheSelectBlockToRemove_mru_lru(dev= %d)\n",dev_id);
#endif
#ifdef CTEST
	cpu_timer = csecond();
#endif

Node_LL* result_node = new Node_LL();

	result_node->idx = -1;
	short dev_id_idx = (dev_id == -1) ? LOC_NUM - 1: dev_id;

	if (DevCache[dev_id_idx] == NULL)
		error("CacheSelectBlockToRemove_mru_lru(%d): Called on empty buffer\n", dev_id);
	mru_lru_queues[dev_id_idx].lock_ll.lock();
	Node_LL* node = mru_lru_queues[dev_id_idx].start_iterration();
	while(DevCache[dev_id_idx]->BlockState[node->idx] != AVAILABLE){
		node = mru_lru_queues[dev_id_idx].next_in_line();
		if(node->idx < 0) break;
	}
	if(node->idx >=0 && DevCache[dev_id_idx]->BlockState[node->idx] == AVAILABLE){
		// lprintf(lvl-1, "|-----> CacheSelectBlockToRemove_mru_lru: Invalidating\n");
		mru_lru_queues[dev_id_idx].invalidate(node);
		free(result_node);
		result_node = node;
	}
	mru_lru_queues[dev_id_idx].lock_ll.unlock();

#ifdef CTEST
	cpu_timer = csecond() - cpu_timer;
	lprintf(lvl-1, "CacheSelectBlockToRemove_mru(%d): t_select = %lf\n",dev_id, cpu_timer);
	cpu_timer = csecond();
#endif
#ifdef DEBUG
	lprintf(lvl-1, "<-----|\n");
#endif
	return result_node;
}
#endif

void CacheGetLock(void*){
		while(__sync_lock_test_and_set (&globalock, 1));
}

void CacheReleaseLock(void*){
	__sync_lock_release(&globalock);
}


void CacheCheckCorrect(short dev_id, int BlockIdx){
	short dev_id_idx = idxize(dev_id);
	if (DevCache[dev_id_idx] == NULL)
		error("CacheCheckCorrect(%d,%d): Called on empty buffer\n", dev_id, BlockIdx);
	if (BlockIdx < 0 || BlockIdx >= DevCache[dev_id_idx]->BlockNum)
		error("CacheCheckCorrect(%d,%d): Invalid BlockIdx = %d\n", dev_id, BlockIdx, BlockIdx);
}

state CacheGetBlockStateNoLock(short dev_id, int BlockIdx){
	short dev_id_idx = idxize(dev_id);
	CacheCheckCorrect(dev_id, BlockIdx);
	state result = DevCache[dev_id_idx]->BlockState[BlockIdx];
	return result;
}

state CacheSetBlockState(short dev_id, int BlockIdx, state new_state){
	short dev_id_idx = idxize(dev_id);
	state result = DevCache[dev_id_idx]->BlockState[BlockIdx];
	DevCache[dev_id_idx]->BlockState[BlockIdx] = new_state;
	return result;
}

state CacheUpdateBlockState(short dev_id, int BlockIdx){
	short dev_id_idx = idxize(dev_id);
	state prev_state = DevCache[dev_id_idx]->BlockState[BlockIdx];
	if(DevCache[dev_id_idx]->BlockReaders[BlockIdx] > 0){
		if (prev_state!=R && prev_state!=AVAILABLE)
			error("CacheUpdateBlockState(dev_id=%d, BlockIdx=%d): Block with %d readers has state %s\n",
				dev_id, BlockIdx, DevCache[dev_id_idx]->BlockReaders[BlockIdx],
				print_state(prev_state));
		else 	DevCache[dev_id_idx]->BlockState[BlockIdx] = R;
	}
	else if (prev_state==R && DevCache[dev_id_idx]->BlockReaders[BlockIdx] < 0)
		error("CacheUpdateBlockState(dev_id=%d, BlockIdx=%d): Block with %d readers has state R\n",
					dev_id, BlockIdx, DevCache[dev_id_idx]->BlockReaders[BlockIdx]);
	else if (prev_state==R && DevCache[dev_id_idx]->BlockReaders[BlockIdx] == 0)
		DevCache[dev_id_idx]->BlockState[BlockIdx] = AVAILABLE;
	return prev_state;
}

void CacheInvalidate(void* wrapped_CacheWrap_p){
	short lvl = 5;
	CacheWrap_p unwrap = (CacheWrap_p) wrapped_CacheWrap_p;
	int BlockIdx = unwrap->BlockIdx;
	short dev_id = unwrap->dev_id, dev_id_idx = idxize(unwrap->dev_id);
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	CacheCheckCorrect(dev_id, BlockIdx);
	if(unwrap->lock_flag) while(__sync_lock_test_and_set (&globalock, 1));

	if(DevCache[dev_id_idx]->BlockReaders[BlockIdx]!=0)
			error("CacheInvalidate(dev_id=%d, BlockIdx=%d): Still has %d readers\n",
				dev_id, BlockIdx, DevCache[dev_id_idx]->BlockReaders[BlockIdx]);

	state prev_state = CacheSetBlockState(dev_id, BlockIdx, EMPTY);
	if (prev_state!= AVAILABLE && prev_state!= EMPTY && prev_state!= EXCLUSIVE)
		error("CacheInvalidate(dev_id=%d, BlockIdx=%d) prev_state = %s\n",
			dev_id, BlockIdx, print_state(prev_state));
	if(unwrap->lock_flag) __sync_lock_release(&globalock);
#ifdef DEBUG
  lprintf(lvl, "CacheInvalidate(dev_id=%d, BlockIdx=%d) ran succesfully.\n", dev_id, BlockIdx);
#endif
#ifdef STEST
	total_cache_timer[dev_id_idx]+= csecond();
#endif
}

void CacheStartRead(void* wrapped_CacheWrap_p){
	short lvl = 5;
	CacheWrap_p unwrap = (CacheWrap_p) wrapped_CacheWrap_p;
	int BlockIdx = unwrap->BlockIdx;
	short dev_id = unwrap->dev_id, dev_id_idx = idxize(unwrap->dev_id);
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	CacheCheckCorrect(dev_id, BlockIdx);

	if(unwrap->lock_flag) while(__sync_lock_test_and_set (&globalock, 1));
		DevCache[dev_id_idx]->BlockReaders[BlockIdx]++;
	CacheUpdateBlockState(dev_id, BlockIdx);
	if(unwrap->lock_flag) __sync_lock_release(&globalock);
#ifdef DEBUG
  lprintf(lvl, "CacheStartRead(dev_id=%d, BlockIdx=%d) ran succesfully.\n", dev_id, BlockIdx);
#endif
#ifdef STEST
	total_cache_timer[dev_id_idx]+= csecond();
#endif
}

void CacheEndRead(void* wrapped_CacheWrap_p){
	short lvl = 5;
	CacheWrap_p unwrap = (CacheWrap_p) wrapped_CacheWrap_p;
	int BlockIdx = unwrap->BlockIdx;
	short dev_id = unwrap->dev_id, dev_id_idx = idxize(unwrap->dev_id);
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	CacheCheckCorrect(dev_id, BlockIdx);
	if(unwrap->lock_flag) while(__sync_lock_test_and_set (&globalock, 1));

	if(DevCache[dev_id_idx]->BlockReaders[BlockIdx]==0)
			error("CacheEndRead(dev_id=%d, BlockIdx=%d): Already zero readers\n",
				dev_id, BlockIdx);

	DevCache[dev_id_idx]->BlockReaders[BlockIdx]--;
	CacheUpdateBlockState(dev_id, BlockIdx);
	if(unwrap->lock_flag) __sync_lock_release(&globalock);
#ifdef DEBUG
  lprintf(lvl, "CacheEndRead(dev_id=%d, BlockIdx=%d) ran succesfully.\n", dev_id, BlockIdx);
#endif
#ifdef STEST
	total_cache_timer[dev_id_idx]+= csecond();
#endif
}

void CacheStartWrite(void* wrapped_CacheWrap_p){
	short lvl = 5;
	CacheWrap_p unwrap = (CacheWrap_p) wrapped_CacheWrap_p;
	int BlockIdx = unwrap->BlockIdx;
	short dev_id = unwrap->dev_id, dev_id_idx = idxize(unwrap->dev_id);
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	CacheCheckCorrect(dev_id, BlockIdx);
	if(unwrap->lock_flag) while(__sync_lock_test_and_set (&globalock, 1));
	state prev_state = CacheSetBlockState(dev_id, BlockIdx, EXCLUSIVE);
	if (prev_state!= EMPTY && prev_state!= AVAILABLE)
		error("CacheStartWrite(dev_id=%d, BlockIdx=%d) prev_state = %s\n",
			dev_id, BlockIdx, print_state(prev_state));
	if(unwrap->lock_flag) __sync_lock_release(&globalock);
#ifdef DEBUG
  lprintf(lvl, "CacheStartWrite(dev_id=%d, BlockIdx=%d) ran succesfully.\n", dev_id, BlockIdx);
#endif
#ifdef STEST
	total_cache_timer[dev_id_idx]+= csecond();
#endif
}

void CacheEndWrite(void* wrapped_CacheWrap_p){
	short lvl = 5;
	CacheWrap_p unwrap = (CacheWrap_p) wrapped_CacheWrap_p;
	int BlockIdx = unwrap->BlockIdx;
	short dev_id = unwrap->dev_id, dev_id_idx = idxize(unwrap->dev_id);
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	CacheCheckCorrect(dev_id, BlockIdx);
	if(unwrap->lock_flag) while(__sync_lock_test_and_set (&globalock, 1));
	state prev_state = CacheSetBlockState(dev_id, BlockIdx, EXCLUSIVE);
	if (prev_state!= EXCLUSIVE)
		error("CacheEndWrite(dev_id=%d, BlockIdx=%d) prev_state = %s\n",
			dev_id, BlockIdx, print_state(prev_state));
	if(unwrap->lock_flag) __sync_lock_release(&globalock);
#ifdef DEBUG
  lprintf(lvl, "CacheEndWrite(dev_id=%d, BlockIdx=%d) ran succesfully.\n", dev_id, BlockIdx);
#endif
#ifdef STEST
	total_cache_timer[dev_id_idx]+= csecond();
#endif
}

void CacheStartFetch(void* wrapped_CacheWrap_p){
	short lvl = 5;
	CacheWrap_p unwrap = (CacheWrap_p) wrapped_CacheWrap_p;
	int BlockIdx = unwrap->BlockIdx;
	short dev_id = unwrap->dev_id, dev_id_idx = idxize(unwrap->dev_id);
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	CacheCheckCorrect(dev_id, BlockIdx);
	if(unwrap->lock_flag) while(__sync_lock_test_and_set (&globalock, 1));
	state prev_state = CacheSetBlockState(dev_id, BlockIdx, FETCHING);
	if (prev_state!= EMPTY)
		error("CacheStartFetch(dev_id=%d, BlockIdx=%d) prev_state = %s\n",
			dev_id, BlockIdx, print_state(prev_state));
	if(unwrap->lock_flag) __sync_lock_release(&globalock);
#ifdef DEBUG
	lprintf(lvl, "CacheStartFetch(dev_id=%d, BlockIdx=%d) ran succesfully.\n", dev_id, BlockIdx);
#endif
#ifdef STEST
	total_cache_timer[dev_id_idx]+= csecond();
#endif
}

void CacheEndFetchStartRead(void* wrapped_CacheWrap_p){
	short lvl = 5;
	CacheWrap_p unwrap = (CacheWrap_p) wrapped_CacheWrap_p;
	int BlockIdx = unwrap->BlockIdx;
	short dev_id = unwrap->dev_id, dev_id_idx = idxize(unwrap->dev_id);
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	CacheCheckCorrect(dev_id, BlockIdx);
	if(unwrap->lock_flag) while(__sync_lock_test_and_set (&globalock, 1));
	state prev_state = CacheSetBlockState(dev_id, BlockIdx, AVAILABLE);
	if (prev_state!= FETCHING)
		error("CacheEndFetchStartRead(dev_id=%d, BlockIdx=%d) prev_state = %s\n",
			dev_id, BlockIdx, print_state(prev_state));
	DevCache[dev_id_idx]->BlockReaders[BlockIdx]++;
	CacheUpdateBlockState(dev_id, BlockIdx);
	if(unwrap->lock_flag) __sync_lock_release(&globalock);
	free(wrapped_CacheWrap_p);
#ifdef DEBUG
  lprintf(lvl, "CacheEndFetchStartRead(dev_id=%d, BlockIdx=%d) ran succesfully.\n", dev_id, BlockIdx);
#endif
#ifdef STEST
	total_cache_timer[dev_id_idx]+= csecond();
#endif
}

void CacheEndFetchStartWrite(void* wrapped_CacheWrap_p){
	short lvl = 5;
	CacheWrap_p unwrap = (CacheWrap_p) wrapped_CacheWrap_p;
	int BlockIdx = unwrap->BlockIdx;
	short dev_id = unwrap->dev_id, dev_id_idx = idxize(unwrap->dev_id);
#ifdef STEST
	total_cache_timer[dev_id_idx]-= csecond();
#endif
	CacheCheckCorrect(dev_id, BlockIdx);
	if(unwrap->lock_flag) while(__sync_lock_test_and_set (&globalock, 1));
	state prev_state = CacheSetBlockState(dev_id, BlockIdx, AVAILABLE);
	if (prev_state!= FETCHING)
		error("CacheEndFetchStartWrite(dev_id=%d, BlockIdx=%d) prev_state = %s\n",
			dev_id, BlockIdx, print_state(prev_state));
	prev_state = CacheSetBlockState(dev_id, BlockIdx, EXCLUSIVE);
	if(unwrap->lock_flag) __sync_lock_release(&globalock);
	free(wrapped_CacheWrap_p);
#ifdef DEBUG
  lprintf(lvl, "CacheEndFetchStartWrite(dev_id=%d, BlockIdx=%d) ran succesfully.\n", dev_id, BlockIdx);
#endif
#ifdef STEST
	total_cache_timer[dev_id_idx]+= csecond();
#endif
}

long long CoCoGetBlockSize(short dev_id){
	short lvl = 5;
#ifdef DEBUG
	lprintf(lvl-1, "|-----> CoCoGetBlockSize(dev = %d)\n", dev_id);
#endif
	short dev_id_idx = (dev_id == -1) ? LOC_NUM - 1: dev_id;
	if (DevCache[dev_id_idx] == NULL)
		error("CoCoGetBlockSize(%d): Called on empty buffer\n", dev_id);
	return DevCache[dev_id_idx]->BlockSize;
}

void CoCoPeLiaRequestMaxBuffer(short dev_id, long long block_num, long long block_size, long long bufsize_limit){
  short lvl = 3;
#ifdef DEBUG
	lprintf(lvl-1, "|-----> CoCoPeLiaRequestMaxBuffer(dev_id=%d, block_num = %lld, block_size = %lld, bufsize_limit = %lld)\n",
dev_id, block_num, block_size, bufsize_limit);
#endif
#ifdef TEST
	double cpu_timer = csecond();
#endif
  short dev_id_idx = idxize(dev_id);
	Cache_p temp_DevCache = CoCoPeLiaDevCacheInit(dev_id, block_num, block_size);
	if (temp_DevCache->mem_buf_sz > 0){
	  long long free_dev_mem, max_dev_mem,  prev_DevCache_sz = 0;
		if (DevCache[dev_id_idx] != NULL) prev_DevCache_sz = DevCache[dev_id_idx]->mem_buf_sz;
		CoCoPeLiaDevGetMemInfo(&free_dev_mem, &max_dev_mem);
	  long long problem_avail_mem = free_dev_mem - max_dev_mem*(1-PROBLEM_GPU_PERCENTAGE/100.0) + prev_DevCache_sz;
	  // For debuging large cases
	  if(MEM_LIMIT>=free_dev_mem)
	  	problem_avail_mem=MEM_LIMIT;
	  else
	  	problem_avail_mem=free_dev_mem;

	  #ifdef DEBUG
	  	lprintf(lvl, "====================================\n");
	  	lprintf(lvl, "Buffer mem management:\n");
	  	lprintf(lvl, " -Buffer requested for BlockSize=%zu MB and BlockNum=%d\n", (size_t) temp_DevCache->BlockSize/1024/1024, temp_DevCache->BlockNum);
	  	lprintf(lvl, " -Mem required for matrices: %zu MB\n", (size_t) temp_DevCache->mem_buf_sz/1024/1024);
	    lprintf(lvl, " -Mem available in loc(%d): %zu MB\n", dev_id, (size_t) problem_avail_mem/1024/1024);
	  #endif
		if (bufsize_limit <= 0){
		  if (temp_DevCache->mem_buf_sz <= problem_avail_mem){
#ifdef DEBUG
		    lprintf(lvl, " -Requested buffer fits in loc(%d)\n", dev_id);
#endif
			;}
			else{
		    temp_DevCache->BlockNum =  (int) (problem_avail_mem/temp_DevCache->BlockSize);
				temp_DevCache->mem_buf_sz = temp_DevCache->BlockNum*temp_DevCache->BlockSize;
#ifdef DEBUG
		    lprintf(lvl, " -Requested buffer does not fit in loc(%d)\n", dev_id);
#endif
			}
	}
	else{
		if(bufsize_limit > problem_avail_mem)
			error("CoCoPeLiaRequestBuffer(dev_id=%d): Requested cache with bufsize_limit =%zu MB bigger than problem_avail_mem = %zu MB\n",
				dev_id, (size_t) bufsize_limit/1024/1024, (size_t) problem_avail_mem/1024/1024);
		else if(bufsize_limit < temp_DevCache->BlockSize)
					error("CoCoPeLiaRequestBuffer(dev_id=%d): Requested cache with bufsize_limit =%zu MB smaller than Blocksize = %zu MB\n",
						dev_id, (size_t) bufsize_limit/1024/1024, (size_t) temp_DevCache->BlockSize/1024/1024);
		else{
			temp_DevCache->BlockNum =  (int) (bufsize_limit/temp_DevCache->BlockSize);
			temp_DevCache->mem_buf_sz = temp_DevCache->BlockNum*temp_DevCache->BlockSize;
		}
	}
#if CACHE_SCHEDULING_POLICY==2 || CACHE_SCHEDULING_POLICY==3 // Memory for hash
	mru_lru_hash[dev_id_idx] = (Node_LL**) malloc(temp_DevCache->BlockNum * sizeof(Node_LL*));
#endif

	#ifdef DEBUG
	  if (prev_DevCache_sz >= temp_DevCache->mem_buf_sz)
			lprintf(lvl, " -Loc(%d) buf available: %zu MB\n", dev_id, (size_t) prev_DevCache_sz/1024/1024);
	  else if (prev_DevCache_sz > 0) lprintf(lvl, " -Smaller Loc(%d) buf available -> replacing : %zu -> %zu MB\n",
			dev_id, (size_t) prev_DevCache_sz/1024/1024, (size_t) temp_DevCache->mem_buf_sz/1024/1024);
	  else lprintf(lvl, " -Initializing new Loc(%d) buffer: %zu MB\n", dev_id, (size_t) temp_DevCache->mem_buf_sz/1024/1024);
	#endif
	  if (prev_DevCache_sz >= temp_DevCache->mem_buf_sz){
			temp_DevCache->mem_buf_sz = DevCache[dev_id_idx]->mem_buf_sz;
			temp_DevCache->gpu_mem_buf = DevCache[dev_id_idx]->gpu_mem_buf;
			free(DevCache[dev_id_idx]->BlockState);
			free(DevCache[dev_id_idx]->BlockReaders);
			free(DevCache[dev_id_idx]->BlockCurrentTileDim);
			free(DevCache[dev_id_idx]->BlockCurrentTilePtr);
			free(DevCache[dev_id_idx]);
			DevCache[dev_id_idx] = temp_DevCache;
		}
	  else{
			if (prev_DevCache_sz > 0){
			  CoCoFree(DevCache[dev_id_idx]->gpu_mem_buf, dev_id);
				free(DevCache[dev_id_idx]->BlockState);
				free(DevCache[dev_id_idx]->BlockReaders);
				free(DevCache[dev_id_idx]->BlockCurrentTileDim);
				free(DevCache[dev_id_idx]->BlockCurrentTilePtr);
				free(DevCache[dev_id_idx]);
			}
	    //DevCache[dev_id_idx] = temp_DevCache;
			//DevCache[dev_id_idx]->gpu_mem_buf = CoCoMalloc(DevCache[dev_id_idx]->mem_buf_sz,
			//	dev_id);
			temp_DevCache->gpu_mem_buf = CoCoMalloc(temp_DevCache->mem_buf_sz, dev_id);
			DevCache[dev_id_idx] = temp_DevCache;
	  }
	  CoCoSyncCheckErr();

#ifdef TEST
		cpu_timer = csecond() - cpu_timer;
		lprintf(lvl, "GPU(%d) Buffer allocation with sz = %zu MB: t_alloc = %lf ms\n" ,
	    dev_id, (size_t) DevCache[dev_id_idx]->mem_buf_sz/1024/1024, cpu_timer*1000);
#endif

#ifdef DEBUG
		lprintf(lvl, "GPU(%d) Buffer allocation (Size = %zu MB, Blocksize = %zu MB, BlockNum = %d)\n" ,
	  dev_id, (size_t) DevCache[dev_id_idx]->mem_buf_sz/1024/1024,
	  (size_t) DevCache[dev_id_idx]->BlockSize/1024/1024,  DevCache[dev_id_idx]->BlockNum);
		lprintf(lvl-1, "<-----|\n");
#endif
	}
	else{;
#ifdef DEBUG
		lprintf(lvl, "GPU(%d) does not require a buffer - all data available\n" , dev_id);
		lprintf(lvl-1, "<-----|\n");
#endif
	}
}


const char *printlist(state *list, int length){
	char* outstring = (char*) malloc(length*64*sizeof(char));
  sprintf(outstring, "[");
  for (int i =0; i < length; i++) sprintf(outstring + strlen(outstring), " %s", print_state(list[i]));
	sprintf(outstring + strlen(outstring), " ]");
  return outstring;
}

void CachePrint(short dev_id){
	short dev_id_idx = idxize(dev_id);
	if (DevCache[dev_id_idx] == NULL)
		error("CachePrint(%d): Called on empty buffer\n", dev_id);
	lprintf(0, "Cache(dev_id=%d):\n%s\n%s\n", dev_id,
		printlist(DevCache[dev_id_idx]->BlockReaders, DevCache[dev_id_idx]->BlockNum),
		printlist(DevCache[dev_id_idx]->BlockState, DevCache[dev_id_idx]->BlockNum));

}

#ifdef STEST
double CacheGetTimer(short dev_id){
	return total_cache_timer[idxize(dev_id)];
}
#endif
*/
