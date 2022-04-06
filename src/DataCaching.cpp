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

LinkedList::LinkedList(Cache_p cache, std::string name){
	short lvl = 2;
	if(cache==NULL)
		error("LinkedList::LinkedList(): Creating cache that doesn't belong to a cache.\n");
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::LinkedList(name=%s):\n", cache->dev_id, Name.c_str());
#endif
	Parent = cache;
	Name = name;
	start = NULL;
	end = NULL;
	length = 0;
	iter = NULL;
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::LinkedList(name=%s)\n", Parent->dev_id, Name.c_str());
#endif
}

LinkedList::~LinkedList(){
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::~LinkedList(name=%s)\n", Parent->dev_id, Name.c_str());
#endif
	Node_LL_p tmp;
	while(start != NULL){
		tmp = start;
		start = tmp->next;
		delete tmp;
	}
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::~LinkedList(name=%s)\n", Parent->dev_id, Name.c_str());
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
						\n||        Name         | %s\
						\n|| - - - - - - - - - - - - - - - - - - - -\
						\n||       Length        | %d\
						\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", Parent->id, Parent->dev_id, Name.c_str(), length);
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
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::invalidate(name=%s, node_id=%d)\n", Parent->dev_id, Name.c_str(), node->idx);
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
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::invalidate(name=%s, node_id=%d)\n", Parent->dev_id, Name.c_str(), node->idx);
#endif
}

void LinkedList::push_back(int idx, bool lockfree){
	// Pushes an element in the back of the queue.
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::push_back(name=%s, idx=%d)\n", Parent->dev_id, Name.c_str(), idx);
#endif
	if(!lockfree)
		lock();
	if(idx >= Parent->BlockNum)
		error("[dev_id=%d] LinkedList::push_back(name=%s): Index given(%d) is larger than the number of blocks(%d).\n", Parent->dev_id, Name.c_str(), idx, Parent->BlockNum);
	else if(length > Parent->BlockNum)
		error("[dev_id=%d] LinkedList::push_back(name=%s): Called to put another element but max length is reached with BlockNum=%d and length=%d\n", Parent->dev_id, Name.c_str(), Parent->BlockNum, length);
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
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::push_back(name=%s, idx=%d)\n", Parent->dev_id, Name.c_str(), idx);
#endif

}


Node_LL_p LinkedList::start_iterration(){
	// Returns the first valid element without removing it.
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::start_iterration(name=%s)\n", Parent->dev_id, Name.c_str());
#endif
	if(start == NULL) // Queue is empty.
		error("[dev_id=%d] LinkedList::start_iterration(name=%s): Fifo queue is empty. Can't pop element.\n", Parent->dev_id, Name.c_str());
	else{ // Queue not empty.
		iter = start;
		while(iter != NULL && !iter->valid)
			iter = iter->next;
		if(iter == NULL){
			Node_LL_p tmp_node = new Node_LL();
			tmp_node->idx = -1;
		#ifdef CDEBUG
			lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::start_iterration(name=%s)\n", Parent->dev_id, Name.c_str());
		#endif
			return tmp_node;
		}
		else{
		#ifdef CDEBUG
			lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::start_iterration(name=%s)\n", Parent->dev_id, Name.c_str());
		#endif
			return iter;
		}
	}
}

Node_LL_p LinkedList::next_in_line(){
	// Returns next element in iterration.
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::next_in_line(name=%s)\n", Parent->dev_id, Name.c_str());
#endif
	if(iter == NULL)
		error("[dev_id=%d] LinkedList::next_in_line(name=%s): Iterration not started. Call check_first.\n", Parent->dev_id, Name.c_str());
	else{
		iter = iter->next;
		while(iter != NULL && !iter->valid)
			iter = iter->next;
		if(iter == NULL){
			Node_LL_p tmp_node = new Node_LL();
			tmp_node->idx = -1;
			return tmp_node;
		#ifdef CDEBUG
			lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::next_in_line(name=%s)\n", Parent->dev_id, Name.c_str());
		#endif
		}
		else{
		#ifdef CDEBUG
			lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::next_in_line(name=%s)\n", Parent->dev_id, Name.c_str());
		#endif
			return iter;
		}
	}
}

