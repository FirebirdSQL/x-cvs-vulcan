#ifndef _ANALYSE_MODULE_H_
#define _ANALYSE_MODULE_H_

class AnalyseModule
{
public:
	AnalyseModule(const char *fileName);
	~AnalyseModule(void);
	void reset(void);
	void bumpCommentLines(void);

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
};

#endif
