/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		opt_proto.h
 *	DESCRIPTION:	Prototype header file for opt.cpp
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
 */

#ifndef JRD_OPT_PROTO_H
#define JRD_OPT_PROTO_H

#include "../jrd/jrd.h"
#include "../jrd/btr.h"
#include "../jrd/rse.h"
#include "../jrd/lls.h"


BOOLEAN OPT_access_path(TDBB tdbb, const Request*, UCHAR*, SSHORT, USHORT*);
class Rsb* OPT_compile(TDBB, class Csb *,
							   struct rse *, struct lls *);
struct jrd_nod* OPT_make_dbkey(TDBB tdbb, struct opt *, struct jrd_nod *,
								  USHORT);
struct jrd_nod* OPT_make_index(TDBB, struct opt *, Relation *,
								  struct idx *);
int OPT_match_index(TDBB tdbb, struct opt *, USHORT, struct idx *);
void OPT_set_index(TDBB, Request *, class Rsb **, Relation *,
						  struct idx *);

#endif // JRD_OPT_PROTO_H

