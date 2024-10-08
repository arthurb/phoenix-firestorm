/**
 *
 * Copyright (c) 2009-2011, Kitty Barnett
 *
 * The source code in this file is provided to you under the terms of the
 * GNU Lesser General Public License, version 2.1, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. Terms of the LGPL can be found in doc/LGPL-licence.txt
 * in this distribution, or online at http://www.gnu.org/licenses/lgpl-2.1.txt
 *
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to
 * abide by those obligations.
 *
 */

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llavatarnamecache.h"
#include <llchatentry.h>
#include "llclipboard.h"
#include "llcombobox.h"
#include "llinventoryfunctions.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "lltexteditor.h"
#include "llviewerjointattachment.h"
#include "llviewerobjectlist.h"
#include "llvoavatarself.h"

#include "rlvfloaters.h"
#include "rlvhelper.h"
#include "rlvhandler.h"
#include "rlvlocks.h"

// ============================================================================
// Helper functions
//

// Checked: 2012-07-14 (RLVa-1.4.7)
std::string rlvGetItemName(const LLViewerInventoryItem* pItem)
{
    if ( (pItem) && ((LLAssetType::AT_BODYPART == pItem->getType()) || (LLAssetType::AT_CLOTHING == pItem->getType())) )
    {
        return llformat("%s (%s)", pItem->getName().c_str(), LLWearableType::getInstance()->getTypeName(pItem->getWearableType()).c_str());
    }
    else if ( (pItem) && (LLAssetType::AT_OBJECT == pItem->getType()) && (isAgentAvatarValid()) )
    {
        std::string strAttachPtName;
        gAgentAvatarp->getAttachedPointName(pItem->getUUID(), strAttachPtName);
        return llformat("%s (%s)", pItem->getName().c_str(), strAttachPtName.c_str());
    }
    return (pItem) ? pItem->getName() : LLStringUtil::null;
}

// Checked: 2012-07-14 (RLVa-1.4.7)
std::string rlvGetItemType(const LLViewerInventoryItem* pItem)
{
    if (pItem)
    {
        switch (pItem->getType())
        {
            case LLAssetType::AT_BODYPART:
            case LLAssetType::AT_CLOTHING:
                return "Wearable";
            case LLAssetType::AT_OBJECT:
                return "Attachment";
            default:
                break;
        }
    }
    return "Unknown";
}

std::string rlvGetItemNameFromObjID(const LLUUID& idObj, bool fIncludeAttachPt = true)
{
    const LLViewerObject* pObj = gObjectList.findObject(idObj);
    if ( (pObj) && (pObj->isAvatar()) )
    {
        LLAvatarName avName;
        if (LLAvatarNameCache::get(pObj->getID(), &avName))
            return avName.getCompleteName();
        return ((LLVOAvatar*)pObj)->getFullname();
    }

    const LLViewerObject* pObjRoot = (pObj) ? pObj->getRootEdit() : NULL;
    const LLViewerInventoryItem* pItem = ((pObjRoot) && (pObjRoot->isAttachment())) ? gInventory.getItem(pObjRoot->getAttachmentItemID()) : NULL;

    const std::string strItemName = (pItem) ? pItem->getName() : idObj.asString();
    if ( (!fIncludeAttachPt) || (!pObj) || (!pObj->isAttachment()) || (!isAgentAvatarValid()) )
        return strItemName;

    const LLViewerJointAttachment* pAttachPt = get_if_there(gAgentAvatarp->mAttachmentPoints, RlvAttachPtLookup::getAttachPointIndex(pObjRoot), (LLViewerJointAttachment*)NULL);
    const std::string strAttachPtName = (pAttachPt) ? pAttachPt->getName() : std::string("Unknown");
    return llformat("%s (%s%s)", strItemName.c_str(), strAttachPtName.c_str(), (pObj == pObjRoot) ? "" : ", child");
}

// Checked: 2011-05-23 (RLVa-1.3.0c) | Added: RLVa-1.3.0c
bool rlvGetShowException(ERlvBehaviour eBhvr)
{
    switch (eBhvr)
    {
        case RLV_BHVR_RECVCHAT:
        case RLV_BHVR_RECVEMOTE:
        case RLV_BHVR_SENDIM:
        case RLV_BHVR_RECVIM:
        case RLV_BHVR_STARTIM:
        case RLV_BHVR_TPLURE:
        case RLV_BHVR_TPREQUEST:
        case RLV_BHVR_ACCEPTTP:
        case RLV_BHVR_ACCEPTTPREQUEST:
        case RLV_BHVR_SHOWNAMES:
        case RLV_BHVR_SHOWNAMETAGS:
            return true;
        default:
            return false;
    }
}

