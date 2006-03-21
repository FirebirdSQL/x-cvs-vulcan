#ifndef _PROJECT_ANALYST_H_
#define _PROJECT_ANALYST_H_

class Element;
class AnalyseModule;

class ProjectAnalyst
{
public:
	ProjectAnalyst(int argc, const char** argv);
	void analyse(Element* element);
	void analyse(const char* fileName);
	
	AnalyseModule	*modules;
	~ProjectAnalyst(void);
};

#endif
