/* $Id$ */
#ifndef _MSGFILE_H_
#define _MSGFILE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "Sync.h"
#include "SyncObject.h"

START_NAMESPACE

class MsgFile
{
public:
	MsgFile(void);
	virtual ~MsgFile(void);
	int lookupMsg(int facility, int number, int length, TEXT* buffer, USHORT* flags);
	int openMsgFile(const char* filename);
	void	close(void);

	SyncObject	syncObject;
	ULONG	msg_top_tree;
	int		msg_file;
	USHORT	msg_bucket_size;
	USHORT	msg_levels;
	UCHAR	*msg_bucket;
};

END_NAMESPACE

#endif