// Checked: 2012-07-29 (RLVa-1.4.7)
std::string rlvLockMaskToString(ERlvLockMask eLockType)
{
    switch (eLockType)
    {
        case RLV_LOCK_ADD:
            return "add";
        case RLV_LOCK_REMOVE:
            return "rem";
        default:
            return "unknown";
    }
}

// Checked: 2012-07-29 (RLVa-1.4.7)
std::string rlvFolderLockPermissionToString(RlvFolderLocks::ELockPermission eLockPermission)
{
    switch (eLockPermission)
    {
        case RlvFolderLocks::PERM_ALLOW:
            return "allow";
        case RlvFolderLocks::PERM_DENY:
            return "deny";
        default:
            return "unknown";
    }
}

// Checked: 2012-07-29 (RLVa-1.4.7)
std::string rlvFolderLockScopeToString(RlvFolderLocks::ELockScope eLockScope)
{
    switch (eLockScope)
    {
        case RlvFolderLocks::SCOPE_NODE:
            return "node";
        case RlvFolderLocks::SCOPE_SUBTREE:
            return "subtree";
        default:
            return "unknown";
    }
}

// Checked: 2012-07-29 (RLVa-1.4.7)
std::string rlvFolderLockSourceToTarget(RlvFolderLocks::folderlock_source_t lockSource)
{
    switch (lockSource.first)
    {
        case RlvFolderLocks::ST_ATTACHMENT:
            {
                std::string strAttachName = rlvGetItemNameFromObjID(boost::get<LLUUID>(lockSource.second));
                return llformat("Attachment (%s)", strAttachName.c_str());
            }
        case RlvFolderLocks::ST_ATTACHMENTPOINT:
            {
                const LLViewerJointAttachment* pAttachPt = RlvAttachPtLookup::getAttachPoint(boost::get<S32>(lockSource.second));
                return llformat("Attachment point (%s)", (pAttachPt) ? pAttachPt->getName().c_str() : "Unknown");
            }
        case RlvFolderLocks::ST_FOLDER:
            {
                return "Folder: <todo>";
            }
        case RlvFolderLocks::ST_ROOTFOLDER:
            {
                return "Root folder";
            }
        case RlvFolderLocks::ST_SHAREDPATH:
            {
                const std::string& strPath = boost::get<std::string>(lockSource.second);
                return llformat("Shared path (#RLV%s%s)", (!strPath.empty()) ? "/" : "", strPath.c_str());
            }
        case RlvFolderLocks::ST_WEARABLETYPE:
            {
                const std::string& strTypeName = LLWearableType::getInstance()->getTypeName(boost::get<LLWearableType::EType>(lockSource.second));
                return llformat("Wearable type (%s)", strTypeName.c_str());
            }
        default:
            {
                return "(Unknown)";
            }
    }
}

// ============================================================================
// RlvFloaterBehaviours member functions
//

// Checked: 2010-04-18 (RLVa-1.3.1c) | Modified: RLVa-1.2.0e
void RlvFloaterBehaviours::onOpen(const LLSD& sdKey)
{
    m_ConnRlvCommand = gRlvHandler.setCommandCallback(boost::bind(&RlvFloaterBehaviours::onCommand, this, _1, _2));

    refreshAll();
}

// Checked: 2010-04-18 (RLVa-1.3.1c) | Modified: RLVa-1.2.0e
void RlvFloaterBehaviours::onClose(bool fQuitting)
{
    m_ConnRlvCommand.disconnect();
}

// Checked: 2010-04-18 (RLVa-1.3.1c) | Modified: RLVa-1.2.0e
void RlvFloaterBehaviours::onAvatarNameLookup(const LLUUID& idAgent, const LLAvatarName& avName)
{
    uuid_vec_t::iterator itLookup = std::find(m_PendingLookup.begin(), m_PendingLookup.end(), idAgent);
    if (itLookup != m_PendingLookup.end())
        m_PendingLookup.erase(itLookup);
    if (getVisible())
        refreshAll();
}

