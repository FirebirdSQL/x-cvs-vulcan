#include <stdio.h>
#include "fbdev.h"
#include "common.h"
#include "AdminException.h"
#include "InputFile.h"
#include "XMLParse.h"
#include "Element.h"

/*
<ProjectSummary 
TotalModules="429" 
TotalLines="63432" 
CodeModules="206" 
HeaderModules="221" 
PreprocessedModules="0" 
OtherModules="2" 
NumberFunctions="2839" 
AverageArguments="5.00" 
AverageFunctionLines="14.86" 
AverageInternalComments="0.94" 
AverageInternalWhiteSpace="2.12" 
AverageCodeLines="11.80" 
ProjectName="Netfrastructure DB Engine"/>
*/

static const char *columns[] = {
	"TotalModules",			"Total Modules",
	"TotalLines",			"Total Lines",
	"CodeModules",			"Code Modules",
	"HeaderModules",		"Header Modules",
	"PreprocessedModules",	"Preprocessed Modules",
	"OtherModules",			"Other Modules",
	"NumberFunctions",		"Number Functions",
	"AverageArguments",		"Average Arguments",
	"AverageFunctionLines", "Average FunctionLines",
	"AverageCodeLines",		"Average Code Lines",
	"AverageInternalComments", "Average Internal Comments",
	"AverageInternalWhiteSpace", "Average Internal WhiteSpace",
	NULL };
	
int main(int argc, const char **argv)
{
	Element *summaries[10];
	int count = argc - 1;
	int n;
	
	for (n = 1; n < argc; ++n)
		{
		InputFile confFile (argv[n]);
		XMLParse confXml;
		confXml.pushStream (&confFile);
		summaries[n - 1] = confXml.parse();
		}
	
	FILE *file = fopen("summary.html", "w");
	fprintf(file, "<html><head><title>Database Engine Codebase Analysis</title></head><body>\n");
	fprintf(file, "<table border=1><tr><th>&nbsp;</th>\n");
	
	for (n = 0; n < count; ++n)
		fprintf(file, "<th>%s</th>\n", summaries[n]->getAttributeValue("ProjectName", "&nbsp;"));

	fprintf(file, "</tr>\n");
	
	for (const char **col = columns; col[0]; col += 2)
		{
		fprintf(file, "<tr>\n");
		fprintf(file, "<td><b>%s</b></td>", col[1]);
		
		for (n = 0; n < count; ++n)
			fprintf(file, "<td align=right>%s</td>", summaries[n]->getAttributeValue(col[0], ""));
			
		fprintf(file, "</tr>\n");
		}
	fprintf(file, "</table></body></html>\n");
	fclose(file);
	return 0;
}