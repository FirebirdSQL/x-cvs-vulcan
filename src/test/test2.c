#include <stdio.h>
#include <memory.h>
#include "firebird.h"
#include "MemMgr.h"

#define HASH_SIZE	1001

struct Hash {
	void	*object;
	Hash	*collision;
	}

static Hash	*hashTable [HASH_SIZE];

int main (int argc, const char **argv)
{
	
}