#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "firebird.h"
#include "ibase.h"
#include "StatusPrint.h"
#include "gds_proto.h"
#include "ib_stdio.h"

#ifdef _WIN32
#include <windows.h>
#endif

// The following defines a static vector of message strings

#include "gen/msgs.h"

StatusPrint::StatusPrint(void)
{
}

StatusPrint::~StatusPrint(void)
{
}

ISC_STATUS StatusPrint::printStatus(const ISC_STATUS *statusVector)
{
	if (!statusVector || (!statusVector[1] && statusVector[2] == isc_arg_end))
		return FB_SUCCESS;

	const ISC_STATUS* vector = statusVector;
	char buffer [1024];
	
	if (!interpretStatus(sizeof (buffer), buffer, &vector))
		return statusVector[1];

	putError (buffer);
	buffer[0] = '-';

	while (interpretStatus(sizeof (buffer) - 1, buffer + 1, &vector))
		putError(buffer);

	return statusVector[1];
}

void StatusPrint::putError(const char *text)
{
//	printf ("%s\n", text);
	ib_fputs(text, ib_stderr);
	ib_fputc('\n', ib_stderr);
	ib_fflush(ib_stderr);
}

ISC_STATUS StatusPrint::interpretStatus(int bufferLength, char* buffer, const ISC_STATUS** vectorPtr)
{
	const ISC_STATUS *vector = *vectorPtr;
	
	if (vector[0] == isc_arg_tkts_error) 
		vector += 2;

	if (!*vector)
		return 0;

	const ISC_STATUS* v;
	ISC_STATUS code;
	
	/* handle a case: "no errors, some warning(s)" */
	
	if (vector[1] == 0 && vector[2] == isc_arg_warning) 
		{
		v = vector + 4;
		code = vector[3];
		}
	else 
		{
		v = vector + 2;
		code = vector[1];
		}

	char* args[10], **arg = args;
	char temp [256];
	char *p = temp, *endTemp = temp + sizeof (temp) - 1;
	
	/* Parse and collect any arguments that may be present */
	
	for (;;) 
		{
		ISC_STATUS type = *v++;
		switch (type) 
			{
			case isc_arg_string:
			case isc_arg_number:
				*arg++ = (char*) *v++;
				continue;

			case isc_arg_cstring:
				{
				int l = (int) *v++;
				const char *q = (const char*) *v++;
				*arg++ = p;
				
				for (; p < endTemp && l > 0; --l)
					*p++ = *q++;
					
				if (p < endTemp)
					*p++ = 0;
				}
				continue;

			default:
				break;
			}
		--v;
		break;
		}

	/* Handle primary code on a system by system basis */

	switch (vector[0]) 
		{
		case isc_arg_warning:
		case isc_arg_gds:
			{
			USHORT fac = 0, class_ = 0;
			ISC_STATUS decoded = gds__decode(code, &fac, &class_);
			
			if (gds__msg_format(0, fac, decoded,
								128, buffer, args[0], args[1], args[2], args[3],
								args[4]) < 0)
				{
				if ((decoded < FB_NELEM(messages) - 1) && (decoded >= 0))
					sprintf(buffer, messages[decoded], args[0], args[1], args[2],
							args[3], args[4]);
				else
					sprintf(buffer, "unknown ISC error %ld", code);	/* TXNN */
				}
			}
			break;

		case isc_arg_interpreted:
			{
			p = buffer;
			
			for (const char *q = (const char*) vector[1]; *p++ = *q++;)
				;
			}
			break;

		case isc_arg_unix:
		case isc_arg_dos:
		case isc_arg_vms:
		case isc_arg_win32:
			getOSText(vector[0], code, bufferLength, buffer);
			break;

		default:
			return 0;
		}
		
	*vectorPtr = v;

	return (int) strlen (buffer);
}

void StatusPrint::getOSText(int type, int code, int bufferLength, TEXT* buffer)
{
	switch (type)
		{
#ifdef WIN_NT
		case isc_arg_win32:
			if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,
								code,
								GetUserDefaultLangID(),
								buffer,
								bufferLength,
								NULL) &&
				!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,
								code,
								0, // TMN: Fallback to system known language
								buffer,
								bufferLength,
								NULL))
				snprintf(buffer, bufferLength, "unknown Win32 error %ld", code);	/* TXNN */
				
			break;
#endif

#ifdef VMS
		case isc_arg_vms:
			{
			l = 0;
			struct dsc$descriptor_s desc;
			desc.dsc$b_class = DSC$K_CLASS_S;
			desc.dsc$b_dtype = DSC$K_DTYPE_T;
			desc.dsc$w_length = 128;
			desc.dsc$a_pointer = s;
			TEXT flags[4];
			ISC_STATUS status = sys$getmsg(code, &l, &desc, 15, flags);
			
			if (status & 1)
				s[l] = 0;
			else
				sprintf(s, "uninterpreted VMS code %x", code);	/* TXNN */
				
			}
			break;
#endif

		case isc_arg_unix:
			/* The  strerror()  function  returns  the appropriate description
				string, or an unknown error message if the error code is unknown. */
			snprintf(buffer, bufferLength, "%s", strerror(code));	/* TXNN */
			break;

		case isc_arg_dos:
			snprintf(buffer, bufferLength, "unknown dos error %ld", code);	/* TXNN */
			break;
		
		default:
			snprintf(buffer, bufferLength, "unknown system error code %d/%d", type, code);
		}
}