// static
std::string RlvFloaterBehaviours::getFormattedBehaviourString(ERlvBehaviourFilter eFilter)
{
    std::ostringstream strRestrictions;

    strRestrictions << RlvStrings::getVersion(LLUUID::null) << "\n";

    for (const auto& rlvObjectEntry : RlvHandler::instance().getObjectMap())
    {
        strRestrictions << "\n" << rlvGetItemNameFromObjID(rlvObjectEntry.first) << ":\n";
        for (const RlvCommand& rlvCmd : rlvObjectEntry.second.getCommandList())
        {
            bool fIsException = (rlvCmd.hasOption()) && (rlvGetShowException(rlvCmd.getBehaviourType()));
            if ( ((ERlvBehaviourFilter::BEHAVIOURS_ONLY == eFilter) && (fIsException)) ||
                 ((ERlvBehaviourFilter::EXCEPTIONS_ONLY == eFilter) && (!fIsException)) )
            {
                continue;
            }

            std::string strOption; LLUUID idOption;
            if ( (rlvCmd.hasOption()) && (idOption.set(rlvCmd.getOption(), false)) && (idOption.notNull()) )
            {
                LLAvatarName avName;
                if (gObjectList.findObject(idOption))
                    strOption = rlvGetItemNameFromObjID(idOption, true);
                else if (LLAvatarNameCache::get(idOption, &avName))
                    strOption = (!avName.getAccountName().empty()) ? avName.getAccountName() : avName.getDisplayName();
                else if (!gCacheName->getGroupName(idOption, strOption))
                    strOption = rlvCmd.getOption();
            }

            strRestrictions << "  -> " << rlvCmd.asString();
            if ((!strOption.empty()) && (strOption != rlvCmd.getOption()))
                strRestrictions << "  [" << strOption << "]";
            strRestrictions << "\n";
        }
    }

    return strRestrictions.str();
}

// Checked: 2011-05-26 (RLVa-1.3.1c) | Added: RLVa-1.3.1c
void RlvFloaterBehaviours::onBtnCopyToClipboard()
{
    LLWString wstrRestrictions = utf8str_to_wstring(getFormattedBehaviourString(ERlvBehaviourFilter::ALL));
    LLClipboard::instance().copyToClipboard(wstrRestrictions, 0, static_cast<S32>(wstrRestrictions.length()));
}

// Checked: 2011-05-23 (RLVa-1.3.1c) | Modified: RLVa-1.3.1c
void RlvFloaterBehaviours::onCommand(const RlvCommand& rlvCmd, ERlvCmdRet eRet)
{
    if ( (RLV_TYPE_ADD == rlvCmd.getParamType()) || (RLV_TYPE_REMOVE == rlvCmd.getParamType()) )
        refreshAll();
}

// Checked: 2011-05-23 (RLVa-1.3.1c) | Added: RLVa-1.3.1c
bool RlvFloaterBehaviours::postBuild()
{
    getChild<LLUICtrl>("copy_btn")->setCommitCallback(boost::bind(&RlvFloaterBehaviours::onBtnCopyToClipboard, this));
    return true;
}

