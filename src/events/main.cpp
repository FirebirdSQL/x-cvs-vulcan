#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "Events.h"

int main(int argc, const char **argv)
{
	const char *dbName = "events.fdb";

	for (int n = 1; n < argc;)
		{
		const char *arg = argv[n++];
		if (*arg == '-')
			switch (arg[1])
				{
				case 'd':
					dbName = argv[n++];
					break;

				default:
					fprintf(stderr, "don't understand switch %s\n", arg);
					return 1;
				}
		}

	isc_db_handle dbHandle = 0;
	isc_attach_database(NULL, 0, dbName,  &dbHandle, 0, NULL);
	//Events events(dbHandle, 2, "george", "martha");
	Events events(dbHandle, 0);
	events.addEvent("george");
	events.addEvent("martha");
	
	if (!events.waitForEvents())
		{
		fprintf (stderr, "event initialization failed\n");
		return 1;
		}
		
	events.cancelEvents();			// unnecessary -- debugging only
	
	for (;;)
		{
		int bits = events.waitForEvents();
		
		if (!bits)
			{
			fprintf (stderr, "event initialization failed\n");
			return 1;
			}
			
		printf ("Event mask: %d\n", bits);
		
		if (bits & 1)
			break;
		}

	isc_detach_database(NULL, &dbHandle);
	
	return 0;
}