Node_LL_p LinkedList::remove(Node_LL_p node, bool lockfree){
	// Removes first element.
	short lvl = 2;

#ifdef CDEBUG
	if(node!=NULL)
		lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::remove(name=%s, idx=%d)\n", Parent->dev_id, Name.c_str(), node->idx);
#endif

	if(node == NULL) // Node not found.
		error("[dev_id=%d] LinkedList::remove(name=%s): Input node not found. Can't pop element.\n", Parent->dev_id, Name.c_str());
	else if(node->idx >= Parent->BlockNum)
		error("[dev_id=%d] LinkedList::remove(name=%s): Index of given node(%d) is larger than the number of blocks(%d).\n", Parent->dev_id, Name.c_str(), node->idx, Parent->BlockNum);
	else{
		if(!lockfree)
			lock();
		if(node!=start)
			(node->previous)->next = node->next;
		else{
			start = node->next;
			if(start != NULL)
				start->previous = NULL;
		}
		if(node!=end)
			(node->next)->previous = node->previous;
		else{
			end = node->previous;
			// node != start
			if(end != NULL){
				end->next = NULL;
			}
		}
		node->next = NULL;
		node->previous = NULL;
		length--;
		if(!lockfree)
			unlock();
#ifdef CDEBUG
		lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::remove(name=%s, idx=%d)\n", Parent->dev_id, Name.c_str(), node->idx);
#endif
		return node;
	}
}

void LinkedList::put_first(Node_LL* node, bool lockfree){
	// Puts the element first on the list.
	short lvl = 2;

#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::put_first(name=%s, idx=%d)\n", Parent->dev_id, Name.c_str(), node->idx);
#endif

	if(node == NULL)
		error("[dev_id=%d] LinkedList::put_first(name=%s): Called to put a node that doesn't exist.\n", Parent->dev_id, Name.c_str());
	else{
		if(!lockfree){
			lock();
		}
		// Add it to the new queue
		node->next = start;
		if(length > 0)
			start->previous = node;
		else
			end = node;
		start = node;
		length++;
		if(!lockfree){
			unlock();
		}
	}
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::put_first(name=%s, idx=%d)\n", Parent->dev_id, Name.c_str(), node->idx);
#endif
}

void LinkedList::put_last(Node_LL_p node, bool lockfree){
	// Takes a node and puts it in the end of the list.
	short lvl = 2;

#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] LinkedList::put_last(name=%s, idx=%d)\n", Parent->dev_id, Name.c_str(), node->idx);
#endif

	if(node == NULL)
		error("[dev_id=%d] LinkedList::put_last(name=%s): Called to put a node that doesn't exist.\n", Parent->dev_id, Name.c_str());
	else{
		if(!lockfree){
			lock();
		}
		// Add it to the new queue
		node->previous = end;
		if(length > 0)
			end->next = node;
		else
			start = node;
		end = node;
		length++;
		if(!lockfree){
			unlock();
		}
	}
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] LinkedList::put_last(name=%s, idx=%d)\n", Parent->dev_id, Name.c_str(), node->idx);
#endif
}


void LinkedList::lock(){
	while(__sync_lock_test_and_set(&lock_ll, 1));
	// Lock++;
	// Lock.lock();
}

void LinkedList::unlock(){
	   __sync_lock_release(&lock_ll);
	// Lock--;
}

bool LinkedList::is_locked(){
	if(lock_ll==0)
		return false;
	return true;
}

#endif

