#include <stdio.h>
#include <string.h>
#include "fbdev.h"
#include "common.h"
#include "AnalyseModule.h"
#include "AdminException.h"
#include "Element.h"

struct Extension {
	const char	*extension;
	ModuleType	type;
	};

static Extension extensions[]  = {
	".cpp",	Code,
	".h",	Header,
	".y",	Preprocessed,
	".epp",	Preprocessed,
	".def",	Other,
	NULL };
	
AnalyseModule::AnalyseModule()
{
	lineComment = false;
	lines = 0;
	numberArguments = 0;
	numberFunctions = 0;
	internalCommentLines = 0;
	externalCommentLines = 0;
	internalWhiteLines = 0;
	parenLevel = 0;
	braceLevel = 0;
	multiLineComment = false;
	singleLineComment = false;
	inFunction = false;
	bodyLines = 0;
	argList = false;
	arg = false;
	args = 0;
	numberModules = 0;
	codeModules = 0;
	headerModules = 0;
	preprocessedModules = 0;
	otherModules = 0;
}

AnalyseModule::~AnalyseModule(void)
{
}

void AnalyseModule::bumpCommentLines(void)
{
	if (inFunction)
		++internalCommentLines;
	else
		++externalCommentLines;
}

void AnalyseModule::parseModule(const char* fileName, bool echo)
{
	module = fileName;
	const char *p = strrchr(fileName, '.');
	type = Other;
	
	for (Extension *extension = extensions; extension->extension; ++extension)
		if (strcmp(p, extension->extension) == 0)
			{
			type = extension->type;
			break;
			}

	FILE *file = fopen(fileName, "r");
	
	if (!file)
		throw AdminException("can't open \"%s\"", fileName);
	
	char line[512];
	char c;
	char prior = 0;
	char quote = 0;
	int whiteLines = 0;
	bool white;
	bool escape = false;
	
	while (fgets(line, sizeof(line), file))
		{
		if (echo)
			fputs(line, stdout);
			
		white = true;
		
		if (!multiLineComment && line[0] == '#')
			continue;
			
		for (const char *p = line; c = *p++; prior = c)
			{
			if (quote)
				{
				if (c == '\n')
					{
					if (!escape)
						throw AdminException("unbalanced quote in %s", fileName);
					++lines;
					escape = false;
					}
				else if (escape)
					escape = false;
				else if (c == '\\')
					escape = true;
				else if (c == quote)
					quote = 0;
				continue;
				}
				
			switch (c)
				{
				case '\n':
					++lines;
					if (inFunction)
						++bodyLines;
					if (multiLineComment)
						bumpCommentLines();
					if (singleLineComment)
						{
						singleLineComment = false;
						continue;
						}
					if (white)
						++whiteLines;
					break;
				
				case '/':
					white = false;
					if (multiLineComment && prior == '*')
						{
						multiLineComment = false;
						continue;
						}
					break;
				
				case ' ':
				case '\t':
				case '\r':
					break;
				
				default:
					white = false;
				}
			
			if (singleLineComment || multiLineComment)
				continue;
				
			switch (c)
				{
				case '\'':
				case '"':
					quote = c;
					escape = false;
					break;
					
				case '*':
					if (prior == '/')
						multiLineComment = true;
					break;
				
				case '/':
					if (prior == '/')
						{
						singleLineComment = true;
						bumpCommentLines();
						}
					break;
					
				case '{':
					if (braceLevel == 0)
						{
						numberArguments += args;
						args = 0;
						++numberFunctions;
						inFunction = true;
						whiteLines = 0;
						}
					++braceLevel;
					break;
				
				case '}':
					if (braceLevel == 0)
						throw AdminException("unbalanced braces in %s", fileName);
					if (parenLevel != 0)
						throw AdminException("unmatched parenthesis in %s", fileName);
					--braceLevel;
					
					if (braceLevel == 0 && inFunction)
						{
						inFunction = false;
						internalWhiteLines += whiteLines;
						}
					break;
					
				case '(':
					if (parenLevel == 0 && braceLevel == 0)
						{
						//fputs(line, stdout);
						args = 0;
						if (strncmp(p, "void)", 6) == 0)
							{
							p += 5;
							continue;
							}
						argList = true;
						arg = false;
						}
					++parenLevel;
					break;
				
				case ')':
					if (parenLevel == 0)
						//throw AdminException("unbalanced parenthesis in %s", fileName);
						break;
					if (argList)
						{
						argList = false;
						arg = false;
						}
					--parenLevel;
					break;
				
				case ',':
					if (argList)
						{
						++args;
						arg = false;
						}
					break;
					
				default:
					if (argList && !arg)
						{
						++args;
						arg = true;
						}
				}
			}
		}
	
	fclose(file);
	
	if (parenLevel != 0)
		throw AdminException("unmatched parenthesis in %s", fileName);
	
	if (braceLevel != 0)
		throw AdminException("unmatched braces in %s", fileName);
}

void AnalyseModule::aggregate(AnalyseModule* module)
{
	++numberModules;
	lines += module->lines;
	
	switch (module->type)
		{
		case Code:
		case Preprocessed:
			if (module->type == Code)
				++codeModules;
			else
				++preprocessedModules;
				
			numberFunctions += module->numberFunctions;
			numberArguments += module->numberArguments;
			internalCommentLines += module->internalCommentLines;
			externalCommentLines += module->externalCommentLines;
			bodyLines += module->bodyLines;
			internalWhiteLines += module->internalWhiteLines;
			numberArguments += module->numberArguments;
			numberArguments += module->numberArguments;
			break;
			
		case Header:
			++headerModules;
			break;
		
		default:
			++otherModules;
		}
		
}

Element* AnalyseModule::genSummary(void)
{
	char temp[128];
	
	Element *summary = new Element("ProjectSummary");
	summary->addAttribute("TotalModules", numberModules);
	summary->addAttribute("TotalLines", lines);
	summary->addAttribute("CodeModules", codeModules);
	summary->addAttribute("HeaderModules", headerModules);
	summary->addAttribute("PreprocessedModules", preprocessedModules);
	summary->addAttribute("OtherModules", otherModules);
	summary->addAttribute("NumberFunctions", numberFunctions);
	summary->addAttribute("AverageArguments", formatValue((double) numberArguments / numberFunctions, "%.2f", temp));
	summary->addAttribute("AverageFunctionLines", formatValue((double) bodyLines / numberFunctions, "%.2f", temp));
	summary->addAttribute("AverageInternalComments", formatValue((double) internalCommentLines / numberFunctions, "%.2f", temp));
	summary->addAttribute("AverageInternalWhiteSpace", formatValue((double) internalWhiteLines / numberFunctions, "%.2f", temp));
	summary->addAttribute("AverageCodeLines", 
						  formatValue((double) (bodyLines - internalCommentLines - internalWhiteLines) / numberFunctions, "%.2f", temp));
	
	return summary;
}

const char* AnalyseModule::formatValue(double value, const char *format, char *buffer)
{
	sprintf(buffer, format, value);
	
	return buffer;
}

bool AnalyseModule::isDerivedFile(const char* source)
{
	if (type != Code)
		return false;
	
	for (const char *p = source, *q = module; *p == *q; ++p, ++q)
		;
	
	if (p > source && p[-1] == '.')
		return true;
	
	return false;	
}
