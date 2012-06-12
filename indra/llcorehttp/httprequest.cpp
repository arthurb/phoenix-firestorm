/**
 * @file httprequest.cpp
 * @brief Implementation of the HTTPRequest class
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

#include "httprequest.h"

#include "_httprequestqueue.h"
#include "_httpreplyqueue.h"
#include "_httpservice.h"
#include "_httppolicy.h"
#include "_httpoperation.h"
#include "_httpoprequest.h"
#include "_httpopsetpriority.h"
#include "_httpopcancel.h"
#include "_httpopsetget.h"

#include "lltimer.h"


namespace
{

bool has_inited(false);

}

namespace LLCore
{

// ====================================
// HttpRequest Implementation
// ====================================


HttpRequest::policy_t HttpRequest::sNextPolicyID(1);


HttpRequest::HttpRequest()
	: //HttpHandler(),
	  mReplyQueue(NULL),
	  mRequestQueue(NULL)
{
	mRequestQueue = HttpRequestQueue::instanceOf();
	mRequestQueue->addRef();

	mReplyQueue = new HttpReplyQueue();
}


HttpRequest::~HttpRequest()
{
	if (mRequestQueue)
	{
		mRequestQueue->release();
		mRequestQueue = NULL;
	}

	if (mReplyQueue)
	{
		mReplyQueue->release();
		mReplyQueue = NULL;
	}
}


// ====================================
// Policy Methods
// ====================================


HttpStatus HttpRequest::setPolicyGlobalOption(EGlobalPolicy opt, long value)
{
	// *FIXME:  Fail if thread is running.

	return HttpService::instanceOf()->getGlobalOptions().set(opt, value);
}


HttpStatus HttpRequest::setPolicyGlobalOption(EGlobalPolicy opt, const std::string & value)
{
	// *FIXME:  Fail if thread is running.

	return HttpService::instanceOf()->getGlobalOptions().set(opt, value);
}


HttpRequest::policy_t HttpRequest::createPolicyClass()
{
	policy_t policy_id = 1;
	
	return policy_id;
}


HttpStatus HttpRequest::setPolicyClassOption(policy_t policy_id,
											 EClassPolicy opt,
											 long value)
{
	HttpStatus status;

	return status;
}


// ====================================
// Request Methods
// ====================================


HttpStatus HttpRequest::getStatus() const
{
	return mLastReqStatus;
}


HttpHandle HttpRequest::requestGetByteRange(policy_t policy_id,
											priority_t priority,
											const std::string & url,
											size_t offset,
											size_t len,
											HttpOptions * options,
											HttpHeaders * headers,
											HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpRequest * op = new HttpOpRequest();
	if (! (status = op->setupGetByteRange(policy_id, priority, url, offset, len, options, headers)))
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	op->setReplyPath(mReplyQueue, user_handler);
	mRequestQueue->addOp(op);			// transfers refcount
	
	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpHandle HttpRequest::requestPost(policy_t policy_id,
									priority_t priority,
									const std::string & url,
									BufferArray * body,
									HttpOptions * options,
									HttpHeaders * headers,
									HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpRequest * op = new HttpOpRequest();
	if (! (status = op->setupPost(policy_id, priority, url, body, options, headers)))
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	op->setReplyPath(mReplyQueue, user_handler);
	mRequestQueue->addOp(op);			// transfers refcount
	
	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpHandle HttpRequest::requestPut(policy_t policy_id,
								   priority_t priority,
								   const std::string & url,
								   BufferArray * body,
								   HttpOptions * options,
								   HttpHeaders * headers,
								   HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpRequest * op = new HttpOpRequest();
	if (! (status = op->setupPut(policy_id, priority, url, body, options, headers)))
	{
		op->release();
		mLastReqStatus = status;
		return handle;
	}
	op->setReplyPath(mReplyQueue, user_handler);
	mRequestQueue->addOp(op);			// transfers refcount
	
	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpHandle HttpRequest::requestNoOp(HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpNull * op = new HttpOpNull();
	op->setReplyPath(mReplyQueue, user_handler);
	mRequestQueue->addOp(op);			// transfer refcount as well

	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);
	
	return handle;
}


HttpStatus HttpRequest::update(long millis)
{
	const HttpTime limit(totalTime() + (1000 * HttpTime(millis)));
	HttpOperation * op(NULL);
	while (limit >= totalTime() && (op = mReplyQueue->fetchOp()))
	{
		// Process operation
		op->visitNotifier(this);
		
		// We're done with the operation
		op->release();
	}
	
	return HttpStatus();
}




// ====================================
// Request Management Methods
// ====================================

HttpHandle HttpRequest::requestCancel(HttpHandle handle, HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle ret_handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpCancel * op = new HttpOpCancel(handle);
	op->setReplyPath(mReplyQueue, user_handler);
	mRequestQueue->addOp(op);			// transfer refcount as well

	mLastReqStatus = status;
	ret_handle = static_cast<HttpHandle>(op);
	
	return ret_handle;
}


HttpHandle HttpRequest::requestSetPriority(HttpHandle request, priority_t priority,
										   HttpHandler * handler)
{
	HttpStatus status;
	HttpHandle ret_handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpSetPriority * op = new HttpOpSetPriority(request, priority);
	op->setReplyPath(mReplyQueue, handler);
	mRequestQueue->addOp(op);			// transfer refcount as well

	mLastReqStatus = status;
	ret_handle = static_cast<HttpHandle>(op);
	
	return ret_handle;
}


// ====================================
// Utility Methods
// ====================================

HttpStatus HttpRequest::createService()
{
	HttpStatus status;

	llassert_always(! has_inited);
	HttpRequestQueue::init();
	HttpRequestQueue * rq = HttpRequestQueue::instanceOf();
	HttpService::init(rq);
	has_inited = true;
	
	return status;
}


HttpStatus HttpRequest::destroyService()
{
	HttpStatus status;

	llassert_always(has_inited);
	HttpService::term();
	HttpRequestQueue::term();
	has_inited = false;
	
	return status;
}


HttpStatus HttpRequest::startThread()
{
	HttpStatus status;

	HttpService::instanceOf()->startThread();
	
	return status;
}


HttpHandle HttpRequest::requestStopThread(HttpHandler * user_handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpStop * op = new HttpOpStop();
	op->setReplyPath(mReplyQueue, user_handler);
	mRequestQueue->addOp(op);			// transfer refcount as well

	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);

	return handle;
}

// ====================================
// Dynamic Policy Methods
// ====================================

HttpHandle HttpRequest::requestSetHttpProxy(const std::string & proxy, HttpHandler * handler)
{
	HttpStatus status;
	HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);

	HttpOpSetGet * op = new HttpOpSetGet();
	op->setupSet(GP_HTTP_PROXY, proxy);
	op->setReplyPath(mReplyQueue, handler);
	mRequestQueue->addOp(op);			// transfer refcount as well

	mLastReqStatus = status;
	handle = static_cast<HttpHandle>(op);

	return handle;
}


}   // end namespace LLCore