void RlvFloaterBehaviours::refreshAll()
{
    LLCtrlListInterface* pBhvrList = childGetListInterface("behaviour_list");
    LLCtrlListInterface* pExceptList = childGetListInterface("exception_list");
    LLCtrlListInterface* pModifierList = childGetListInterface("modifier_list");
    if ( (!pBhvrList) || (!pExceptList) || (!pModifierList) )
        return;
    pBhvrList->operateOnAll(LLCtrlListInterface::OP_DELETE);
    pExceptList->operateOnAll(LLCtrlListInterface::OP_DELETE);
    pModifierList->operateOnAll(LLCtrlListInterface::OP_DELETE);

    if (!isAgentAvatarValid())
        return;

    //
    // Set-up a row we can just reuse
    //
    LLSD sdBhvrRow; LLSD& sdBhvrColumns = sdBhvrRow["columns"];
    sdBhvrColumns[0] = LLSD().with("column", "behaviour").with("type", "text");
    sdBhvrColumns[1] = LLSD().with("column", "issuer").with("type", "text");

    LLSD sdExceptRow; LLSD& sdExceptColumns = sdExceptRow["columns"];
    sdExceptColumns[0] = LLSD().with("column", "behaviour").with("type", "text");
    sdExceptColumns[1] = LLSD().with("column", "option").with("type", "text");
    sdExceptColumns[2] = LLSD().with("column", "issuer").with("type", "text");

    LLSD sdModifierRow; LLSD& sdModifierColumns = sdModifierRow["columns"];
    sdModifierColumns[0] = LLSD().with("column", "modifier").with("type", "text");
    sdModifierColumns[1] = LLSD().with("column", "value").with("type", "text");
    sdModifierColumns[2] = LLSD().with("column", "primary").with("type", "text");

    //
    // List behaviours
    //
    for (const auto& rlvObjectEntry : RlvHandler::instance().getObjectMap())
    {
        const std::string strIssuer = rlvGetItemNameFromObjID(rlvObjectEntry.first);

        for (const RlvCommand& rlvCmd : rlvObjectEntry.second.getCommandList())
        {
            std::string strOption; LLUUID idOption;
            if ( (rlvCmd.hasOption()) && (idOption.set(rlvCmd.getOption(), false)) && (idOption.notNull()) )
            {
                LLAvatarName avName;
                if (gObjectList.findObject(idOption))
                {
                    strOption = rlvGetItemNameFromObjID(idOption, true);
                }
                else if (LLAvatarNameCache::get(idOption, &avName))
                {
                    strOption = (!avName.getAccountName().empty()) ? avName.getAccountName() : avName.getDisplayName();
                }
                else if (!gCacheName->getGroupName(idOption, strOption))
                {
                    if (m_PendingLookup.end() == std::find(m_PendingLookup.begin(), m_PendingLookup.end(), idOption))
                    {
                        LLAvatarNameCache::get(idOption, boost::bind(&RlvFloaterBehaviours::onAvatarNameLookup, this, _1, _2));
                        m_PendingLookup.push_back(idOption);
                    }
                    strOption = rlvCmd.getOption();
                }
            }

            if ( (rlvCmd.hasOption()) && (rlvGetShowException(rlvCmd.getBehaviourType())) )
            {
                // List under the "Exception" tab
                sdExceptRow["enabled"] = gRlvHandler.isException(rlvCmd.getBehaviourType(), idOption);
                sdExceptColumns[0]["value"] = rlvCmd.getBehaviour();
                sdExceptColumns[1]["value"] = strOption;
                sdExceptColumns[2]["value"] = strIssuer;
                pExceptList->addElement(sdExceptRow, ADD_BOTTOM);
            }
            else
            {
                // List under the "Restrictions" tab
                sdBhvrColumns[0]["value"] = (strOption.empty()) ? rlvCmd.asString() : rlvCmd.getBehaviour() + ":" + strOption;
                sdBhvrColumns[1]["value"] = strIssuer;
                pBhvrList->addElement(sdBhvrRow, ADD_BOTTOM);
            }
        }
    }

    //
    // List modifiers
    //
    for (int idxModifier = 0; idxModifier < RLV_MODIFIER_COUNT; idxModifier++)
    {
        const RlvBehaviourModifier* pBhvrModifier = RlvBehaviourDictionary::instance().m_BehaviourModifiers[idxModifier];
        if (pBhvrModifier)
        {
            sdModifierRow["enabled"] = (pBhvrModifier->hasValue());
            sdModifierColumns[0]["value"] = pBhvrModifier->getName();

            if (pBhvrModifier->hasValue())
            {
                const RlvBehaviourModifierValue& modValue = pBhvrModifier->getValue();
                if (typeid(float) == modValue.type())
                    sdModifierColumns[1]["value"] = llformat("%f", boost::get<float>(modValue));
                else if (typeid(int) == modValue.type())
                    sdModifierColumns[1]["value"] = llformat("%d", boost::get<int>(modValue));
                else if (typeid(bool) == modValue.type())
                    sdModifierColumns[1]["value"] = (boost::get<bool>(modValue)) ? "true" : "false";
                else if (typeid(LLUUID) == modValue.type())
                    sdModifierColumns[1]["value"] = boost::get<LLUUID>(modValue).asString();
            }
            else
            {
                sdModifierColumns[1]["value"] = "(default)";
            }

            sdModifierColumns[2]["value"] = (pBhvrModifier->getPrimaryObject().notNull()) ? rlvGetItemNameFromObjID(pBhvrModifier->getPrimaryObject()) : LLStringUtil::null;
            pModifierList->addElement(sdModifierRow, ADD_BOTTOM);
        }
    }
}

// ============================================================================
// RlvFloaterLocks member functions
//

// Checked: 2010-03-11 (RLVa-1.2.0)
void RlvFloaterLocks::onOpen(const LLSD& sdKey)
{
    m_ConnRlvCommand = gRlvHandler.setCommandCallback(boost::bind(&RlvFloaterLocks::onRlvCommand, this, _1, _2));

    refreshAll();
}

// Checked: 2010-03-11 (RLVa-1.2.0)
void RlvFloaterLocks::onClose(bool fQuitting)
{
    m_ConnRlvCommand.disconnect();
}

