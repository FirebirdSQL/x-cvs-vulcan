/*
 *      PROGRAM:        JRD access method
 *      MODULE:         Symb.h
 *      DESCRIPTION:    Common descriptions
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#ifndef JRD_SYMB_H
#define JRD_SYMB_H

/* symbol definitions */

enum sym_t {
	SYM_rel,					/* relation block */
	SYM_fld,					/* field block */
	SYM_fun,					/* UDF function block */
	SYM_prc,					/* stored procedure block */
	SYM_sql,					/* SQL request cache block */
    SYM_blr,					/* BLR request cache block */
    SYM_label					/* CVC: I need to track labels if LEAVE is implemented. */
};
typedef sym_t SYM_T;

class Symbol : public pool_alloc<type_sym>
{
    public:
	TEXT*	sym_string;		/* address of asciz string */
	SYM_T	sym_type;		/* symbol type */
	void*	sym_object;		/* general pointer to object */
	Symbol* sym_collision;	/* collision pointer */
	Symbol* sym_homonym;	/* homonym pointer */
};

#endif

