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

void CacheBlock::remove_reader(bool lockfree){
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

void CacheBlock::remove_writer(bool lockfree){
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
	CBlock_unwraped->CBlock->remove_reader(CBlock_unwraped->lockfree);
	return NULL;
}

void* CBlock_RW_wrap(void* CBlock_wraped){
	//TODO: include lock flag
	CBlock_wrap_p CBlock_unwraped = (CBlock_wrap_p) CBlock_wraped;
	CBlock_unwraped->CBlock->remove_writer(CBlock_unwraped->lockfree);
	return NULL;
}

void* CBlock_RESET_wrap(void* CBlock_wraped){
	//TODO: include lock flag
	CBlock_wrap_p CBlock_unwraped = (CBlock_wrap_p) CBlock_wraped;
	CBlock_unwraped->CBlock->reset(CBlock_unwraped->lockfree, true); //TODO: second lock must be set depending on what forceReset does
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
