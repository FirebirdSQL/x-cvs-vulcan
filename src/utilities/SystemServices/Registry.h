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
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by Paul Reeves for the Firebird Project.
 *  Copyright (c) 2006 Paul Reeves
 *  All Rights Reserved.
 *
 *
 */


static const char* REG_KEY_ROOT_COMPANY	= "SOFTWARE\\Firebird Project";
static const char* REG_KEY_ROOT_PRODUCT	= "SOFTWARE\\Firebird Project\\Firebird Server";

// This was originally intended to support multiple instances. The intended usage might now be 
// deprecated.
static const char* REG_KEY_ROOT_INSTANCES	= "SOFTWARE\\Firebird Project\\Firebird Server\\Instances";

//FB_DEFAULT_INSTANCE refers to the default instance of this version of Firebird.
static const char* FB_DEFAULT_INSTANCE	= "Vulcan";


#ifdef USE_REGISTRY_CLASS

class Registry
{
public
}
#endif
