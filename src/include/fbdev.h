#ifndef INCLUDE_FBDev
#define INCLUDE_FBDev

/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *  All rights reserved.
 *
 *  Contributor(s):
 *       Mark O'Donohue <mark.odonohue@ludwig.edu.au>
 *       Mike Nordell   <tamlin@algonet.se>
 *       John Bellardo  <bellardo@cs.ucsd.edu>
 *
 *
 *  $Id$
 *
 */


#include "gen/autoconfig.h"

#ifdef NAMESPACE
namespace NAMESPACE{}		// declare namespace before use
using namespace NAMESPACE;
#define START_NAMESPACE		namespace NAMESPACE {
#define CLASS(cls)			namespace NAMESPACE { class cls; };
#define END_NAMESPACE		}
#else
#define START_NAMESPACE
#define CLASS(cls)			class cls;
#define END_NAMESPACE
#endif

#if defined(WIN_NT)
#define FB_DLL_EXPORT __declspec(dllexport)
#else
#define FB_DLL_EXPORT
#endif
#if defined(SOLX86)
#define __PRAGMA_REDEFINE_EXTNAME 
#define __EXTENSIONS__
#endif

//
// Macro for function attribute definition
//
#if defined(__GNUC__)
#define ATTRIBUTE_FORMAT(a,b) __attribute__ ((format(printf,a,b)))
#else
#define ATTRIBUTE_FORMAT(a,b)
#endif

#ifndef NO_SHARED_CACHE
#define SHARED_CACHE
#endif

#ifdef __cplusplus
#include "fb_exception.h"
#endif

// 
#if defined(SUPERSERVER) || defined(WIN_NT)
//#define SERVER_SHUTDOWN		1
#endif

// Check if we need thread synchronization
//#if defined(SUPERSERVER) || defined(SUPERCLIENT) || \
//    defined(WIN_NT) || defined(SOLARIS_MT) || defined (VMS)
#define MULTI_THREAD 1
//#endif

#ifndef NULL
#define NULL            0L
#endif

#if defined __cplusplus && (defined __GNUC__ && !defined(MINGW))
#include "../common/classes/alloc.h"
#endif

#include "common.h"

#endif /* INCLUDE_Firebird */
