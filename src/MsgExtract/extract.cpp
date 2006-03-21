#include <stdio.h>
#include "fbdev.h"
#include "MsgExtract.h"
#include "Connection.h"

int main(int argc, const char **argv)
{
	try
		{
		MsgExtract extract(argc - 1, argv + 1);
		extract.genAll();
		}
	catch (SQLException& exception)
		{
		fprintf(stderr, "%s\n", exception.getText());
		return -1;
		}
	
	return 0;
}