// OcsArchive.h: interface for the COcsArchive class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OCSARCHIVE_H__2A5C04CA_B761_11D1_AB1B_0000C01D2301__INCLUDED_)
#define AFX_OCSARCHIVE_H__2A5C04CA_B761_11D1_AB1B_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class COcsArchive  
{
public:
	static void ocsArchiveError (int number, char *text, struct ocs_env *env);
	operator OCS_ENV();
	COcsArchive(CArchive& ar);
	virtual ~COcsArchive();
	static int ocsArchiveGet (void* read_handle, char *data, int length, struct ocs_env*);
	static int ocsArchivePut (void* write_handle, char *data, int length, struct ocs_env*);
	static void* ocsAlloc (int size, struct ocs_env *env);
	static void ocsFree (void *block, struct ocs_env *env);

	CArchive	*archive;
	OCS_ENV		env;
};

#endif // !defined(AFX_OCSARCHIVE_H__2A5C04CA_B761_11D1_AB1B_0000C01D2301__INCLUDED_)
