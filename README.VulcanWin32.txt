The MSVC7 project files for Vulcan are configured 
for an initial boot build.  They contain precompiled 
versions of the .epp files needed for burp, gpre, 
and the dsql files in the engine. 


Before you begin, define the environmental variable
VULCAN to be vulcan\install, where "vulcan" is the
path to the root of the vulcan build tree.  E.g.
G:\builds\vulcan.

If you have not done so, you should create the enviroment 
variables ISC_PASSWORD and ISC_USER.  For the moment,
SYSDBA and masterke are the right values.
 
Add the directory %VULCAN%\bin to the user or
system PATH environmental variable.  This value should
precede the path to other Firebird/InterBase 
installations.

Add the %VULCAN%\bin directory to the Projects settings 
of VC++ option dialog (Tools->Options->Projects->VC++ Directories)

The build requires bison to compile the dsql parse.y
file.  

To avoid overwriting existing MSVC7 "solutions", the 
kit provides a solution file called Vulcan.sln.template 
which you can copy or rename to create the preliminary
solution file for the project.  That file is located in
vulcan\builds\VisualStudio7.  Copy or rename it to
vulcan.sln in the same directory


After downloading the sources intially, and any time you
execute a 'clean' on the tree, you must run the command
file boot_copy.bat from this top level directory. 
