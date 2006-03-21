
Notes regarding some of the files in this directory, as of March 6, 2006.


1.	ThreadTest.cpp is the original thread test program, which has been
	structured as a VC project file.

2.	threadtest1.c .. threadtest3.c are the test programs submitted by
	SAS as they are used at SAS.  They have not (yet) been updated to
	run as VC projects, and today must be built using gcc or some other
	command line tool.

3.	The source files submitted by SAS have a #define for SSA_OS_UNIX.
	This is a symbol used by our build system; this should be defined
	if a build on a Unix system is being done that requires Unix header
	<sys/resource.h>, for example.

4.	These tests should be wrappered correctly so that they can be built
	more easily by anyone.  As of the initial commit of threadtest2 and
	threadtest3, the primary goal is to expose the testware for review.
	A followup push to support more conforming builds will be required.

5.	Additionally, we'll need to push better documentation on how to run
	the test tools.  For example,

		c:\home> threadtest3 mytestdb.fdb 10

	runs the test stream against the database 'mytest.fdb' and uses 10
	threads to construct connections.  Today, many of the test config
	parameters are compile-time constants, which should be cleaned up.


