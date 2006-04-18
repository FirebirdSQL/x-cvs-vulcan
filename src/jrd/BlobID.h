/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		blb.h
 *	DESCRIPTION:	Blob handling definitions
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 *
 */

#ifndef JRD_BLOBID_H
#define JRD_BLOBID_H

#include "gdsassert.h"
#include "RecordNumber.h"

/* 
  Blob id.  A blob has two states -- temporary and permanent.  In each
  case, the blob id is 8 bytes (2 longwords) long.  In the case of a
  temporary blob, the first word is NULL and the second word points to
  an internal blob block.  In the case of a permanent blob, the first
  short contains the relation id, the next byte is unused, and the next
  five bytes are the record id.  However, the last four bytes are stored
  as a native format longword. 
*/

// This structure must occupy 8 bytes
struct bid {
	union {
		// Internal decomposition of the structure
		struct {
			USHORT bid_relation_id;		/* Relation id (or null) */
			UCHAR bid_reserved_for_relation;	/* Reserved for future expansion of relation space. */
			UCHAR bid_number[5]; // This is either record number encoded as 40-bit record number
								 // or 32-bit temporary ID of blob or array prefixed with zero byte
		} bid_internal;

		// This is how bid structure represented in public API.
		// Must be present to enforce alignment rules when structure is declared on stack
		struct {
			ULONG bid_quad_high;
			ULONG bid_quad_low;
		} bid_quad;
	};

	ULONG& bid_temp_id() {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		return *reinterpret_cast<ULONG*>(bid_internal.bid_number + 1);
	}

	ULONG bid_temp_id() const {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		return *reinterpret_cast<const ULONG*>(bid_internal.bid_number + 1);
	}

	bool isEmpty() const { 
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		return bid_quad.bid_quad_high == 0 && bid_quad.bid_quad_low == 0; 
	}

	void clear() {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		bid_quad.bid_quad_high = 0;
		bid_quad.bid_quad_low = 0;
	}

	void set_temporary(ULONG temp_id) {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		clear();
		bid_temp_id() = temp_id;
	}

	void set_permanent(USHORT relation_id, RecordNumber num) {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		clear();
		bid_internal.bid_relation_id = relation_id;
		num.bid_encode(bid_internal.bid_number);
	}

	RecordNumber get_permanent_number() const {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		RecordNumber temp;
		temp.bid_decode(bid_internal.bid_number);
		return temp;
	}

	bool operator == (const bid& other) const {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		return bid_quad.bid_quad_high == other.bid_quad.bid_quad_high && 
			bid_quad.bid_quad_low == other.bid_quad.bid_quad_low;
	}
};

/*
typedef struct bid {
	INT32 bid_relation_id;		// Relation id (or null)
	union {
		//class blb *bid_blob;	// Pointer to blob block
		SLONG bid_blob;			// blob handle
		SLONG bid_number;			// Record number
	} bid_stuff;
} *BID;
*/

#endif /* JRD_BLOBID_H */