// Checked: 2012-07-14 (RLVa-1.4.7)
bool RlvFloaterLocks::postBuild()
{
    getChild<LLUICtrl>("refresh_btn")->setCommitCallback(boost::bind(&RlvFloaterLocks::refreshAll, this));

    return true;
}

// Checked: 2010-03-11 (RLVa-1.2.0a) | Added: RLVa-1.2.0a
void RlvFloaterLocks::onRlvCommand(const RlvCommand& rlvCmd, ERlvCmdRet eRet)
{
    // Refresh on any successful @XXX=y|n command where XXX is any of the attachment or wearable locking behaviours
    if ( (RLV_RET_SUCCESS == eRet) && ((RLV_TYPE_ADD == rlvCmd.getParamType()) || (RLV_TYPE_REMOVE == rlvCmd.getParamType())) )
    {
        switch (rlvCmd.getBehaviourType())
        {
            case RLV_BHVR_DETACH:
            case RLV_BHVR_ADDATTACH:
            case RLV_BHVR_REMATTACH:
            case RLV_BHVR_ADDOUTFIT:
            case RLV_BHVR_REMOUTFIT:
                refreshAll();
                break;
            default:
                break;
        }
    }
}

// Checked: 2010-03-18 (RLVa-1.2.0)
void RlvFloaterLocks::refreshAll()
{
    LLScrollListCtrl* pLockList = getChild<LLScrollListCtrl>("lock_list");
    pLockList->operateOnAll(LLCtrlListInterface::OP_DELETE);

    if (!isAgentAvatarValid())
        return;

    //
    // Set-up a row we can just reuse
    //
    LLSD sdRow;
    LLSD& sdColumns = sdRow["columns"];
    sdColumns[0]["column"] = "lock_type";   sdColumns[0]["type"] = "text";
    sdColumns[1]["column"] = "lock_addrem"; sdColumns[1]["type"] = "text";
    sdColumns[2]["column"] = "lock_target"; sdColumns[2]["type"] = "text";
    sdColumns[3]["column"] = "lock_origin"; sdColumns[3]["type"] = "text";

    //
    // List attachment locks
    //
    sdColumns[0]["value"] = "Attachment";
    sdColumns[1]["value"] = "rem";

    const RlvAttachmentLocks::rlv_attachobjlock_map_t& attachObjRem = gRlvAttachmentLocks.getAttachObjLocks();
    for (RlvAttachmentLocks::rlv_attachobjlock_map_t::const_iterator itAttachObj = attachObjRem.begin();
            itAttachObj != attachObjRem.end(); ++itAttachObj)
    {
        sdColumns[2]["value"] = rlvGetItemNameFromObjID(itAttachObj->first);
        sdColumns[3]["value"] = rlvGetItemNameFromObjID(itAttachObj->second);

        pLockList->addElement(sdRow, ADD_BOTTOM);
    }

    //
    // List attachment point locks
    //
    sdColumns[0]["value"] = "Attachment Point";

    sdColumns[1]["value"] = "add";
    const RlvAttachmentLocks::rlv_attachptlock_map_t& attachPtAdd = gRlvAttachmentLocks.getAttachPtLocks(RLV_LOCK_ADD);
    for (RlvAttachmentLocks::rlv_attachptlock_map_t::const_iterator itAttachPt = attachPtAdd.begin();
            itAttachPt != attachPtAdd.end(); ++itAttachPt)
    {
        const LLViewerJointAttachment* pAttachPt =
            get_if_there(gAgentAvatarp->mAttachmentPoints, itAttachPt->first, (LLViewerJointAttachment*)NULL);
        sdColumns[2]["value"] = pAttachPt->getName();
        sdColumns[3]["value"] = rlvGetItemNameFromObjID(itAttachPt->second);

        pLockList->addElement(sdRow, ADD_BOTTOM);
    }

    sdColumns[1]["value"] = "rem";
    const RlvAttachmentLocks::rlv_attachptlock_map_t& attachPtRem = gRlvAttachmentLocks.getAttachPtLocks(RLV_LOCK_REMOVE);
    for (RlvAttachmentLocks::rlv_attachptlock_map_t::const_iterator itAttachPt = attachPtRem.begin();
            itAttachPt != attachPtRem.end(); ++itAttachPt)
    {
        const LLViewerJointAttachment* pAttachPt =
            get_if_there(gAgentAvatarp->mAttachmentPoints, itAttachPt->first, (LLViewerJointAttachment*)NULL);
        sdColumns[2]["value"] = pAttachPt->getName();
        sdColumns[3]["value"] = rlvGetItemNameFromObjID(itAttachPt->second);

        pLockList->addElement(sdRow, ADD_BOTTOM);
    }

    //
    // List wearable type locks
    //
    sdColumns[0]["value"] = "Wearable Type";

    sdColumns[1]["value"] = "add";
    const RlvWearableLocks::rlv_wearabletypelock_map_t& wearableTypeAdd = gRlvWearableLocks.getWearableTypeLocks(RLV_LOCK_ADD);
    for (RlvWearableLocks::rlv_wearabletypelock_map_t::const_iterator itWearableType = wearableTypeAdd.begin();
            itWearableType != wearableTypeAdd.end(); ++itWearableType)
    {
        sdColumns[2]["value"] = LLWearableType::getInstance()->getTypeLabel(itWearableType->first);
        sdColumns[3]["value"] = rlvGetItemNameFromObjID(itWearableType->second);

        pLockList->addElement(sdRow, ADD_BOTTOM);
    }

    sdColumns[1]["value"] = "rem";
    const RlvWearableLocks::rlv_wearabletypelock_map_t& wearableTypeRem = gRlvWearableLocks.getWearableTypeLocks(RLV_LOCK_REMOVE);
    for (RlvWearableLocks::rlv_wearabletypelock_map_t::const_iterator itWearableType = wearableTypeRem.begin();
            itWearableType != wearableTypeRem.end(); ++itWearableType)
    {
        sdColumns[2]["value"] = LLWearableType::getInstance()->getTypeName(itWearableType->first);
        sdColumns[3]["value"] = rlvGetItemNameFromObjID(itWearableType->second);

        pLockList->addElement(sdRow, ADD_BOTTOM);
    }

    //
    // List "nostrip" (soft) locks
    //
    sdColumns[1]["value"] = "nostrip";
    sdColumns[3]["value"] = "(Agent)";

    LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
    LLFindWearablesEx f(true, true);
    gInventory.collectDescendentsIf(LLAppearanceMgr::instance().getCOF(), folders, items, false, f);

    for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
    {
        const LLViewerInventoryItem* pItem = *itItem;
        if (!RlvForceWear::instance().isStrippable(pItem->getUUID()))
        {
            sdColumns[0]["value"] = rlvGetItemType(pItem);
            sdColumns[2]["value"] = rlvGetItemName(pItem);

            pLockList->addElement(sdRow, ADD_BOTTOM);
        }
    }

    //
    // List folder locks
    //
    {
        // Folder lock descriptors
        const RlvFolderLocks::folderlock_list_t& folderLocks = RlvFolderLocks::instance().getFolderLocks();
        for (RlvFolderLocks::folderlock_list_t::const_iterator itFolderLock = folderLocks.begin();
                itFolderLock != folderLocks.end(); ++itFolderLock)
        {
            const RlvFolderLocks::folderlock_descr_t* pLockDescr = *itFolderLock;
            if (pLockDescr)
            {
                sdColumns[0]["value"] = "Folder Descriptor";
                sdColumns[1]["value"] =
                    rlvLockMaskToString(pLockDescr->eLockType) + "/" +
                    rlvFolderLockPermissionToString(pLockDescr->eLockPermission) + "/" +
                    rlvFolderLockScopeToString(pLockDescr->eLockScope);
                sdColumns[2]["value"] = rlvFolderLockSourceToTarget(pLockDescr->lockSource);
                sdColumns[3]["value"] = rlvGetItemNameFromObjID(pLockDescr->idRlvObj);

                pLockList->addElement(sdRow, ADD_BOTTOM);
            }
        }
    }

    {
        // Folder locked attachments and wearables
        uuid_vec_t idItems;
        const uuid_vec_t& folderLockAttachmentsIds = RlvFolderLocks::instance().getAttachmentLookups();
        idItems.insert(idItems.end(), folderLockAttachmentsIds.begin(), folderLockAttachmentsIds.end());
        const uuid_vec_t& folderLockWearabels = RlvFolderLocks::instance().getWearableLookups();
        idItems.insert(idItems.end(), folderLockWearabels.begin(), folderLockWearabels.end());

        for (uuid_vec_t::const_iterator itItemId = idItems.begin(); itItemId != idItems.end(); ++itItemId)
        {
            const LLViewerInventoryItem* pItem = gInventory.getItem(*itItemId);
            if (pItem)
            {
                sdColumns[0]["value"] = rlvGetItemType(pItem);
                sdColumns[1]["value"] = rlvLockMaskToString(RLV_LOCK_REMOVE);
                sdColumns[2]["value"] = rlvGetItemName(pItem);
                sdColumns[3]["value"] = "<Folder Lock>";

                pLockList->addElement(sdRow, ADD_BOTTOM);
            }
        }
    }
}

