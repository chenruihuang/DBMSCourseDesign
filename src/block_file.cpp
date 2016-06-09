#include "block_file.h"
#include <cstring>

// -----------------------------------------------------------------------------
//  Some points to NOTE:
//  1) 2 types of block # are used (i.e. the internal # (e.g. act_block) 
//     and external # (e.g. pos)). internal # is one larger than external #
//     because the first block of the file is used to store header info. 
//     data info is stored starting from the 2nd block (excluding the header
//     block).both types of # start from 0.
//
//  2) "act_block" is internal block #. "number" is the # of data block (i.e. 
//     excluding the header block). maximum actblock equals to number. Maximum 
//     external block # equals to number - 1 
//
//  3) cache_cont records the internal numbers. 
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  BlockFile: structure of reading and writing file for b-tree
// -----------------------------------------------------------------------------
BlockFile::BlockFile(				// constructor
	char* name,							// file name
	int b_length)						// block length
{
	file_name_ = new char[strlen(name) + 1];
	strcpy(file_name_, name);
	block_length_ = b_length;

	num_blocks_ = 0;				// num of blocks, init to 0
	// -------------------------------------------------------------------------
	//  Init <fp> and open <file_name_>. If <file_name_> exists, then fp != 0,
	//  and we excute if-clause program. Otherwise, we excute else-clause 
	//  program.
	//
	//  "rb+": read or write data from or into binary doc. if the file not 
	//  exist, it will return NULL.
	//  �ö����Ƹ�ʽ���ļ�׼����д
	// -------------------------------------------------------------------------
	if ((fp_ = fopen(name, "rb+")) != nullptr) {
		// ---------------------------------------------------------------------
		//  Init <new_flag_> (since the file exists, <new_flag_> is false).
		//  Reinit <block_length_> (determined by the doc itself).
		//  Reinit <num_blocks_> (number of blocks in doc itself).
		// ---------------------------------------------------------------------
		new_flag_ = false;			// reinit <block_length_> by file
		block_length_ = fread_number();
		num_blocks_ = fread_number();
	}
	else {
		// ---------------------------------------------------------------------
		//  <file_name_> not exists. we construct new file and reinit paras.
		// ---------------------------------------------------------------------
		if (block_length_ < BFHEAD_LENGTH) {
			// -----------------------------------------------------------------
			//  Ensure <block_length_> is larger than or equal to 8 bytes.
			//  8 bytes = 4 bypes <block_length_> + 4 bytes <num_blocks_>.
			//  ��˼�ǣ�����header���ִ�����2��int����������8 bytes�Ĵ�С
			//  block_length_���⻹С���ǲ�����ġ�
			// -----------------------------------------------------------------
			error("BlockFile::BlockFile couldnot open file.\n", true);
		}

		// ---------------------------------------------------------------------
		//  "wb+": read or write data from or into binary doc. if file not
		//  exist, we will construct a new file.
		// ---------------------------------------------------------------------
		fp_ = fopen(file_name_, "wb+");
		if (fp_ == nullptr) {
			error("BlockFile::BlockFile could not create file.\n", true);
		}

		// ---------------------------------------------------------------------
		//  Init <new_flag_>: as file is just constructed (new), it is true.
		//  Write <block_length_> and <num_blocks_> to the header of file.
		//  Since the file is empty (new), <num_blocks_> is 0 (no blocks in it).
		// ---------------------------------------------------------------------
		new_flag_ = true;
		fwrite_number(block_length_);
		fwrite_number(0);

		// ---------------------------------------------------------------------
		//  Since <block_length_> >= 8 bytes, for the remain bytes, we will 
		//  init 0 to them.
		//  ��0���һ��blockʣ�µĲ��֡�
		//  ftell() ���ش��ļ���ͷ��Ŀǰ��λ���м���bytes��
		// ---------------------------------------------------------------------
		char* buffer = nullptr;
		int len = block_length_ - static_cast<int>(ftell(fp_));				// cmpt remain length of a block
		buffer = new char[len];
									// set to 0 to remain bytes
		memset(buffer, 0, sizeof(buffer));
		put_bytes(buffer, len);

		delete[] buffer;
		buffer = nullptr;
	}
	// -------------------------------------------------------------------------
	//  Redirect file pointer to the start position of the file
	// -------------------------------------------------------------------------
	fseek(fp_, 0, SEEK_SET);
	act_block_ = 0;					// init <act_block_> (no blocks)
}

