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
#include "AnalyseModule.h"
#include "Stream.h"

static const char *vcprojFile;
static const char *swDirectory = "./";
static const char *singleFile;
static const char *projectName;
static const char *outputFile;
static bool	swHelp;
static bool	swEcho;

static const Switches switches [] =
	{
	WORD_ARG(vcprojFile, "vcproj-file")
	ARG_ARG("-d", swDirectory, "Project directory")
	ARG_ARG("-n", projectName, "Project name")
	ARG_ARG("-f", singleFile, "Module filename (debugging only)")
	ARG_ARG("-o", outputFile, "Output summary file")
	SW_ARG("-e", swEcho, "Echo files")
	SW_ARG("-h", swHelp, "Print this text")
	NULL
	};

static const char *projects [] = {
	"ProjectAnalyst", "-d", "g:\\firebird\\fb2-head\\builds\\win32\\msvc7", "engine.vcproj", "-n", "Firebird Engine", "-o", "firebird.summary",
	"ProjectAnalyst", "-d", "d:\\mysql\\mysql-5.0.16\\sql", "mysqld.vcproj", "-n", "MySQL Server", "-o", "mysql.summary",
	"ProjectAnalyst", "-d", "g:\\firebird\\vulcan\\src\\jrd", "jrd.vcproj", "-n", "Vulcan Engine", "-o", "vulcan.summary",
	"ProjectAnalyst", "-d", "d:\\netfrastructure\\NfsStorageEngine", "NfsStorageEngine.vcproj", "-n", "Netfrastructure DB Engine", "-o", "nfs.summary",
	NULL
	};
	
int main(int argc, const char **argv)
{
	//ProjectAnalyst project(argc, argv);
	for (const char **args = projects; args[0]; args += 8)
		ProjectAnalyst project(8, args);
	
	return 0;
}

ProjectAnalyst::ProjectAnalyst(int argc, const char** argv)
{
	modules = NULL;
	Args args;
	args.parse(switches, argc, argv);
	
	if (singleFile)
		{
		AnalyseModule module;
		module.parseModule(singleFile, true);
		return;
		}
		
	char fileName [512];
	PathName::merge(vcprojFile, swDirectory, sizeof(fileName), fileName);
	InputFile confFile (fileName);
	XMLParse confXml;
	confXml.pushStream (&confFile);
	Element *configurations = confXml.parse();
	delete configurations;
	Element *project = confXml.parseNext();
	Element *files = project->findChild("Files");
	analyse(files);
	AnalyseModule *module;
	
	for (module = modules; module; module = module->next)
		if (module->type == Preprocessed)
			for (AnalyseModule **ptr = &modules; *ptr; ptr = &(*ptr)->next)
				if ((*ptr)->isDerivedFile(module->module))
					{
					*ptr = (*ptr)->next;
					break;
					}
	
	AnalyseModule aggregation;
	
	for (module = modules; module; module = module->next)
		aggregation.aggregate(module);
	
	char *p = fileName;
	
	for (const char *q = vcprojFile; *q && *q != '.';)
		*p++ = *q++;

	*p = 0;
	Element *summary = aggregation.genSummary();
	summary->addAttribute("ProjectName", (projectName) ? projectName : fileName);
	Stream stream;
	summary->genXML(0, &stream);
	
	strcpy(p, ".summary");
	FILE *file = fopen((outputFile) ? outputFile : fileName, "w");
	
	for (Segment *segment = stream.segments; segment; segment = segment->next)
		fwrite(segment->address, 1, segment->length, file);
	
	fclose(file);
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

void ProjectAnalyst::analyse(const char* relativeName)
{
	char fileName[512];
	PathName::merge(relativeName, swDirectory, sizeof(fileName), fileName);
	AnalyseModule *module;
	
	try
		{
		module = new AnalyseModule;
		module->parseModule(fileName, swEcho);
		module->next = modules;
		modules = module;
		}
	catch (AdminException& exception)
		{
		delete module;
		fprintf(stderr, "%s\n", exception.getText());
		}
}

ProjectAnalyst::~ProjectAnalyst(void)
{
	for (AnalyseModule *module; module = modules;)
		{
		modules = module->next;
		delete module;
		}
}