// ============================================================================
// RlvFloaterStrings member functions
//

// Checked: 2011-11-08 (RLVa-1.5.0)
RlvFloaterStrings::RlvFloaterStrings(const LLSD& sdKey)
    : LLFloater(sdKey)
    , m_fDirty(false)
    , m_pStringList(NULL)
{
}

// Checked: 2011-11-08 (RLVa-1.5.0)
bool RlvFloaterStrings::postBuild()
{
    // Set up the UI controls
    m_pStringList = findChild<LLComboBox>("string_list");
    m_pStringList->setCommitCallback(boost::bind(&RlvFloaterStrings::checkDirty, this, true));

    LLUICtrl* pDefaultBtn = findChild<LLUICtrl>("default_btn");
    pDefaultBtn->setCommitCallback(boost::bind(&RlvFloaterStrings::onStringRevertDefault, this));

    // Read all string metadata from the default strings file
    llifstream fileStream(RlvStrings::getStringMapPath().c_str(), std::ios::binary); LLSD sdFileData;
    if ( (fileStream.is_open()) && (LLSDSerialize::fromXMLDocument(sdFileData, fileStream)) )
    {
        m_sdStringsInfo = sdFileData["strings"];
        fileStream.close();
    }

    // Populate the combo box
    for (LLSD::map_const_iterator itString = m_sdStringsInfo.beginMap(); itString != m_sdStringsInfo.endMap(); ++itString)
    {
        const LLSD& sdStringInfo = itString->second;
        if ( (!sdStringInfo.has("customizable")) || (!sdStringInfo["customizable"].asBoolean()) )
            continue;
        m_pStringList->add( (sdStringInfo.has("label")) ? sdStringInfo["label"].asString() : itString->first, itString->first);
    }

    refresh();

    return true;
}

