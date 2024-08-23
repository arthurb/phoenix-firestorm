/**
 * @file llallocator.cpp
 * @brief Implementation of the LLAllocator class.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "linden_common.h"
#include "llallocator.h"

//
// stub implementations for when tcmalloc is disabled
//

void LLAllocator::setProfilingEnabled(bool should_enable)
{
}

// static
bool LLAllocator::isProfiling()
{
    return false;
}

std::string LLAllocator::getRawProfile()
{
    return std::string();
}

LLAllocatorHeapProfile const & LLAllocator::getProfile()
{
    mProf.mLines.clear();

    // *TODO - avoid making all these extra copies of things...
    std::string prof_text = getRawProfile();
    //std::cout << prof_text << std::endl;
    mProf.parse(prof_text);
    return mProf;
}