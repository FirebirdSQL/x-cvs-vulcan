#include "firebird.h"
#include "dsql.h"


dsql_msg::dsql_msg(void)
{
	msg_parameters = NULL;
	msg_par_ordered = NULL;
	//msg_buffer = NULL;
	msg_length = 0;
	msg_parameter = 0;
	msg_index = 0;
	msg_number = 0;
}
