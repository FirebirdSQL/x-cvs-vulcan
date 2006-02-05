#include <stdio.h>
#include <string.h>
#include "fbdev.h"
#include "common.h"
#include "AnalyseModule.h"
#include "AdminException.h"

AnalyseModule::AnalyseModule(const char *fileName)
{
	reset();
	FILE *file = fopen(fileName, "r");
	
	if (!file)
		throw AdminException("can't open \"%s\"", fileName);
	
	char line[512];
	reset();
	char c;
	char prior = 0;
	char quote = 0;
	int whiteLines = 0;
	bool white;
	bool escape = false;
	
	while (fgets(line, sizeof(line), file))
		{
		fputs(line, stdout);
		white = true;
		
		for (const char *p = line; c = *p++; prior = c)
			{
			if (quote)
				{
				if (c == '\n')
					throw AdminException("unbalanced quote in %s", fileName);
				if (escape)
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
						throw AdminException("unbalanced parenthesis in %s", fileName);
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

AnalyseModule::~AnalyseModule(void)
{
}

void AnalyseModule::reset(void)
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
}

void AnalyseModule::bumpCommentLines(void)
{
	if (inFunction)
		++internalCommentLines;
	else
		++externalCommentLines;
}
