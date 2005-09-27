#include "fbdev.h"
#include "jrd.h"
#include "../dsql/dsql_rel.h"
#include "../jrd/ProcParam.h"
#include "../jrd/intl.h"

ProcParam::ProcParam(int id)
{
	paramId = id;
	paramProcedure = NULL;
	paramNext = NULL;
	paramFlags = 0;
}
ProcParam::ProcParam (dsql_fld *field)
{
	paramId = 0;
	paramProcedure = field->fld_procedure;
	paramNext = NULL;
	paramFlags = 0;
	fld_array	= 0;					/* array description, if array */
	fld_name = field->fld_name;			/* Field name */
	fld_dimensions = field->fld_dimensions;		/* used by DSQL */
	fld_dtype = field->fld_dtype;				/* used by DSQL */
	fld_scale = field->fld_scale;				/* used by DSQL */
	fld_sub_type = field->fld_sub_type;
	fld_precision = field->fld_precision;
//	UCHAR		fld_length;			/* Field name length */
	dsqlField = field;
}

ProcParam::~ProcParam()
{
}
void ProcParam::setDescriptor(DSC descriptor)
{
	paramDescriptor = descriptor;
}

dsql_fld* ProcParam::getDsqlField(void)
{
	if (!dsqlField)
		{
		dsqlField = new dsql_fld;
		dsc *desc = &paramDescriptor;	
		dsqlField->fld_dtype = desc->dsc_dtype;	
		dsqlField->fld_scale = desc->dsc_scale;	
		dsqlField->fld_length = desc->dsc_length;		
		dsqlField->fld_sub_type = desc->dsc_sub_type;
		dsqlField->fld_dimensions = 0;		
		dsqlField->fld_name = fld_name;
		dsqlField->fld_id = paramId;
		
		if (dsqlField->fld_dtype <= dtype_any_text)  
			{
			dsqlField->fld_character_set_id = INTL_TYPE_TO_CS(dsqlField->fld_sub_type);
			dsqlField->fld_collation_id = INTL_TYPE_TO_COL(dsqlField->fld_sub_type);
			}
		}
	
	return dsqlField;
}
