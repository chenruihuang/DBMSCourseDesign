#pragma warning(disable:4996)
#include "b_node.h"
#include "b_tree.h"
#include "block_file.h"
#include "LItem.h"
#include <cstring>
#include <cassert>

// -----------------------------------------------------------------------------
//  BTree: 建立在Lj集合上的B+树
// -----------------------------------------------------------------------------
BTree::BTree() // constructor
{
	root_ = -1;
	file_ = nullptr;
	root_ptr_ = nullptr;
}

// -----------------------------------------------------------------------------
BTree::~BTree() // destructor
{
	if (root_ptr_ != nullptr)
	{ // release <root_ptr_>
		delete root_ptr_;
		root_ptr_ = nullptr;
	}

	char* header = new char[file_->get_blocklength()];
	write_header(header); // write <root_> to <header>
	file_->set_header(header); // write back to disk

	delete[] header;
	header = nullptr;

	if (file_ != nullptr)
	{ // release <file_>
		delete file_;
		file_ = nullptr;
	}
}

// -----------------------------------------------------------------------------
void BTree::init(		// init a new tree
	char* fname, 		// file name
	int b_length) 		// block length
{
	FILE* fp = fopen(fname, "r");
	if (fp)
	{ // check whether the file exist
		fclose(fp); // ask whether replace?
//		printf("The file \"%s\" exists. Replace? (y/n)", fname);
//
//		char c = getchar(); // input 'Y' or 'y' or others
//		getchar(); // input <ENTER>
//		if (c != 'y' && c != 'Y')
//		{ // if not remove existing file
//			error("", true); // program will be stopped.
//		}
		remove(fname); // otherwise, remove existing file
	}
	// init <file>, b-tree store here
	file_ = new BlockFile(fname, b_length);

	// -------------------------------------------------------------------------
	//  Init the first node: to store <blocklength> (page size of a node),
	//  <number> (number of nodes including both index node and leaf node), 
	//  and <root> (address of root node)
	// -------------------------------------------------------------------------
	root_ptr_ = new BIndexNode();
	root_ptr_->init(0, this);
	root_ = root_ptr_->get_block();
	delete_root();
}

// -----------------------------------------------------------------------------
void BTree::init_restore(	// load the tree from a tree file
	char* fname) 			// file name
{
	FILE* fp = fopen(fname, "r"); // check whether the file exists
	if (!fp) {
		printf("tree file %s does not exist\n", fname);
		delete[] fname;
		fname = nullptr;
		error("", true);
	}
	fclose(fp);

	// -------------------------------------------------------------------------
	//  It doesn't matter to initialize blocklength to 0.
	//  After reading file, <blocklength> will be reinitialized by file.
	// -------------------------------------------------------------------------
	// init <file>
	file_ = new BlockFile(fname, 0);
	root_ptr_ = nullptr; // init <root_ptr>

	// -------------------------------------------------------------------------
	//  Read the content after first 8 bytes of first block into <header>
	// -------------------------------------------------------------------------
	char* header = new char[file_->get_blocklength()];
	file_->read_header(header); // read remain bytes from header
	read_header(header); // init <root> from <header>

	if (header != nullptr)
	{ // release space
		delete[] header;
		header = nullptr;
	}
}

// -----------------------------------------------------------------------------
int BTree::read_header(// read <root> from buffer
	char* buf) // buffer
{
	memcpy(&root_, buf, SIZEINT);
	return SIZEINT;
}

// -----------------------------------------------------------------------------
int BTree::write_header(// write <root> to buffer
	char* buf) const // buffer (return)
{
	memcpy(buf, &root_, SIZEINT);
	return SIZEINT;
}

// -----------------------------------------------------------------------------
void BTree::load_root() // load <root_ptr> of b-tree
{
	if (root_ptr_ == nullptr)
	{
		root_ptr_ = new BIndexNode();
		root_ptr_->init_restore(this, root_);
	}
}

// -----------------------------------------------------------------------------
void BTree::delete_root() // delete <root_ptr>
{
	if (root_ptr_ != nullptr)
	{
		delete root_ptr_;
		root_ptr_ = nullptr;
	}
}

