#ifndef _DIALECT_H_
#define _DIALECT_H_

#define SQL_DIALECT_V5				1	/* meaning is same as DIALECT_xsqlda */
#define SQL_DIALECT_V6_TRANSITION	2	/* flagging anything that is delimited
										   by double quotes as an error and
										   flagging keyword DATE as an error */
#define SQL_DIALECT_V6				3	/* supports SQL delimited identifier,
										   SQLDATE/DATE, TIME, TIMESTAMP,
										   CURRENT_DATE, CURRENT_TIME,
										   CURRENT_TIMESTAMP, and 64-bit exact
										   numeric type */
#define SQL_DIALECT_CURRENT		SQL_DIALECT_V6	/* latest IB DIALECT */

#endif
