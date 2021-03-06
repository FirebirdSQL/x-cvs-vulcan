/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pio_proto.h
 *	DESCRIPTION:	Prototype header file for unix.cpp, vms.cpp & winnt.cpp
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

#ifndef JRD_PIO_PROTO_H
#define JRD_PIO_PROTO_H

class File;
struct thread_db;

int		PIO_add_file(thread_db* tdbb, File*, const TEXT*, SLONG);
void	PIO_close(File*);
File*	PIO_create(thread_db* tdbb, const TEXT*, SSHORT, BOOLEAN, bool shared);
int		PIO_connection(const TEXT*, USHORT*);
int		PIO_expand(const TEXT*, USHORT, TEXT*);
void	PIO_flush(File*);
void	PIO_force_write(File*, USHORT, bool shared);
void	PIO_header(Database*, SCHAR*, int);
SLONG	PIO_max_alloc(Database*);
SLONG	PIO_act_alloc(Database*);
File*	PIO_open(thread_db* tdbb, const TEXT*, SSHORT, const TEXT*, bool shared);
int		PIO_read(File*, class Bdb*, struct pag*, ISC_STATUS*);

#ifdef SUPERSERVER_V2
int		PIO_read_ahead(Database*, SLONG, SCHAR*, SLONG, struct piob*,
						  ISC_STATUS*);
int		PIO_status(struct piob*, ISC_STATUS*);
#endif

int		PIO_unlink(const TEXT*);
int		PIO_write(File*, class Bdb*, struct pag*, ISC_STATUS*);

#endif // JRD_PIO_PROTO_H