// -----------------------------------------------------------------------------
int BTree::bulkload(	// bulkload a tree from memory
	LItem* Ltable, 		// Lj table
	int n) 				// number of entries
{
	BIndexNode* index_child = nullptr;
	BIndexNode* index_prev_nd = nullptr;
	BIndexNode* index_act_nd = nullptr;

	BLeafNode* leaf_child = nullptr;
	BLeafNode* leaf_prev_nd = nullptr;
	BLeafNode* leaf_act_nd = nullptr;

	int index = -1;
	int block = -1;
	float value = MINREAL;

	bool first_node = false; 	// determine relationship of sibling
	int start_block = -1;		// position of first node
	int end_block = -1;			// position of last node

	int current_level = -1; 	// current level (leaf level is 0)
	int last_start_block = -1; 	// to build b-tree level by level
	int last_end_block = -1; 	// to build b-tree level by level

	// -------------------------------------------------------------------------
	//  Build leaf node from <LItem table> (level = 0)
	// -------------------------------------------------------------------------
	start_block = 0;
	end_block = 0;
	first_node = true;

	//  存储level 0数据 建立叶子节点间的双向链表
	for (int i = 0; i < n; i++) {

		index = Ltable[i].getIndex();
		value = Ltable[i].getValue();

		if (!leaf_act_nd) {
			leaf_act_nd = new BLeafNode();
			leaf_act_nd->init(0, this);      //  level 0, linked to this tree

			if (first_node) {
				// init <start_block>
				first_node = false; 
				start_block = leaf_act_nd->get_block();
			} else { 
				// label sibling
				leaf_act_nd->set_left_sibling(leaf_prev_nd->get_block());
				leaf_prev_nd->set_right_sibling(leaf_act_nd->get_block());

				delete leaf_prev_nd;
				leaf_prev_nd = nullptr;
			}
			end_block = leaf_act_nd->get_block();
		}     
		// add new entry
		leaf_act_nd->add_new_child(index, value);

		if (leaf_act_nd->isFull()) {
			// change next node to store entries
			leaf_prev_nd = leaf_act_nd;
			leaf_act_nd = nullptr;
		}
	}
	if (leaf_prev_nd != nullptr) { 
		// release the space
		delete leaf_prev_nd;
		leaf_prev_nd = nullptr;
	}
	if (leaf_act_nd != nullptr) {
		delete leaf_act_nd;
		leaf_act_nd = nullptr;
	}

	// -------------------------------------------------------------------------
	//  Stop consition: lastEndBlock == lastStartBlock (only one node, as root)
	// -------------------------------------------------------------------------
	current_level = 1; // build the b-tree level by level
	last_start_block = start_block;
	last_end_block = end_block;

	//  建立索引节点
	while (last_end_block > last_start_block) {
		first_node = true;
		for (int i = last_start_block; i <= last_end_block; i++) {
			block = i;
			// get <block>
			if (current_level == 1) {
				leaf_child = new BLeafNode();
				leaf_child->init_restore(this, block);
				value = leaf_child->get_key_of_node();

				delete leaf_child;
				leaf_child = nullptr;
			} else {
				index_child = new BIndexNode();
				index_child->init_restore(this, block);
				value = index_child->get_key_of_node();

				delete index_child;
				index_child = nullptr;
			}

			if (!index_act_nd) {
				index_act_nd = new BIndexNode();
				index_act_nd->init(current_level, this);

				if (first_node) {
					first_node = false;
					start_block = index_act_nd->get_block();
				} else {
					index_act_nd->set_left_sibling(index_prev_nd->get_block());
					index_prev_nd->set_right_sibling(index_act_nd->get_block());

					delete index_prev_nd;
					index_prev_nd = nullptr;
				}
				end_block = index_act_nd->get_block();
			} // add new entry
			index_act_nd->add_new_child(value, block);

			if (index_act_nd->isFull()) {
				index_prev_nd = index_act_nd;
				index_act_nd = nullptr;
			}
		}
		if (index_prev_nd != nullptr) {
			// release the space
			delete index_prev_nd;
			index_prev_nd = nullptr;
		}
		if (index_act_nd != nullptr) {
			delete index_act_nd;
			index_act_nd = nullptr;
		}
		// update info
		last_start_block = start_block;
		last_end_block = end_block; // build b-tree of higher level
		current_level++;
	}
	root_ = last_start_block; // update the <root>

	//  释放内存空间
	if (index_prev_nd != nullptr) {
		delete index_prev_nd;
		index_prev_nd = nullptr;
	}
	if (index_act_nd != nullptr) {
		delete index_act_nd;
		index_act_nd = nullptr;
	}
	if (index_child != nullptr) {
		delete index_child;
		index_child = nullptr;
	}
	if (leaf_prev_nd != nullptr) {
		delete leaf_prev_nd;
		leaf_prev_nd = nullptr;
	}
	if (leaf_act_nd != nullptr) {
		delete leaf_act_nd;
		leaf_act_nd = nullptr;
	}
	if (leaf_child != nullptr) {
		delete leaf_child;
		leaf_child = nullptr;
	}

	return 0; // success to return
}

void BTree::searchLowerAndHigher(float query,
							 	 BLeafNode* & lower, int & lowerIndex,
							     BLeafNode* & higher, int & higherIndex) {
	//  You should use init the b+tree before use this method
	load_root();
	BNode *searchRoot = root_ptr_;
	assert(searchRoot != nullptr);
	int entries;
	while (true) {
		entries = searchRoot->get_num_entries();
		for (int i = 0; i < entries; ++i) {
			if ((i == entries - 1) || (query < searchRoot->get_key(i + 1))) {	
				if (searchRoot->get_level() > 1) {
					int nextNode = dynamic_cast<BIndexNode*>(searchRoot)->get_son(i);
					searchRoot = new BIndexNode();
					searchRoot->init_restore(this, nextNode);
					break;
				} else if (searchRoot->get_level() == 1) {
					int nextNode = dynamic_cast<BIndexNode*>(searchRoot)->get_son(i);
					if (searchRoot != nullptr) {
						delete searchRoot;
						searchRoot = nullptr;
					}
					searchRoot = new BLeafNode();
					searchRoot->init_restore(this, nextNode);
					break;
				} else if (searchRoot->get_level() == 0) {
					BLeafNode *high = dynamic_cast<BLeafNode*>(searchRoot);
					BLeafNode *low = high->get_left_sibling();
					higherIndex = 0;
					if (low == nullptr)	{
						lowerIndex = -1;
					} else {
						bool found = false;
						for (higherIndex = 1; higherIndex < low->get_num_entries(); ++higherIndex) {
							if (low->get_key(higherIndex) >= query) {
								found = true;
								high = low;
								lowerIndex = higherIndex - 1;
								break;
							}
						}
						if (!found) {
							higherIndex = 0;
							if (low == nullptr) {
								lowerIndex = -1;
							} else {
								lowerIndex = low->get_entry_size() - 1;
							}
						}
					}
					//delete searchRoot;
					//searchRoot = nullptr;
					higher = high;
					lower = low;
					return;
				}
			}
		}
	}
} 
