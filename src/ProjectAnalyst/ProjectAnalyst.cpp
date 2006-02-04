#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "fbdev.h"
#include "common.h"
#include "Args.h"
#include "ProjectAnalyst.h"
#include "PathName.h"
#include "ScanDir.h"
#include "AdminException.h"
#include "InputFile.h"
#include "XMLParse.h"
#include "Element.h"
#include "PathName.h"

static const char *vcprojFile;
static bool	swHelp;
static bool	swDirectory = "./";

static const Switches switches [] =
	{
	WORD_ARG(vcprojFile, "vcproj-file")
	SW_ARG("-d", swDirectory, "Project directory")
	SW_ARG("-h", swHelp, "Print this text")
	NULL
	};

static const char *filterFiles [] =
	{ "VisualStudioProject", "Files", "Filter", "File", NULL };
	
static const char *filterFilterFiles [] =
	{ "VisualStudioProject", "Files", "Filter", "Filter", "File", NULL };


int main(int argc, const char **argv)
{
	ProjectAnalyst project(argc, argv);
	
	return 0;
}

ProjectAnalyst::ProjectAnalyst(int argc, const char** argv)
{
	Args args;
	args.parse(switches, argc, argv);
	
	InputFile confFile (vcprojFile);
	XMLParse confXml;
	confXml.pushStream (&confFile);
	Element *configurations = confXml.parse();
	delete configurations;
	Element *project = confXml.parseNext();
	//Element *project = configurations->findChild("VisualStudioProject");
	Element *files = project->findChild("Files");
	analyse(files);
}

void ProjectAnalyst::analyse(Element* element)
{
	for (Element *child = element->children; child; child = child->sibling)
		if (child->name == "File")
			{
			const char *fileName = child->getAttributeValue("RelativePath");
			analyse(fileName);
			}
		else if (child->name == "Filter" || child->name == "Files")
			analyse(child);
			
}

void ProjectAnalyst::analyse(const char* fileName)
{
}
