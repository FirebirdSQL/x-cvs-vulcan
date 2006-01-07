/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		btn.h
 *	DESCRIPTION:	B-tree management code
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Arno Brinkman
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2003 Arno Brinkman and all contributors
 *  signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef JRD_BTN_H_
#define JRD_BTN_H_

//#include "fbdev.h"			// needed for get_long
#include "memory_routines.h"	// needed for get_long

//#include "../jrd/common.h"
#include "../jrd/ods.h"
#include "../include/fb_vector.h"
#include "../common/classes/array.h"

// format of expanded index node, used for backwards navigation
struct btree_exp 
{
	UCHAR btx_previous_length;		// AB: total size for previous node --length of data for previous node
	UCHAR btx_btr_previous_length;	// length of data for previous node on btree page
	UCHAR btx_data[1];				// expanded data element
};

const int BTX_SIZE				= 2;

// Flags (3-bits) used for index node
const int BTN_NORMAL_FLAG		= 0;
const int BTN_END_LEVEL_FLAG	= 1;
const int BTN_END_BUCKET_FLAG	= 2;
const int BTN_ZERO_PREFIX_ZERO_LENGTH_FLAG	= 3;
const int BTN_ZERO_LENGTH_FLAG	= 4;
const int BTN_ONE_LENGTH_FLAG	= 5;
//const int BTN_ZERO_PREFIX_ONE_LENGTH_FLAG	= 6;
//const int BTN_GET_MORE_FLAGS	= 7;


struct IndexNode
{
	UCHAR* nodePointer;	// pointer to where this node can be read from the page
	USHORT prefix;		// size of compressed prefix
	USHORT length;		// length of data in node
	SLONG pageNumber;	// page number
	UCHAR* data;		// Data can be read from here
	RecordNumber recordNumber;	// record number
	bool isEndBucket;
	bool isEndLevel;
};

struct IndexJumpNode
{
	UCHAR* nodePointer;	// pointer to where this node can be read from the page
	USHORT prefix;		// length of prefix against previous jump node
	USHORT length;		// length of data in jump node (together with prefix this is prefix for pointing node)
	USHORT offset;		// offset to node in page  
	UCHAR* data;		// Data can be read from here
};

// format of expanded index buffer
struct exp_index_buf 
{
	USHORT exp_length;
	ULONG exp_incarnation;
	btree_exp exp_nodes[1];
};

const size_t EXP_SIZE	= OFFSETA (exp_index_buf*, exp_nodes);

struct dynKey
{
	USHORT keyLength;
	UCHAR* keyData;
};

typedef firebird::vector<dynKey*> keyList;
typedef firebird::HalfStaticArray<IndexJumpNode, 32> jumpNodeList;

namespace BTreeNode {

	USHORT computePrefix(const UCHAR* prevString, USHORT prevLength, 
				const UCHAR* string, USHORT length);

	SLONG findPageInDuplicates(const btree_page* page, UCHAR* pointer, 
				SLONG previousNumber, RecordNumber findRecordNumber);

	USHORT getJumpNodeSize(const IndexJumpNode* jumpNode, SCHAR flags);
	USHORT getNodeSize(const IndexNode* indexNode, SCHAR flags, bool leafNode = true);
	UCHAR* getPointerFirstNode(btree_page* page, IndexJumpInfo* jumpInfo = NULL);

	bool keyEquality(USHORT length, const UCHAR* data, const IndexNode* indexNode);

#ifdef SCROLLABLE_CURSORS
	UCHAR* lastNode(btree_page* page, exp_index_buf* expanded_page, btree_exp** expanded_node);
#endif

	UCHAR* nextNode(IndexNode* node, UCHAR* pointer, 
				SCHAR flags,  btree_exp** expanded_node);
	UCHAR* previousNode(IndexNode* node, UCHAR* pointer,
				SCHAR flags,  btree_exp** expanded_node);

	void quad_put(SLONG value, UCHAR *data);

	UCHAR* readJumpInfo(IndexJumpInfo* jumpInfo, UCHAR* pagePointer);
	UCHAR* readJumpNode(IndexJumpNode* jumpNode, UCHAR* pagePointer, SCHAR flags);
	UCHAR* readNode(IndexNode* indexNode, UCHAR* pagePointer, SCHAR flags, 
		bool leafNode);

	UCHAR* writeJumpInfo(btree_page* page, const IndexJumpInfo* jumpInfo);
	UCHAR* writeJumpNode(IndexJumpNode* jumpNode, UCHAR* pagePointer, SCHAR flags);
	UCHAR* writeNode(IndexNode* indexNode, UCHAR* pagePointer, SCHAR flags, 
		bool leafNode, bool withData = true);

	void setEndBucket(IndexNode* indexNode, bool leafNode = true);
	void setEndLevel(IndexNode* indexNode, bool leafNode = true);
	void setNode(IndexNode* indexNode, USHORT prefix = 0, USHORT length = 0, 
		RecordNumber recordNumber = RecordNumber(0), SLONG pageNumber = 0,
		bool isEndBucket = false, bool isEndLevel = false);

} // namespace BTreeNode

#endif // JRD_BTN_H_