const char* print_state(state in_state){
	switch(in_state){
		case(INVALID):
			return "INVALID";
		case(NATIVE):
			return "NATIVE";
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

	short lvl = 2;

	if(block_parent!=NULL && block_id>=0 && block_id<block_parent->BlockNum && block_size>0){
	#ifdef CDEBUG
		lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::CacheBlock(id=%d, Parent_id=%d, Size=%llu)\n", block_parent->dev_id, block_id, block_parent->id, block_size);
	#endif
		Lock = 0;
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
		Available = new Event(Parent->dev_id);
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
	short lvl = 2;
	// Destructor of the block.
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::~CacheBlock()\n", Parent->dev_id);
#endif
	lock();
	//reset(true, true);
	//
	if(Owner_p){
		*Owner_p = NULL;
		Owner_p = NULL;
	}
	if(State != NATIVE){
		CoCoFree(Adrs, Parent->dev_id);
		delete Available;
#ifdef CDEBUG
		lprintf(lvl-1, "------- [dev_id=%d] CacheBlock::~CacheBlock(): Deleting non-NATIVE block id =%d\n",
			Parent->dev_id, id);
#endif
	}
	else{;
#ifdef CDEBUG
		lprintf(lvl-1, "------- [dev_id=%d] CacheBlock::~CacheBlock(): Refrain from deleting NATIVE block id =%d\n",
			Parent->dev_id, id);
#endif
	}
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
	if(State==SHARABLE || State==EXCLUSIVE || State == NATIVE)
		PendingReaders++;
	else if(State==AVAILABLE){
		PendingReaders++;
		set_state(SHARABLE, true);
	}
	else
		error("[dev_id=%d] CacheBlock::add_reader(): Can't add reader. Block has State=%s\n", Parent->dev_id, print_state(State));
// #if defined(MRU)
// 	Parent->Queue->put_first(Parent->Hash[id]);
// #elif defined(LRU)
// 	Parent->Queue->put_last(Parent->Hash[id]);
// #endif
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
	if(State==EXCLUSIVE || State==NATIVE)
		PendingWriters++;
	else if(State==AVAILABLE || State==SHARABLE){
		PendingWriters++;
		set_state(EXCLUSIVE, true);
	}
	else
		error("[dev_id=%d] CacheBlock::add_reader(): Can't add reader. Block has State=%s\n", Parent->dev_id, print_state(State));
// #if defined(MRU)
// 	Parent->Queue->put_first(Parent->Hash[id]);
// #elif defined(LRU)
// 	Parent->Queue->put_last(Parent->Hash[id]);
// #endif
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
	if(!lockfree)
		lock();
	if(PendingReaders.load()>0)
		PendingReaders--;
	else
		error("[dev_id=%d] CacheBlock::remove_reader(): Can't remove reader. There are none.\n", Parent->dev_id);
#if defined(MRU)
	Node_LL_p node = Parent->Queue->remove(Parent->Hash[id]);
	Parent->Queue->put_first(node);
#elif defined(LRU)
	Node_LL_p node = Parent->Queue->remove(Parent->Hash[id]);
	Parent->Queue->put_last(node);
#endif
	if(!lockfree)
		unlock();
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::remove_reader(block_id=%d)\n", Parent->dev_id, id);
#endif
}

void CacheBlock::remove_writer(bool lockfree){
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::remove_writer(block_id=%d)\n", Parent->dev_id, id);
#endif
	if(!lockfree)
		lock();
	if(PendingWriters.load()>0)
		PendingWriters--;
	else
		error("[dev_id=%d] CacheBlock::remove_writer(): Can't remove writer. There are none.\n", Parent->dev_id);
#if defined(MRU)
	Node_LL_p node = Parent->Queue->remove(Parent->Hash[id]);
	Parent->Queue->put_first(node);
#elif defined(LRU)
	Node_LL_p node = Parent->Queue->remove(Parent->Hash[id]);
	Parent->Queue->put_last(node);
#endif
	if(!lockfree)
		unlock();
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheBlock::remove_writer(block_id=%d)\n", Parent->dev_id, id);
#endif
}

void* CBlock_RR_wrap(void* CBlock_wraped){
	CBlock_wrap_p CBlock_unwraped = (CBlock_wrap_p) CBlock_wraped;
	CBlock_unwraped->CBlock->remove_reader(CBlock_unwraped->lockfree);
	return NULL;
}

void* CBlock_RW_wrap(void* CBlock_wraped){
	CBlock_wrap_p CBlock_unwraped = (CBlock_wrap_p) CBlock_wraped;
	CBlock_unwraped->CBlock->remove_writer(CBlock_unwraped->lockfree);
	return NULL;
}

void* CBlock_INV_wrap(void* CBlock_wraped){
	CBlock_wrap_p CBlock_unwraped = (CBlock_wrap_p) CBlock_wraped;
	CBlock_unwraped->CBlock->reset(CBlock_unwraped->lockfree, true); //TODO: second lock must be set depending on what forceReset does
	CBlock_unwraped->CBlock->set_state(INVALID, CBlock_unwraped->lockfree);
	return NULL;
}

void CacheBlock::set_owner(void** owner_adrs, bool lockfree){
	short lvl = 2;
	#ifdef CDEBUG
		lprintf(lvl-1, "|-----> CacheBlock::set_owner(owner_adrs=%p)\n", owner_adrs);
	#endif

	if(!lockfree)
		lock();
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

void CacheBlock::allocate(bool lockfree){
	// Allocates a cache block if not already pointing to some memory (not null!)
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl, "|-----> [dev_id=%d] CacheBlock::allocate(block_id=%d)\n", Parent->dev_id, id);
#endif
	if(Adrs == NULL){
		Adrs = CoCoMalloc(Size, Parent->dev_id);
#ifdef CDEBUG
		lprintf(lvl, "------- [dev_id=%d] CacheBlock::allocate(block_id=%d): Allocated Adrs = %p\n", Parent->dev_id, id, Adrs);
#endif
	}
	else{
		#ifdef CDEBUG
			lprintf(lvl, "------- [dev_id=%d] CacheBlock::allocate(block_id=%d) -> Supposedly already allocated block...", Parent->dev_id, id);
		#endif
	}
#ifdef CDEBUG
	lprintf(lvl, "<-----| [dev_id=%d] CacheBlock::allocate(block_id=%d)\n", Parent->dev_id, id);
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
	lprintf(lvl-1, "|-----> [dev_id=%d] CacheBlock::set_state(block_id=%d, prior_state=%s)\n", Parent->dev_id, id, print_state(State));
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
	while(__sync_lock_test_and_set(&Lock, 1));
	// Lock++;
	// Lock.lock();
}

void CacheBlock::unlock(){
	__sync_lock_release(&Lock);
	// Lock--;
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
  Lock = 0;
	id = DevCache_ctr++;
	dev_id = dev_id_in;
	BlockSize = block_size;
	SerialCtr = 0;
	BlockNum = block_num;
	Size = BlockSize*BlockNum;
	Blocks =  (CBlock_p*) malloc (BlockNum * sizeof(CBlock_p));
	for (int idx = 0; idx < BlockNum; idx++) Blocks[idx] = new CacheBlock(idx, this, BlockSize); // Or NULL here and initialize when requested? not sure
#if defined(FIFO)
	InvalidQueue = new LinkedList(this, "InvalidQueue");
	for(int idx = 0; idx < BlockNum; idx++) InvalidQueue->push_back(idx);
	Queue = new LinkedList(this, "Queue");
#elif defined(MRU) || defined(LRU)
	Hash = (Node_LL_p*) malloc(BlockNum * sizeof(Node_LL_p));
	InvalidQueue = new LinkedList(this, "InvalidQueue");
	for(int idx = 0; idx < BlockNum; idx++){
		InvalidQueue->push_back(idx);
		Hash[idx] = InvalidQueue->end;
	}
	Queue = new LinkedList(this, "Queue");
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
	delete InvalidQueue;
	delete Queue;
#elif defined(MRU) || defined(LRU)
	free(Hash);
	delete InvalidQueue;
	delete Queue;
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
		lprintf(lvl-1, "|| Start of Queue with Invalid Blocks ||\
							\n=======================================\n");
		InvalidQueue->draw_queue();
		lprintf(lvl-1, "============================\
							\n|| End of Queue with Invalid Blocks ||\
							\n================================================================================\n");
		lprintf(lvl-1, "|| Start of Queue with Valid Blocks ||\
							\n=======================================\n");
		Queue->draw_queue();
		lprintf(lvl-1, "============================\
							\n|| End of Queue with Valid Blocks ||\
							\n================================================================================\n");
		#endif
	}
	lprintf(lvl-1, "\n");
	if(!lockfree)
		unlock();
}

void Cache::allocate(bool lockfree){
	// Allocates all cacheblocks in cache
	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] Cache::allocate()\n", dev_id);
#endif
	for(int i=0; i<BlockNum; i++)
		if(Blocks[i]!=NULL) Blocks[i]->allocate(true);
		else error("[dev_id=%d] Cache::allocate() -> Blocks[%d] was NULL\n", dev_id, i);
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] Cache::allocate()\n", dev_id);
#endif
}