// -----------------------------------------------------------------------------
BlockFile::~BlockFile()				// destructor
{
	if (file_name_) {				// release space of <file_name_>
		delete[] file_name_;
		file_name_ = nullptr;
	}
	if (fp_) fclose(fp_);			// close <fp_>
}

// -----------------------------------------------------------------------------
void BlockFile::fwrite_number(		// write an <int> value to bin file
	int value) const  // a value of type <int>
{
	put_bytes(reinterpret_cast<char *>(&value), SIZEINT);
}

// -----------------------------------------------------------------------------
int BlockFile::fread_number() const  // read an <int> value from bin file
{
	char ca[SIZEINT];
	get_bytes(ca, SIZEINT);

	return *reinterpret_cast<int *>(ca);
}

// -----------------------------------------------------------------------------
//  Note that this func does not read the header of blockfile. It fetches the 
//  info in the first block excluding the header of blockfile.
//  ��������header��Ϊ��ȡ�������������׼����
// -----------------------------------------------------------------------------
void BlockFile::read_header(		// read remain bytes excluding header
	char* buffer)						// contain remain bytes (return)
{
									// jump out of first 8 bytes
	fseek(fp_, BFHEAD_LENGTH, SEEK_SET);
									// read remain bytes into <buffer>
	get_bytes(buffer, block_length_ - BFHEAD_LENGTH); 

	if (num_blocks_ < 1) {			// no remain bytes
		fseek(fp_, 0, SEEK_SET);	// fp return to beginning pos
		act_block_ = 0;				// no act block
	} else {
		// ---------------------------------------------------------------------
		//  Since we have read the first block (header block) of block file,
		//  thus <act_block_> = 1, and the file pointer point to the 2nd block
		//  (first block to store real data).
		// ---------------------------------------------------------------------
		act_block_ = 1;
	}
}

// -----------------------------------------------------------------------------
//  Note that this func does not write the header of blockfile. It writes the 
//  info in the first block excluding the header of blockfile.
//  �ڻ�����header��2��int���ֱ�Ϊblock�ĳ��Ⱥ͸��������ⴢ��������Ϣ��
// -----------------------------------------------------------------------------
void BlockFile::set_header(			// set remain bytes excluding header
	char* header)						// contain remain bytes
{
									// jump out of first 8 bytes
	fseek(fp_, BFHEAD_LENGTH, SEEK_SET);
									// write remain bytes into <buffer>
	put_bytes(header, block_length_ - BFHEAD_LENGTH);
	
	if (num_blocks_ < 1) {			// no remain bytes
		fseek(fp_, 0, SEEK_SET);	// fp return to beginning pos
		act_block_ = 0;				// no act block
	}
	else {
		// ---------------------------------------------------------------------
		//  Since we have write the first block (header block) of block file,
		//  thus <act_block_> = 1, and the file pointer point to the 2nd block 
		//  (first block to store real data).
		// ---------------------------------------------------------------------
		act_block_ = 1;
	}
}

