#ifndef __DSQL_REL__
#define __DSQL_REL__

#include "JString.h"
#include "SyncObject.h"

class dsql_sym;
class dsql_fld;
class Relation;
class dsql_nod;
class Procedure;
class Field;

struct dsc;

//! Relation block
class dsql_rel //: public pool_alloc_rpt<SCHAR, dsql_type_dsql_rel>
{
public:
	dsql_rel(void);
	dsql_rel*	rel_next;			//!< Next relation in database
	dsql_sym*	rel_symbol;			//!< Hash symbol for relation
	dsql_fld*	rel_fields;		//!< Field block
	dsql_fld*	rel_junk;		//!< Field block
	dsql_rel*	rel_base_relation;	//!< base relation for an updatable view
	JString		rel_name;			//!< Name of relation
	JString		rel_owner;			//!< Owner of relation
	int			rel_id;				//!< Relation id
	USHORT		rel_dbkey_length;
	USHORT		rel_flags;
	Relation	*jrdRelation;
	SyncObject	syncFields;
	//TEXT		rel_data[3];
	void dropField(dsql_fld* field);
	~dsql_rel(void);
	dsql_fld* addField(int id, JString name);
	void addField(dsql_fld *field);
	void purgeTemporaryFields(void);
	void orderFields(void);
};

// rel_flags bits
enum rel_flags_vals {
	REL_new_relation	= 1, //!< relation is newly defined, not committed yet
	REL_dropped			= 2, //!< relation has been dropped
	REL_view			= 4, //!< relation is a view 
	REL_external		= 8  //!< relation is an external table
};

class dsql_fld //: public pool_alloc_rpt<SCHAR, dsql_type_fld>
{
public:
	dsql_fld(void);
	dsql_fld*	fld_next;				//!< Next field in relation
	dsql_rel*	fld_relation;			//!< Parent relation
	Procedure*	fld_procedure;			//!< Parent procedure
	dsql_nod*	fld_ranges;				//!< ranges for multi dimension array
	dsql_nod*	fld_character_set;		//!< null means not specified
	dsql_nod*	fld_sub_type_name;		//!< Subtype name for later resolution
	USHORT		fld_flags;
	USHORT		fld_id;					//!< Field in in database
	USHORT		fld_dtype;				//!< Data type of field
	FLD_LENGTH	fld_length;				//!< Length of field
	UCHAR		fld_element_dtype;		//!< Data type of array element
	USHORT		fld_element_length;		//!< Length of array element
	SCHAR		fld_scale;				//!< Scale factor of field
	SSHORT		fld_sub_type;			//!< Subtype for text & blob fields
	USHORT		fld_precision;			//!< Precision for exact numeric types
	USHORT		fld_character_length;	//!< length of field in characters
	USHORT		fld_seg_length;			//!< Segment length for blobs
	SSHORT		fld_dimensions;			//!< Non-zero means array
	SSHORT		fld_character_set_id;	//!< ID of field's character set
	SSHORT		fld_collation_id;		//!< ID of field's collation
	SSHORT		fld_ttype;				//!< ID of field's language_driver
	Field		*field;
	JString		fld_name;
	void setType(dsc* desc, int dimensions);
};

// values used in fld_flags

enum fld_flags_vals {
	FLD_computed	= 1,
	FLD_drop		= 2,
	FLD_dbkey		= 4,
	FLD_national	= 8,	//!< field uses NATIONAL character set
	FLD_nullable	= 16,
	FLD_temporary	= 32	// Field is temporary, used for DDL operations only
};

#endif
