/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by James A. Starkey
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c)  2003 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */

/*
 *  Traditional local Galaxy/Interbase/Firebird functions.
 */

#include <string.h>
#include <memory.h>
#include "firebird.h"
#include "common.h"
#include "ibase.h"
#include "../jrd/event.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/gds_proto.h"

struct EventArg {
	AsyncEvent	event;
	SCHAR	*buffer;
	};

static void stuffInt (UCHAR **ptr, int verb, int value)
{
	UCHAR *p = *ptr;
	*p++ = verb;
	*p++ = 4;
	*p++ = value;
	*p++ = value >> 8;
	*p++ = value >> 16;
	*p++ = value >> 24;
	*ptr = p;
}

static void stuffByte (UCHAR **ptr, int verb, UCHAR value)
{
	UCHAR *p = *ptr;
	*p++ = verb;
	*p++ = 1;
	*p++ = value;
	*ptr = p;
}

static void stuffString (UCHAR **ptr, int verb, const char *string)
{
	UCHAR *p = *ptr;
	*p++ = verb;
	*p++ = strlen (string);

	for (const char *q = string; *q;)
		*p++ = *q++;

	*ptr = p;
}

static void stuffSecData (UCHAR **ptr, const USER_SEC_DATA *data)
{
	UCHAR *p = *ptr;

	stuffString (&p, fb_apb_account, data->user_name);

	if (data->sec_flags & sec_uid_spec)
		stuffInt (&p, fb_apb_uid, data->uid);

	if (data->sec_flags & sec_gid_spec)
		stuffInt (&p, fb_apb_gid, data->gid);

	if (data->sec_flags & sec_server_spec)
		stuffString (&p, fb_apb_uid, data->server);

	if (data->sec_flags & sec_password_spec)
		stuffString (&p, fb_apb_uid, data->password);

	if (data->sec_flags & sec_group_name_spec)
		stuffString (&p, fb_apb_group, data->group_name);

	if (data->sec_flags & sec_first_name_spec)
		stuffString (&p, fb_apb_first_name, data->first_name);

	if (data->sec_flags & sec_middle_name_spec)
		stuffString (&p, fb_apb_middle_name, data->middle_name);

	if (data->sec_flags & sec_last_name_spec)
		stuffString (&p, fb_apb_last_name, data->last_name);

	if (data->sec_flags & sec_dba_user_name_spec)
		stuffString (&p, fb_apb_dba_account, data->dba_user_name);

	if (data->sec_flags & sec_dba_password_spec)
		stuffString (&p, fb_apb_dba_password, data->dba_user_name);

	*ptr = p;
}

extern "C" {

ISC_STATUS ISC_EXPORT isc_add_user(ISC_STATUS * status, const USER_SEC_DATA *data)
	{
	UCHAR apb [512], *p = apb;
	stuffByte (&p, fb_apb_operation, fb_apb_create_account);
	stuffSecData (&p, data);

	return fb_update_account_info (status, NULL, p - apb, apb);
	}

ISC_STATUS API_ROUTINE isc_modify_user(ISC_STATUS * status, const USER_SEC_DATA *data)
	{
	UCHAR apb [512], *p = apb;
	stuffByte (&p, fb_apb_operation, fb_apb_update_account);
	stuffSecData (&p, data);

	return fb_update_account_info (status, NULL, p - apb, apb);
	}

ISC_STATUS API_ROUTINE isc_delete_user(ISC_STATUS * status, const USER_SEC_DATA *data)
	{
	UCHAR apb [512], *p = apb;
	stuffByte (&p, fb_apb_operation, fb_apb_delete_account);
	stuffSecData (&p, data);

	return fb_update_account_info (status, NULL, p - apb, apb);
	}

static void eventAst (EventArg *eventArg, int length, UCHAR *buffer)
	{
	memcpy (eventArg->buffer, buffer, length);
	ISC_event_post (&eventArg->event);
	}

ISC_STATUS API_ROUTINE isc_event_wait (ISC_STATUS *status, isc_db_handle* dbHandle, USHORT eventLength, const SCHAR *events, SCHAR *buffer)
	{
	EventArg eventArg;
	eventArg.buffer = buffer;
	ISC_LONG eventId;
	ISC_event_init(&eventArg.event, 0, 0);
	SLONG eventCount = ISC_event_clear (&eventArg.event);

	if (isc_que_events (status, dbHandle, &eventId, eventLength, events, (FPTR_EVENT_CALLBACK) eventAst, &eventArg))
		return status [1];

	AsyncEvent* vector [1];
	vector [0] = &eventArg.event;
	ISC_event_wait (1, vector, &eventCount, -1, 0, NULL);

	return FB_SUCCESS;
	}


ISC_STATUS API_ROUTINE isc_print_status(const ISC_STATUS *status)
{
	return gds__print_status (status);
}


ISC_STATUS API_ROUTINE isc_wait_for_event(ISC_STATUS *status,
										  isc_db_handle* dbHandle,
										  SSHORT eventsLength,
										  const SCHAR *events, 
										  SCHAR *eventsUpdated)
{
	return isc_event_wait (status, dbHandle, eventsLength, events, eventsUpdated);
}

} /* extern "C" */