// Checked: 2011-11-08 (RLVa-1.5.0)
void RlvFloaterStrings::onClose(bool fQuitting)
{
    checkDirty(false);
    if (m_fDirty)
    {
        // Save the custom string overrides
        RlvStrings::saveToFile(gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, RLV_STRINGS_FILE));

        // Remind the user their changes require a relog to take effect
        LLNotificationsUtil::add("RLVaChangeStrings");
    }
}

// Checked: 2011-11-08 (RLVa-1.5.0)
void RlvFloaterStrings::onStringRevertDefault()
{
    if (!m_strStringCurrent.empty())
    {
        RlvStrings::setCustomString(m_strStringCurrent, LLStringUtil::null);
        m_fDirty = true;
    }
    refresh();
}

// Checked: 2011-11-08 (RLVa-1.5.0)
void RlvFloaterStrings::checkDirty(bool fRefresh)
{
    LLTextEditor* pStringValue = findChild<LLTextEditor>("string_value");
    if (!pStringValue->isPristine())
    {
        RlvStrings::setCustomString(m_strStringCurrent, pStringValue->getText());
        m_fDirty = true;
    }

    if (fRefresh)
    {
        refresh();
    }
}

// Checked: 2011-11-08 (RLVa-1.5.0)
void RlvFloaterStrings::refresh()
{
    m_strStringCurrent = (-1 != m_pStringList->getCurrentIndex()) ? m_pStringList->getSelectedValue().asString() : LLStringUtil::null;

    LLTextEditor* pStringDescr = findChild<LLTextEditor>("string_descr");
    pStringDescr->clear();
    LLTextEditor* pStringValue = findChild<LLTextEditor>("string_value");
    pStringValue->setEnabled(!m_strStringCurrent.empty());
    pStringValue->clear();
    if (!m_strStringCurrent.empty())
    {
        if (m_sdStringsInfo[m_strStringCurrent].has("description"))
            pStringDescr->setText(m_sdStringsInfo[m_strStringCurrent]["description"].asString());
        pStringValue->setText(RlvStrings::getString(m_strStringCurrent));
        pStringValue->makePristine();
    }

    findChild<LLUICtrl>("default_btn")->setEnabled(!m_strStringCurrent.empty());
}

// ============================================================================
// RlvFloaterConsole
//

