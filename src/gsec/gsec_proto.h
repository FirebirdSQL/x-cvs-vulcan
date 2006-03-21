#ifndef GSEC_PROTO_H
#define GSEC_PROTO_H

#include "../jrd/thd.h"

#ifdef SERVICE_THREAD
THREAD_ENTRY_DECLARE GSEC_main(THREAD_ENTRY_PARAM);
#endif

// Output reporting utilities
void	GSEC_print_status(tsec* tdsec, const ISC_STATUS*);
void	GSEC_error_redirect(tsec* tdsec, const ISC_STATUS*, USHORT, const TEXT*, const TEXT*);
void	GSEC_error(tsec* tdsec, USHORT, const TEXT*, const TEXT*, const TEXT*, const TEXT*,
	const TEXT*);
void	GSEC_print(tsec* tdsec, USHORT, const TEXT*, const TEXT*, const TEXT*, const TEXT*,
	const TEXT*);
void	GSEC_print_partial(tsec* tdsec, USHORT, const TEXT*, const TEXT*, const TEXT*,
	const TEXT*, const TEXT*);

#endif // GSEC_PROTO_H

