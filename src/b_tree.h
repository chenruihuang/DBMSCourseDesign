#ifndef __B_TREE_H
#define __B_TREE_H


class BlockFile;
class BNode;
class BLeafNode;
class LItem;

// -----------------------------------------------------------------------------
//  BTree: 建立在Lj集合上的B+树
// -----------------------------------------------------------------------------
class BTree {
public:
	int root_;						// address of disk for root

	BlockFile* file_;				// file in disk to store
	BNode* root_ptr_;				// pointer of root

	// -------------------------------------------------------------------------
	BTree();						// constructor
	~BTree();						// destructor

	// -------------------------------------------------------------------------
	void init(						// init a new b-tree
		char* fname,					// file name
		int b_length);					// block length

	void init_restore(				// load an exist b-tree
		char* fname);					// file name

	// -------------------------------------------------------------------------
	int bulkload(					// bulkload b-tree from hash table in mem
		LItem* Ltable,				// hash table
		int n);						// number of entries
 
 	//  find the nearest node entry
 	//  lower[lowerIndex] <= query; higher[higherIndex] > query
	void searchLowerAndHigher(float query,
						 	  BLeafNode* & lower, int & lowerIndex,
						      BLeafNode* & higher, int & higherIndex);  

private:
	// -------------------------------------------------------------------------
	int read_header(				// read <root> from buffer
		char* buf);						// the buffer

	int write_header(				// write <root> into buffer
		char* buf) const;						// the buffer (return)

	// -------------------------------------------------------------------------
	void load_root();				// load root of b-tree

	void delete_root();				// delete root of b-tree
};

#endif
