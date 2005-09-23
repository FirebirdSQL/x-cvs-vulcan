#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "Events.h"

int main(int argc, const char *argv)
{
	isc_db_handle dbHandle = NULL;
	isc_attach_database(NULL, 0, "localhost:events.fdb", &dbHandle, 0, NULL);
	Events events(dbHandle, 2, "george", "martha");
	
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