CBlock_p Cache::assign_Cblock(){
	// Assigns a block from cache to be used for memory.

	short lvl = 2;
#ifdef CDEBUG
	lprintf(lvl-1, "|-----> [dev_id=%d] Cache::assign_Cblock()\n", dev_id);
#endif

	CBlock_p result = NULL;
	lock(); // Lock cache
#if defined(NAIVE)
	if (SerialCtr >= BlockNum){
#endif
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
			// Queue->lock();
			// int remove_block_idx = Queue->remove(remove_block, true);
			// Queue->push_back(remove_block_idx, true);
			// Queue->unlock();
			Queue->put_last(remove_block);
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
#if defined(NAIVE)
	}
	else{
		result = Blocks[SerialCtr];
		result->reset(true,false);
// #if defined(FIFO)
// 		Queue->push_back(SerialCtr);
// #elif defined(MRU)
// 		Queue->lock();
// 		Queue->push_back(SerialCtr, true);
// 		Hash[SerialCtr] = Queue->end;
// 		Queue->put_first(Hash[SerialCtr], true);
// 		Queue->unlock();
// #elif defined(LRU)
// 		Queue->lock();
// 		Queue->push_back(SerialCtr, true);
// 		Hash[SerialCtr] = Queue->end;
// 		Queue->unlock();
// #endif
		SerialCtr++;
		unlock(); // Unlock cache
	}
#endif
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] Cache::assign_Cblock()\n", dev_id);
#endif
  return result;
}