// -----------------------------------------------------------------------------
//  Read a <block> from <index>
//
//  We point out the difference of counting among the <number>, <act_block> 
//  and <pos>.
//  (1) <num_blocks_>: ��¼block�ĸ�����������header��block����1��ʼ��
//  (2) <act_block_>: ��¼��ǰ�Ѿ���д��block�ĸ���������header��block��
//      ��˵������ڶ�д�ļ���ʱ��<act_block>Ϊ1���Ѿ�������header���������ļ�ָ�����Ӧ�� 
//  (3) <index> : ��¼���Ǵ����д��block��λ�ã�������header��block����0��ʼ��
//      ���統<index> = 0ʱ���ļ�ָ��ָ��header��block����һ��block����ʱ<act_block_>����1��
//
//  ���ӣ����numberΪ3����ô�ļ���һ����4��block��������һ��header��block��3��data��block��
//
//  ���ļ����򿪣�<act_block_>Ϊ1�����<index>Ϊ1������ζ�����Ǵ����д������block
//  ���ڶ���data��block�����������index�������2��Ȼ��feesk���ļ�ָ���Ƶ��ڶ���data��block��
//
//  �Եڶ���data��block��д�������ļ�ָ��ָ����������ݵ�block����ʱ�Ѿ���д��3��block��
//  ��1��header��2��data���������<act_block> = <index> + 1 = 2 + 1 = 3��
// -----------------------------------------------------------------------------
bool BlockFile::read_block(			// read a <block> from <index>
	Block block,						// a <block> (return)
	int index)							// Ҫ��ȡ�ڼ���data��block����0��ʼ��
{
	index++;						// �����󣬱�ʾҪ����block�еĵڼ��ţ�header��block�ǵ�0�ţ�
									// move to the position
	if (index <= num_blocks_ && index > 0) {
		seek_block(index);
		// �ٸ����ӣ������ʱindex��1������Ҫ���ʵ�һ��data��block��
		// �����ʱact_block_��index��ȣ�ҲΪ1�������Ѿ���д��1���飨����header�飩��
		// �����ʱ�ļ�ָ��ָ���һ��data��block�Ŀ�ͷ������ļ�ָ�벻��Ҫ�ƶ���
		// �鿴seek_block������Դ������һ���˽⡣
	}
	else {
		printf("BlockFile::read_block request the block %d "
			"which is illegal.", index - 1);
		error("\n", true);
	}

	get_bytes(block, block_length_);// read the block
	if (index + 1 > num_blocks_) {	// <fp_> reaches the end of file
		fseek(fp_, 0, SEEK_SET);
		act_block_ = 0;				// <act_block_> rewind to start pos
	}
	else {
		act_block_ = index + 1;		// <act_block_> to next pos
	}
	return true;
}

// -----------------------------------------------------------------------------
//  �������������д�Ѿ����ڵ�block�ģ����ܳ���<num_blocks>�ķ�Χ��
//  ���Ҫ�����µ�block��Ӧ�õ���append_block������
// -----------------------------------------------------------------------------
bool BlockFile::write_block(		// write a <block> into <index>
	Block block,						// a <block>
	int index)							// position of the blocks
{
	index++;						// extrnl block to intrnl block
									// move to the position
	if (index <= num_blocks_ && index > 0) {
		seek_block(index);
	}
	else {
		printf("BlockFile::write_block request the block %d "
			"which is illegal.", index - 1);
		error("\n", true);
	}
	
	put_bytes(block, block_length_);// write this block
	if (index + 1 > num_blocks_) {	// update <act_block_>
		fseek(fp_, 0, SEEK_SET);
		act_block_ = 0;
	}
	else {
		act_block_ = index + 1;
	}
	return true;
}

// -----------------------------------------------------------------------------
//  ���ļ�ĩβ׷���µ�block���ļ�ָ��ָ����׷�ӵ�block����������λ��(��0��ʼ�Ĳ�����header��block���)��
// -----------------------------------------------------------------------------
int BlockFile::append_block(		// append new block at the end of file
	Block block)						// the new block
{
	fseek(fp_, 0, SEEK_END);		// <fp_> point to the end of file
	put_bytes(block, block_length_);// write a <block>
	num_blocks_++;					// add 1 to <num_blocks_>
	
	fseek(fp_, SIZEINT, SEEK_SET);	// <fp_> point to pos of header������һ��int�Ĵ�С����Ϊ��һ��int����һ��block�Ĵ�С
	fwrite_number(num_blocks_);		// update <num_blocks_>

	// -------------------------------------------------------------------------
	//  <fp_> point to the pos of new added block. 
	//  The equation <act_block_> = <num_blocks_> indicates the file pointer 
	//  point to new added block.
	//  Return index of new added block
	// -------------------------------------------------------------------------
	fseek(fp_, -block_length_, SEEK_END);
	act_block_ = num_blocks_;
	return act_block_ - 1;
}

// -----------------------------------------------------------------------------
//  Delete last <num> block in the file.
//
//  ֻ���޸���block�ĸ��������ݻ��Ǵ������ļ��У��ļ���С���䡣
// -----------------------------------------------------------------------------
bool BlockFile::delete_last_blocks(	// delete last <num> blocks
	int num)							// number of blocks to be deleted
{
	if (num > num_blocks_) {		// check whether illegal?
		return false;
	}

	num_blocks_ -= num;				// update <number>
	fseek(fp_, SIZEINT, SEEK_SET);
	fwrite_number(num_blocks_);

	fseek(fp_, 0, SEEK_SET);		// <fp> point to beginning of file
	act_block_ = 0;					// <act_block> = 0
	return true;
}

