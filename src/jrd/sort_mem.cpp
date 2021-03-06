/*
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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "fbdev.h"

//#include "../common/config/config.h"
#include "../jrd/gds_proto.h"
#include "../jrd/sort.h"
#include "../jrd/sort_proto.h"
#include "../jrd/gdsassert.h"
#include "../jrd/sort_mem.h"
#include "Database.h"
#include "ConfObject.h"
#include "Parameters.h"
#include "OSRIMemException.h"

bool SortMem::is_initialized = false;

size_t SortMem::mem_block_size;
size_t SortMem::mem_upper_limit;

size_t SortMem::mem_total_size = 0;

/******************************************************************************
 *
 *	Generic storage block implementation
 */

SortMem::Block::Block(Block *tail, size_t length)
	: next(0), size(length)
{
	// Link block with the chain
	if (tail)
	{
		tail->next = this;
	}
	prev = tail;
}

/******************************************************************************
 *
 *	Virtual memory block implementation
 */

SortMem::MemoryBlock::MemoryBlock(Block* tail, size_t length)
	: Block(tail, length)
{
	// Allocate virtual memory block
	address = reinterpret_cast<char*>(gds__alloc(size));
}

SortMem::MemoryBlock::~MemoryBlock()
{
	// Free virtual memory block
	gds__free(address);
}

size_t SortMem::MemoryBlock::read(size_t position, char *buffer, size_t length)
{
	// Read block from memory
	if (position + length > size)
	{
		length = size - position;
	}
	memcpy(buffer, address + position, length);
	return length;
}

size_t SortMem::MemoryBlock::write(size_t position, char *buffer, size_t length)
{
	// Write block to memory
	if (position + length > size)
	{
		length = size - position;
	}
	memcpy(address + position, buffer, length);
	return length;
}
		
/******************************************************************************
 *
 *	File block implementation
 */

SortMem::FileBlock::FileBlock(Block *tail, size_t length, SortWorkFile* blk, size_t position)
	: Block(tail, length), file(blk), offset(position)
{
}

SortMem::FileBlock::~FileBlock()
{
}

size_t SortMem::FileBlock::read(size_t position, char *buffer, size_t length)
{
	// Read block from file
	if (position + length > size)
	{
		length = size - position;
	}
	return SORT_read_block(file, offset + position,
		reinterpret_cast<unsigned char*>(buffer), length) - offset - position;
//	_lseek(file->sfb_file, offset + position, SEEK_SET);
//	return _read(file->sfb_file, buffer, length);
}

size_t SortMem::FileBlock::write(size_t position, char *buffer, size_t length)
{
	// Write block to file
	if (position + length > size)
	{
		length = size - position;
	}
	return SORT_write_block(file, offset + position, 
		reinterpret_cast<unsigned char*>(buffer), length) - offset - position;
//	_lseek(file->sfb_file, offset + position, SEEK_SET);
//	return _write(file->sfb_file, buffer, length);
}

/******************************************************************************
 *
 *	Virtual scratch file implementation
 */

SortMem::SortMem(SortWorkFile* blk, size_t size)
	: internal(blk), logical_size(0), physical_size(0), file_size(0), head(0), tail(0)
{
	// Initialize itself
	if (!is_initialized)
	{
		mem_block_size = blk->configuration->getValue (SortMemBlockSize,SortMemBlockSizeValue); //Config::getSortMemBlockSize();
		mem_upper_limit = blk->configuration->getValue (SortMemUpperLimit,SortMemUpperLimitValue); //Config::getSortMemUpperLimit();
		is_initialized = true;
	}

	// Allocate one block
	allocate(size);
}

SortMem::~SortMem()
{
	// Free all allocated blocks
	while (tail)
	{
		Block *block = tail->prev;
		delete tail;
		tail = block;
	}
	// We've just freed some memory
	mem_total_size -= physical_size - file_size;
}