void Cache::lock(){
	while(__sync_lock_test_and_set(&Lock, 1));
	// Lock++;
	// Lock.lock();
}

void Cache::unlock(){
	__sync_lock_release(&Lock);
	// Lock--;
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
			result_idx = idx;
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
	Node_LL_p result_node;
	if(cache->InvalidQueue->length > 0){
		result_node = cache->InvalidQueue->remove(cache->InvalidQueue->start);
		cache->Blocks[result_node->idx]->set_state(INVALID);
	}
	else{
		result_node = new Node_LL();
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
				result_node = cache->Queue->remove(node, true);
				cache->Blocks[result_node->idx]->set_state(INVALID, true);
				// cache->Blocks[idx]->unlock();
			#ifdef CDEBUG
				lprintf(lvl-1, "------- [dev_id=%d] CacheSelectBlockToRemove_fifo(): Found available block. Invalidated.\n",cache->dev_id);
			#endif
			}
			cache->Blocks[result_node->idx]->unlock();
		}
		cache->Queue->unlock();
	}
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

	Node_LL_p result_node;
	if(cache->InvalidQueue->length > 0){
		result_node = cache->InvalidQueue->remove(cache->InvalidQueue->start);
		cache->Blocks[result_node->idx]->set_state(INVALID);
	}
	else{
		result_node = new Node_LL();
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
				result_node = cache->Queue->remove(node, true);
				cache->Blocks[result_node->idx]->set_state(INVALID, true);
				// cache->Blocks[idx]->unlock();
			#ifdef CDEBUG
				lprintf(lvl-1, "------- [dev_id=%d] CacheSelectBlockToRemove_mru_lru(): Found available block. Invalidated.\n",cache->dev_id);
			#endif
			}
			cache->Blocks[result_node->idx]->unlock();
		}
		cache->Queue->unlock();
	}
#ifdef CDEBUG
	lprintf(lvl-1, "<-----| [dev_id=%d] CacheSelectBlockToRemove_mru_lru()\n",cache->dev_id);
#endif
	return result_node;
}
#endif
