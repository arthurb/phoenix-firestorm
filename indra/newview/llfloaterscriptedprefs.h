/**
 * @file llfloaterscriptedprefs.h
 * @brief Color controls for the script editor
 * @author Cinder Roxley
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#ifndef LL_FLOATERSCRIPTEDPREFS_H
#define LL_FLOATERSCRIPTEDPREFS_H

#include "llfloater.h"

class LLScriptEditor;
class LLUICtrl;

class LLFloaterScriptEdPrefs : public LLFloater
{
public:
    LLFloaterScriptEdPrefs(const LLSD& key);
    bool postBuild() override;

private:
    ~LLFloaterScriptEdPrefs() {};

    void applyUIColor(LLUICtrl* ctrl, const LLSD& param);
    void getUIColor(LLUICtrl* ctrl, const LLSD& param);

    // <FS:Ansariel> Port old FS script prefs
    void setPreprocInclude();
    void changePreprocIncludePath(const std::vector<std::string>& filenames, std::string proposed_name);

    LLScriptEditor* mEditor;
};

#endif // LL_FLOATERSCRIPTEDPREFS_H