void SortMem::allocate(size_t size)
{
	if (size == 0)
		return;
		
	logical_size += size;

	// We've already got enough space to write data of the given size
	
	if (logical_size <= physical_size)
		return;
		
	// Calculate how much virtual memory we should allocate
	
	size_t smart_size = (mem_block_size > size) ? mem_block_size : size;
	Block *block = NULL;

	// Check whether virtual memory should be allocated or file should be used instead
	
	if (mem_total_size + smart_size <= mem_upper_limit)
		{
		try
			{
			block = FB_NEW (*getDefaultMemoryPool()) MemoryBlock(tail, smart_size);
			}
		catch (MEMORY_EXCEPTION)
			{
			// Check whether we can try to allocate less memory
			
			if (smart_size > size)
				{
				// Allocate minimal possible amount of memory once more
				smart_size = size;
				
				try
					{
					block = FB_NEW (*getDefaultMemoryPool()) MemoryBlock(tail, smart_size);
					}
				catch (MEMORY_EXCEPTION) 
					{
					}
				}
			}
			
		if (block)
			{
			physical_size += smart_size;
			mem_total_size += smart_size;
			}
		}

	if (!block)
		{
		block = FB_NEW (*getDefaultMemoryPool()) FileBlock(tail, size, internal, file_size);
		physical_size += size;
		file_size += size;
		}

	// Append new block to the chain
	
	if (!head)
		head = block;

	tail = block;
}

SortMem::Block* SortMem::seek(size_t &position)
{
	Block *block = 0;

	// Check whether the given offset is valid
	if (position < physical_size)
	{
		if (position < physical_size / 2)
		{
			// Let's walk forward
			block = head;
			while (block && position >= block->size)
			{
				position -= block->size;
				block = block->next;
			}
		}
		else
		{
			// Let's walk backward
			block = tail;
			while (block && physical_size - position > block->size)
			{
				position += block->size;
				block = block->prev;
			}
			position += block->size - physical_size;
		}
	}

	return block;
}

size_t SortMem::read(size_t position, char *address, size_t length)
{
	// If we'are not allowed to use memory, don't waste time
	// playing with all these memory blocks - just use scratch file and return
	if (!mem_upper_limit)
	{
		return SORT_read_block(internal, position, 
			reinterpret_cast<unsigned char*>(address), length);
	}

	size_t copied = 0;

	if (length > 0)
	{
		// Search for the first needed block
		size_t pos = position;
		Block *block = seek(pos);
		fb_assert(block);

		// Read data from as many blocks as necessary
		for (Block *itr = block; itr && length > 0; itr = itr->next, pos = 0)
		{
			size_t n = itr->read(pos, address, length);
			address += n;
			copied += n;
			length -= n;
		}
		fb_assert(!length);
	}

	// New seek value
	return position + copied;
}

size_t SortMem::write(size_t position, char *address, size_t length)
{
	// If we'are not allowed to use memory, don't waste time
	// playing with all these memory blocks - just use scratch file and return
	if (!mem_upper_limit)
	{
		return SORT_write_block(internal, position, 
			reinterpret_cast<unsigned char*>(address), length);
	}

	// There's probably not enough space, try to allocate one more block
	if (position + length >= logical_size)
	{
		allocate(position + length - logical_size);
	}

	size_t copied = 0;

	if (length > 0)
	{
		// Search for the first needed block
		size_t pos = position;
		Block *block = seek(pos);
		fb_assert(block);

		// Write data to as many blocks as necessary
		for (Block *itr = block; itr && length > 0; itr = itr->next, pos = 0)
		{
			size_t n = itr->write(pos, address, length);
			address += n;
			copied += n;
			length -= n;
		}
		fb_assert(!length);
	}

	// New seek value
	return position + copied;
}

SortWorkFile::SortWorkFile (Database *database)
{
	memset (this, 0, sizeof (*this));
	configuration = database->configuration;
}

