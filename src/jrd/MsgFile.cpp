#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <memory.h>

#include "fbdev.h"
#include "common.h"
#include "MsgFile.h"
#include "msg.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h> // umask, close, lseek, read, open, _sopen
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef O_BINARY
#define O_BINARY	0
#endif

MsgFile::MsgFile(void)
{
	msg_file = -1;
	msg_bucket = NULL;
}

MsgFile::~MsgFile(void)
{
	close();
	
	if (msg_bucket)
		delete [] msg_bucket;
}

int MsgFile::lookupMsg(int facility, int number, int length, TEXT* buffer, USHORT* flags)
{
	int status = -1;
	Sync sync(&syncObject, "lookupMsg"); 
	sync.lock(Exclusive); // protect lseeks

	/* Search down index levels to the leaf.  If we get lost, punt */

	const ULONG code = MSG_NUMBER(facility, number);
	const msgnod* end =  (MSGNOD) ((SCHAR *) msg_bucket + msg_bucket_size);
	ULONG position = msg_top_tree;

	status = 0;
	
	for (USHORT n = 1; !status; n++) 
		{
		if (lseek(msg_file, LSEEK_OFFSET_CAST position, 0) < 0)
			status = -6;
		else if (read(msg_file, msg_bucket, msg_bucket_size) < 0)
			status = -7;
		else if (n == msg_levels)
			break;
		else 
			{
			for (const msgnod* node = (MSGNOD) msg_bucket; !status; node++) 
				{
				if (node >= end) 
					{
					status = -8;
					break;
					}
				if (node->msgnod_code >= code) 
					{
					position = node->msgnod_seek;
					break;
					}
				}
			}
		}

	if (!status) 
		{
		for (const msgrec* leaf = (MSGREC) msg_bucket; !status;
			 leaf = NEXT_LEAF(leaf)) 
			{
			if (leaf >= (MSGREC) end || leaf->msgrec_code > code) 
				{
				status = -1;
				break;
				}
			if (leaf->msgrec_code == code) 
				{
				/* We found the correct message, so return it to the user */
				const USHORT n = MIN(length - 1, leaf->msgrec_length);
				memcpy(buffer, leaf->msgrec_text, n);
				buffer[n] = 0;

				if (flags)
					*flags = leaf->msgrec_flags;

				status = leaf->msgrec_length;
				break;
				}
			}
		}

	return status;
}

int MsgFile::openMsgFile(const char* filename)
{
	fileName = filename;
	msg_file = ::open(filename, O_RDONLY | O_BINARY, 0);
	
	if (msg_file < 0)
		return -2;

	isc_msghdr header;
	
	if (read(msg_file, &header, sizeof(header)) < 0) 
		{
		close();
		return -3;
		}
		
	if (header.msghdr_major_version != MSG_MAJOR_VERSION ||
		header.msghdr_minor_version < MSG_MINOR_VERSION) 
		{
		close();
		return -4;
		}

	msg_bucket_size = header.msghdr_bucket_size;
	msg_levels = header.msghdr_levels;
	msg_top_tree = header.msghdr_top_tree;
	msg_bucket = new UCHAR [msg_bucket_size];
	
	return 0;
}

void MsgFile::close(void)
{
	if (msg_file >= 0)
		{
		::close (msg_file);
		msg_file = -1;
		}
}
