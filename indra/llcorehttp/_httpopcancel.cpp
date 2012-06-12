/**
 * @file _httpopcancel.cpp
 * @brief Definitions for internal class HttpOpCancel
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "_httpopcancel.h"

#include <cstdio>
#include <algorithm>

#include "httpcommon.h"
#include "httphandler.h"
#include "httpresponse.h"

#include "_httprequestqueue.h"
#include "_httpreplyqueue.h"
#include "_httpservice.h"
#include "_httppolicy.h"
#include "_httplibcurl.h"


namespace LLCore
{


// ==================================
// HttpOpCancel
// ==================================


HttpOpCancel::HttpOpCancel(HttpHandle handle)
	: HttpOperation(),
	  mHandle(handle)
{}


HttpOpCancel::~HttpOpCancel()
{}


void HttpOpCancel::stageFromRequest(HttpService * service)
{
	// *FIXME:  Need cancel functionality into services
	addAsReply();
}


}   // end namespace LLCore

		
