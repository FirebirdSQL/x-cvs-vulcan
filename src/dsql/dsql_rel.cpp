#include "firebird.h"
#include "common.h"
#include "dsql_rel.h"
#include "dsc.h"
#include "../jrd/intl.h"
#include "Sync.h"
#include "Field.h"

dsql_fld::dsql_fld(void)
{
	fld_next = NULL;
	fld_relation = NULL;
	fld_procedure = NULL;
	fld_ranges = NULL;
	fld_character_set = NULL;
	fld_sub_type_name = NULL;

	fld_flags = 0;
	fld_id = 0;
	fld_dtype = 0;
	fld_length = 0;
	fld_element_dtype = 0;
	fld_element_length = 0;
	fld_scale = 0;
    fld_sub_type = 0;
	fld_precision = 0;
	fld_character_length = 0;
	fld_seg_length = 0;
	fld_dimensions = 0;
	fld_character_set_id = 0;
	fld_collation_id = 0;
	fld_ttype = 0;
	field = NULL;
}


dsql_rel::dsql_rel(void)
{
	rel_next = NULL;
	rel_symbol = NULL;
	rel_fields = NULL;
	rel_junk = NULL;
	rel_base_relation = NULL;
	rel_id = 0;
	rel_dbkey_length = 0;
	rel_flags = 0;
	jrdRelation = NULL;
}

void dsql_rel::dropField(dsql_fld* field)
{
	for (dsql_fld **ptr = &rel_fields; *ptr; ptr = &(*ptr)->fld_next)
		if (*ptr == field)
			{
			*ptr = field->fld_next;
			field->fld_next = rel_junk;
			rel_junk = field;
			break;
			}
}

dsql_rel::~dsql_rel(void)
{
	dsql_fld *field;
	
	while (field = rel_fields)
		{
		rel_fields = field->fld_next;
		delete field;
		}
		
	while (field = rel_junk)
		{
		rel_junk = field->fld_next;
		delete field;
		}
}

dsql_fld* dsql_rel::addField(int id, JString name)
{
	dsql_fld *field = new dsql_fld;
	field->fld_name = name;
	field->fld_id = id;
	addField(field);
	
	return field;
}

void dsql_fld::setType(dsc* desc, int dimensions)
{
	fld_dtype = desc->dsc_dtype;
	fld_scale = desc->dsc_scale;
	fld_precision = desc->dsc_length;
	fld_length = desc->dsc_length;
	fld_sub_type = desc->dsc_sub_type;
	fld_ttype = desc->dsc_sub_type;
	fld_dimensions = dimensions;

    if (fld_dtype <= dtype_any_text)  
		{
        fld_character_set_id = INTL_TYPE_TO_CS(fld_sub_type);
        fld_collation_id = INTL_TYPE_TO_COL(fld_sub_type);
		}
}

void dsql_rel::addField(dsql_fld *field)
{
	for (dsql_fld **ptr = &rel_fields;; ptr = &(*ptr)->fld_next)
		if (!*ptr)
			{
			*ptr = field;
			break;
			}
}

void dsql_rel::purgeTemporaryFields(void)
{
	for (dsql_fld *field, **ptr = &rel_fields; field = *ptr;)
		if (field->fld_flags & FLD_temporary)
			{
			*ptr = field->fld_next;
			delete field;
			}
		else
			ptr = &field->fld_next;
}

void dsql_rel::orderFields(void)
{
	Sync sync (&syncFields, "dsql_rel::orderFields");
	sync.lock(Exclusive);
	dsql_fld *fields = rel_fields;
	rel_fields = NULL;
	
	// Really crud sort, but what hell...
	
	for (dsql_fld *field; field = fields;)
		{
		fields = field->fld_next;
		dsql_fld **ptr = &rel_fields;
		
		for (dsql_fld *fld; fld = *ptr; ptr = &(*ptr)->fld_next)
			if (field->field && fld->field && field->field->fld_position < fld->field->fld_position)
				break;
		
		field->fld_next = *ptr;
		*ptr = field;
		}
}
