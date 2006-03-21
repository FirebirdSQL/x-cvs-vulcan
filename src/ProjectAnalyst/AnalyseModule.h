#ifndef _ANALYSE_MODULE_H_
#define _ANALYSE_MODULE_H_

#include "JString.h"

enum ModuleType {
	Code,
	Header,
	Preprocessed,
	Other
	};

class Element;

class AnalyseModule
{
public:
	AnalyseModule();
	~AnalyseModule(void);
	void		bumpCommentLines(void);
	void		parseModule(const char* fileName, bool echo);
	void		aggregate(AnalyseModule* module);
	Element*	genSummary(void);
	const char* formatValue(double value, const char *format, char *buffer);

	AnalyseModule	*next;
	ModuleType		type;
	JString			module;
	
	bool	lineComment;
	bool	multiLineComment;
	bool	singleLineComment;
	bool	inFunction;
	bool	argList;
	bool	arg;
	int		lines;
	int		args;
	int		numberArguments;
	int		numberFunctions;
	int		bodyLines;
	int		internalCommentLines;
	int		externalCommentLines;
	int		internalWhiteLines;
	int		parenLevel;
	int		braceLevel;
	
	int		numberModules;
	int		codeModules;
	int		headerModules;
	int		preprocessedModules;
	int		otherModules;
	bool isDerivedFile(const char* source);
};

#endif
