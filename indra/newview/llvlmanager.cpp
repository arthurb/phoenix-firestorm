/**
 * @file llvlmanager.cpp
 * @brief LLVLManager class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llvlmanager.h"

#include "indra_constants.h"
#include "patch_code.h"
#include "patch_dct.h"
#include "llviewerregion.h"
#include "llframetimer.h"
#include "llsurface.h"
#include "llbitpack.h"

const   char    LAND_LAYER_CODE                 = 'L';
const   char    WIND_LAYER_CODE                 = '7';
const   char    CLOUD_LAYER_CODE                = '8';

// <FS:CR> Aurora Sim
// Extended land layer for Aurora Sim
const   char    WATER_LAYER_CODE                = 'W';
const   char    AURORA_LAND_LAYER_CODE          = 'M';
const   char    AURORA_WATER_LAYER_CODE         = 'X';
const   char    AURORA_WIND_LAYER_CODE          = '9';
const   char    AURORA_CLOUD_LAYER_CODE         = ':';
// </FS:CR> Aurora Sim

LLVLManager gVLManager;

LLVLManager::~LLVLManager()
{
    S32 i;
    for (i = 0; i < mPacketData.size(); i++)
    {
        delete mPacketData[i];
    }
    mPacketData.clear();
}

void LLVLManager::addLayerData(LLVLData *vl_datap, const S32Bytes mesg_size)
{
// <FS:CR> Aurora Sim
    //if (LAND_LAYER_CODE == vl_datap->mType)
    if (LAND_LAYER_CODE == vl_datap->mType || AURORA_LAND_LAYER_CODE == vl_datap->mType)
// </FS:CR> Aurora Sim
    {
        mLandBits += mesg_size;
    }
// <FS:CR> Aurora Sim
    //else if (WIND_LAYER_CODE == vl_datap->mType)
    else if (WIND_LAYER_CODE == vl_datap->mType || AURORA_WIND_LAYER_CODE == vl_datap->mType)
// </FS:CR> Aurora Sim
    {
        mWindBits += mesg_size;
    }
// <FS:CR> Aurora Sim
    //else if (CLOUD_LAYER_CODE == vl_datap->mType)
    else if (CLOUD_LAYER_CODE == vl_datap->mType || AURORA_CLOUD_LAYER_CODE == vl_datap->mType)
// </FS:CR> Aurora Sim
    {
        mCloudBits += mesg_size;
    }
// <FS:CR> Aurora Sim
    else if (WATER_LAYER_CODE == vl_datap->mType || AURORA_CLOUD_LAYER_CODE == vl_datap->mType)
    {
        mWaterBits += mesg_size * 8;
    }
// </FS:CR> Aurora Sim
    else
    {
        LL_ERRS() << "Unknown layer type!" << (S32)vl_datap->mType << LL_ENDL;
    }

    mPacketData.push_back(vl_datap);
}

void LLVLManager::unpackData(const S32 num_packets)
{
    static LLFrameTimer decode_timer;

    S32 i;
    for (i = 0; i < mPacketData.size(); i++)
    {
        LLVLData *datap = mPacketData[i];

        LLBitPack bit_pack(datap->mData, datap->mSize);
        LLGroupHeader goph;

        decode_patch_group_header(bit_pack, &goph);
        if (LAND_LAYER_CODE == datap->mType)
        {
            datap->mRegionp->getLand().decompressDCTPatch(bit_pack, &goph, false);
        }
// <FS:CR> Aurora Sim
        else if (AURORA_LAND_LAYER_CODE == datap->mType)
        {
            datap->mRegionp->getLand().decompressDCTPatch(bit_pack, &goph, true);
        }
        //else if (WIND_LAYER_CODE == datap->mType)
        else if (WIND_LAYER_CODE == datap->mType || AURORA_WIND_LAYER_CODE == datap->mType)
// </FS:CR> Aurora Sim
        {
            datap->mRegionp->mWind.decompress(bit_pack, &goph);

        }
// <FS:CR> Aurora Sim
        //else if (CLOUD_LAYER_CODE == datap->mType)
        else if (CLOUD_LAYER_CODE == datap->mType || AURORA_CLOUD_LAYER_CODE == datap->mType)
        {

        }
        else if (WATER_LAYER_CODE == datap->mType || AURORA_WATER_LAYER_CODE == datap->mType)
        {
// </FS:CR> Aurora Sim

        }
    }

    for (i = 0; i < mPacketData.size(); i++)
    {
        delete mPacketData[i];
    }
    mPacketData.clear();

}

void LLVLManager::resetBitCounts()
{
    mLandBits = mWindBits = mCloudBits = (S32Bits)0;
}

U32Bits LLVLManager::getLandBits() const
{
    return mLandBits;
}

U32Bits LLVLManager::getWindBits() const
{
    return mWindBits;
}

U32Bits LLVLManager::getCloudBits() const
{
    return mCloudBits;
}

S32Bytes LLVLManager::getTotalBytes() const
{
    return mLandBits + mWindBits + mCloudBits;
}

void LLVLManager::cleanupData(LLViewerRegion *regionp)
{
    S32 cur = 0;
    while (cur < mPacketData.size())
    {
        if (mPacketData[cur]->mRegionp == regionp)
        {
            delete mPacketData[cur];
            mPacketData.erase(mPacketData.begin() + cur);
        }
        else
        {
            cur++;
        }
    }
}

LLVLData::LLVLData(LLViewerRegion *regionp, const S8 type, U8 *data, const S32 size)
{
    mType = type;
    mData = data;
    mRegionp = regionp;
    mSize = size;
}

LLVLData::~LLVLData()
{
    delete [] mData;
    mData = NULL;
    mRegionp = NULL;
}
