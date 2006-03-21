#ifndef _BLRPRINT_H_
#define _BLRPRINT_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "fb_types.h"
#include "constants.h"

START_NAMESPACE

class BlrPrint
{
public:
	BlrPrint(const UCHAR* blr, FPTR_PRINT_CALLBACK routine, void* user_arg, SSHORT language);
	~BlrPrint(void);
	
	void		error(const TEXT*, ...) ATTRIBUTE_FORMAT(2,3);
	void		format(const char*, ...) ATTRIBUTE_FORMAT(2,3);
	void		indent(SSHORT);
	void		print_blr(UCHAR);
	SCHAR		print_byte();
	SCHAR		print_char();
	void		print_cond();
	int			print_dtype();
	void		print_join();
	SLONG		print_line(SSHORT);
	void		print_verb(SSHORT);
	int			print_word();


	const UCHAR*	ctl_blr;				/* Running blr string */
	const UCHAR*	ctl_blr_start;		/* Original start of blr string */
	void*			ctl_user_arg;			/* User argument */
	TEXT*			ctl_ptr;
	SSHORT			ctl_language;
	TEXT			ctl_buffer[PRETTY_BUFFER_SIZE];
	FPTR_PRINT_CALLBACK ctl_routine; /* Call back */
	void print(void);
	static void defaultPrint(void* arg, SSHORT offset, const TEXT* line);
};

END_NAMESPACE

#endif