static const char s_strRlvConsolePrompt[] = "> ";
static const char s_strRlvConsoleDisabled[] = "RLVa is disabled";
static const char s_strRlvConsoleInvalid[] = "Invalid command";

RlvFloaterConsole::RlvFloaterConsole(const LLSD& sdKey)
    : LLFloater(sdKey)
{
}

RlvFloaterConsole::~RlvFloaterConsole()
{
}

bool RlvFloaterConsole::postBuild()
{
    m_pInputEdit = getChild<LLChatEntry>("console_input");
    m_pInputEdit->setCommitCallback(boost::bind(&RlvFloaterConsole::onInput, this, _1, _2));
    m_pInputEdit->setTextExpandedCallback(boost::bind(&RlvFloaterConsole::reshapeLayoutPanel, this));
    m_pInputEdit->setFocus(true);
    m_pInputEdit->setCommitOnFocusLost(false);

    m_pInputPanel = getChild<LLLayoutPanel>("input_panel");
    m_nInputEditPad = m_pInputPanel->getRect().getHeight() - m_pInputEdit->getRect().getHeight();

    m_pOutputText = getChild<LLTextEditor>("console_output");
    m_pOutputText->appendText(s_strRlvConsolePrompt, false);

    return true;
}

void RlvFloaterConsole::onClose(bool fQuitting)
{
    RlvBehaviourDictionary::instance().clearModifiers(gAgent.getID());
    gRlvHandler.processCommand(gAgent.getID(), "clear", true);
}

void RlvFloaterConsole::addCommandReply(const std::string& strCommand, const std::string& strReply)
{
    m_pOutputText->appendText(llformat("%s: ", strCommand.c_str()), true);
    m_pOutputText->appendText(strReply, false);
}

void RlvFloaterConsole::onInput(LLUICtrl* pCtrl, const LLSD& sdParam)
{
    LLChatEntry* pInputEdit = static_cast<LLChatEntry*>(pCtrl);
    std::string strInput = pInputEdit->getText();
    LLStringUtil::trim(strInput);

    m_pOutputText->appendText(strInput, false);
    pInputEdit->setText(LLStringUtil::null);

    if (!rlv_handler_t::isEnabled())
    {
        m_pOutputText->appendText(s_strRlvConsoleDisabled, true);
    }
    else if ( (strInput.length() <= 3) || (RLV_CMD_PREFIX != strInput[0]) )
    {
        m_pOutputText->appendText(s_strRlvConsoleInvalid, true);
    }
    else
    {
        strInput.erase(0, 1);
        LLStringUtil::toLower(strInput);

        std::string strExecuted, strFailed, strRetained, *pstr;

        boost_tokenizer tokens(strInput, boost::char_separator<char>(",", "", boost::drop_empty_tokens));
        for (std::string strCmd : tokens)
        {
            ERlvCmdRet eRet = gRlvHandler.processCommand(gAgent.getID(), strCmd, true);
            if ( RLV_RET_SUCCESS == (eRet & RLV_RET_SUCCESS) )
                pstr = &strExecuted;
            else if ( RLV_RET_FAILED == (eRet & RLV_RET_FAILED) )
                pstr = &strFailed;
            else if (RLV_RET_RETAINED == eRet)
                pstr = &strRetained;
            else
            {
                RLV_ASSERT(false);
                pstr = &strFailed;
            }

            if (const char* pstrSuffix = RlvStrings::getStringFromReturnCode(eRet))
                strCmd.append(" (").append(pstrSuffix).append(")");
            else if (RLV_RET_SUCCESS == (eRet & RLV_RET_SUCCESS))
                strCmd.clear(); // Only show feedback on successful commands when there's an informational notice

            if (!pstr->empty())
                pstr->append(", ");
            pstr->append(strCmd);
        }

        if (!strExecuted.empty())
            m_pOutputText->appendText("INFO: @" + strExecuted, true);
        if (!strFailed.empty())
            m_pOutputText->appendText("ERR: @" + strFailed, true);
        if (!strRetained.empty())
            m_pOutputText->appendText("RET: @" + strRetained, true);

        if (RlvForceWear::instanceExists())
            RlvForceWear::instance().done();
    }

    m_pOutputText->appendText(s_strRlvConsolePrompt, true);
}

void RlvFloaterConsole::reshapeLayoutPanel()
{
    m_pInputPanel->reshape(m_pInputPanel->getRect().getWidth(), m_pInputEdit->getRect().getHeight() + m_nInputEditPad, false);
}

// ============================================================================
