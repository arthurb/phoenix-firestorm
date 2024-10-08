/**
 * @file llpreviewnotecard.cpp
 * @brief Implementation of the notecard editor
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

#include "llpreviewnotecard.h"

#include "llinventory.h"

#include "llagent.h"
#include "lldraghandle.h"
#include "llexternaleditor.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "llinventorydefines.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llmd5.h"
#include "llnotificationsutil.h"
#include "llmd5.h"
#include "llresmgr.h"
#include "roles_constants.h"
#include "llscrollbar.h"
#include "llselectmgr.h"
#include "lltrans.h"
#include "llviewertexteditor.h"
#include "llfilesystem.h"
#include "llviewerinventory.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "lldir.h"
#include "llviewerstats.h"
#include "llviewercontrol.h"        // gSavedSettings
#include "llappviewer.h"        // app_abort_quit()
#include "lllineeditor.h"
#include "lluictrlfactory.h"
#include "llviewerassetupload.h"

// [SL:KB] - Patch: UI-FloaterSearchReplace | Checked: 2010-11-05 (Catznip-2.3.0a) | Added: Catznip-2.3.0a
#include "llfloatersearchreplace.h"
// [/SL:KB]
#include "llautoreplace.h"
#include "llscrollcontainer.h"

///----------------------------------------------------------------------------
/// Class LLPreviewNotecard
///----------------------------------------------------------------------------

// Default constructor
LLPreviewNotecard::LLPreviewNotecard(const LLSD& key) //const LLUUID& item_id,
    : LLPreview( key ),
    mLiveFile(NULL)
    // <FS:Ansariel> FIRE-24306: Retain cursor position when saving notecards
    ,mCursorPos(0)
    ,mScrollPos(0)
    // <FS:Ansariel> FIRE-29425: User-selectable font and size for notecards
    ,mFontNameChangedCallbackConnection()
    ,mFontSizeChangedCallbackConnection()
    // </FS:Ansariel>
{
    const LLInventoryItem *item = getItem();
    if (item)
    {
        mAssetID = item->getAssetUUID();
    }
}

LLPreviewNotecard::~LLPreviewNotecard()
{
    delete mLiveFile;

    // <FS:Ansariel> FIRE-29425: User-selectable font and size for notecards
    if (mFontNameChangedCallbackConnection.connected())
    {
        mFontNameChangedCallbackConnection.disconnect();
    }
    if (mFontSizeChangedCallbackConnection.connected())
    {
        mFontSizeChangedCallbackConnection.disconnect();
    }
    // </FS:Ansariel>
}

bool LLPreviewNotecard::postBuild()
{
    mEditor = getChild<LLViewerTextEditor>("Notecard Editor");
    mEditor->setAutoreplaceCallback(boost::bind(&LLAutoReplace::autoreplaceCallback, LLAutoReplace::getInstance(), _1, _2, _3, _4, _5)); // <FS:Ansariel> FIRE-22810: Add autoreplace to notecards
    mEditor->setNotecardInfo(mItemUUID, mObjectID, getKey());
    mEditor->makePristine();

    childSetAction("Save", onClickSave, this);
    getChildView("lock")->setVisible( false);

    childSetAction("Delete", onClickDelete, this);
    getChildView("Delete")->setEnabled(false);

    childSetAction("Edit", onClickEdit, this);

    // <FS:Ansariel> FIRE-13969: Search button
    getChild<LLButton>("Search")->setClickedCallback(boost::bind(&LLPreviewNotecard::onSearchButtonClicked, this));

    const LLInventoryItem* item = getItem();

    childSetCommitCallback("desc", LLPreview::onText, this);
    if (item)
    {
        getChild<LLUICtrl>("desc")->setValue(item->getDescription());
        bool source_library = mObjectUUID.isNull() && gInventory.isObjectDescendentOf(item->getUUID(), gInventory.getLibraryRootFolderID());
        getChildView("Delete")->setEnabled(!source_library);
    }
    getChild<LLLineEditor>("desc")->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);

    // <FS:Ansariel> FIRE-29425: User-selectable font and size for notecards
    mFontNameChangedCallbackConnection = gSavedSettings.getControl("FSNotecardFontName")->getSignal()->connect(boost::bind(&LLPreviewNotecard::onFontChanged, this));
    mFontSizeChangedCallbackConnection = gSavedSettings.getControl("FSNotecardFontSize")->getSignal()->connect(boost::bind(&LLPreviewNotecard::onFontChanged, this));
    onFontChanged();
    // </FS:Ansariel>

    return LLPreview::postBuild();
}

bool LLPreviewNotecard::saveItem()
{
    LLInventoryItem* item = gInventory.getItem(mItemUUID);
    return saveIfNeeded(item);
}

void LLPreviewNotecard::setEnabled(bool enabled)
{
    LLViewerTextEditor* editor = findChild<LLViewerTextEditor>("Notecard Editor");
    // editor is part of xml, if it doesn't exists, nothing else does
    if (editor)
    {
        editor->setEnabled(enabled);
        getChildView("lock")->setVisible( !enabled);
        getChildView("desc")->setEnabled(enabled);
        getChildView("Save")->setEnabled(enabled && (!editor->isPristine()));
    }
}


void LLPreviewNotecard::draw()
{
    LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");
    bool changed = !editor->isPristine();

    getChildView("Save")->setEnabled(changed && getEnabled());

    LLPreview::draw();
}

// [SL:KB] - Patch: UI-FloaterSearchReplace | Checked: 2010-11-05 (Catznip-2.3.0a) | Added: Catznip-2.3.0a
bool LLPreviewNotecard::hasAccelerators() const
{
    return true;
}
// [/SL:KB]

// virtual
bool LLPreviewNotecard::handleKeyHere(KEY key, MASK mask)
{
    if(('S' == key) && (MASK_CONTROL == (mask & MASK_CONTROL)))
    {
        saveIfNeeded();
        return true;
    }

// [SL:KB] - Patch: UI-FloaterSearchReplace | Checked: 2010-11-05 (Catznip-2.3.0a) | Added: Catznip-2.3.0a
    if(('F' == key) && (MASK_CONTROL == (mask & MASK_CONTROL)))
    {
        LLFloaterSearchReplace::show(getEditor());
        return true;
    }
// [/SL:KB]

    return LLPreview::handleKeyHere(key, mask);
}

// virtual
bool LLPreviewNotecard::canClose()
{
    LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

    if(mForceClose || editor->isPristine())
    {
        return true;
    }
    else
    {
        if(!mSaveDialogShown)
        {
            mSaveDialogShown = true;
            // Bring up view-modal dialog: Save changes? Yes, No, Cancel
            LLNotificationsUtil::add("SaveChanges", LLSD(), LLSD(), boost::bind(&LLPreviewNotecard::handleSaveChangesDialog,this, _1, _2));
        }
        return false;
    }
}

/* virtual */
void LLPreviewNotecard::setObjectID(const LLUUID& object_id)
{
    LLPreview::setObjectID(object_id);

    LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");
    editor->setNotecardObjectID(mObjectUUID);
    editor->makePristine();
}

const LLInventoryItem* LLPreviewNotecard::getDragItem()
{
    LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

    if(editor)
    {
        return editor->getDragItem();
    }
    return NULL;
}

// [SL:KB] - Patch: UI-FloaterSearchReplace | Checked: 2010-11-05 (Catznip-2.3.0a) | Added: Catznip-2.3.0a
LLTextEditor* LLPreviewNotecard::getEditor()
{
    return getChild<LLViewerTextEditor>("Notecard Editor");
}
// [/SL:KB]

bool LLPreviewNotecard::hasEmbeddedInventory()
{
    LLViewerTextEditor* editor = NULL;
    editor = getChild<LLViewerTextEditor>("Notecard Editor");
    if (!editor) return false;
    return editor->hasEmbeddedInventory();
}

void LLPreviewNotecard::refreshFromInventory(const LLUUID& new_item_id)
{
    if (new_item_id.notNull())
    {
        mItemUUID = new_item_id;
        setKey(LLSD(new_item_id));
    }
    LL_DEBUGS() << "LLPreviewNotecard::refreshFromInventory()" << LL_ENDL;
    loadAsset();
}

void LLPreviewNotecard::updateTitleButtons()
{
    LLPreview::updateTitleButtons();

    // <FS:Ansariel> Fix XUI parser warning
    //LLUICtrl* lock_btn = getChild<LLUICtrl>("lock");
    //if (lock_btn->getVisible() && !isMinimized()) // lock button stays visible if floater is minimized.
    LLUICtrl* lock_btn = findChild<LLUICtrl>("lock");
    if(lock_btn && lock_btn->getVisible() && !isMinimized()) // lock button stays visible if floater is minimized.
    // </FS:Ansariel>
    {
        LLRect lock_rc = lock_btn->getRect();
        LLRect buttons_rect = getDragHandle()->getButtonsRect();
        buttons_rect.mLeft = lock_rc.mLeft;
        getDragHandle()->setButtonsRect(buttons_rect);
    }
}

void LLPreviewNotecard::loadAsset()
{
    // request the asset.
    const LLInventoryItem* item = getItem();
    LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

    if (!editor)
        return;

    bool fail = false;

    if(item)
    {
        LLPermissions perm(item->getPermissions());
        bool is_owner = gAgent.allowOperation(PERM_OWNER, perm, GP_OBJECT_MANIPULATE);
        bool allow_copy = gAgent.allowOperation(PERM_COPY, perm, GP_OBJECT_MANIPULATE);
        bool allow_modify = canModify(mObjectUUID, item);
        bool source_library = mObjectUUID.isNull() && gInventory.isObjectDescendentOf(mItemUUID, gInventory.getLibraryRootFolderID());

        if (allow_copy || gAgent.isGodlike())
        {
            mAssetID = item->getAssetUUID();
            if(mAssetID.isNull())
            {
                editor->setText(LLStringUtil::null);
                editor->makePristine();
                editor->setEnabled(true);
                mAssetStatus = PREVIEW_ASSET_LOADED;
            }
            else
            {
                LLHost source_sim = LLHost();
                LLSD* user_data = nullptr;
                if (mObjectUUID.notNull())
                {
                    LLViewerObject *objectp = gObjectList.findObject(mObjectUUID);
                    if (objectp && objectp->getRegion())
                    {
                        source_sim = objectp->getRegion()->getHost();
                    }
                    else
                    {
                        // The object that we're trying to look at disappeared, bail.
                        LL_WARNS() << "Can't find object " << mObjectUUID << " associated with notecard." << LL_ENDL;
                        mAssetID.setNull();
                        editor->setText(getString("no_object"));
                        editor->makePristine();
                        editor->setEnabled(false);
                        mAssetStatus = PREVIEW_ASSET_LOADED;
                        return;
                    }
                    user_data = new LLSD();
                    user_data->with("taskid", mObjectUUID).with("itemid", mItemUUID);
                }
                else
                {
                    user_data =  new LLSD(mItemUUID);
                }

                gAssetStorage->getInvItemAsset(source_sim,
                                                gAgent.getID(),
                                                gAgent.getSessionID(),
                                                item->getPermissions().getOwner(),
                                                mObjectUUID,
                                                item->getUUID(),
                                                item->getAssetUUID(),
                                                item->getType(),
                                                &onLoadComplete,
                                                (void*)user_data,
                                                true);
                mAssetStatus = PREVIEW_ASSET_LOADING;
            }
        }
        else
        {
            mAssetID.setNull();
            editor->setText(getString("not_allowed"));
            editor->makePristine();
            editor->setEnabled(false);
            mAssetStatus = PREVIEW_ASSET_LOADED;
        }

        if(!allow_modify)
        {
            editor->setEnabled(false);
            getChildView("lock")->setVisible( true);
            getChildView("Edit")->setEnabled(false);
        }

        if((allow_modify || is_owner) && !source_library)
        {
            getChildView("Delete")->setEnabled(true);
        }
    }
    else if (mObjectUUID.notNull() && mItemUUID.notNull())
    {
        LLViewerObject* objectp = gObjectList.findObject(mObjectUUID);
        if (objectp && (objectp->isInventoryPending() || objectp->isInventoryDirty()))
        {
            // It's a notecard in object's inventory and we failed to get it because inventory is not up to date.
            // Subscribe for callback and retry at inventoryChanged()
            registerVOInventoryListener(objectp, NULL); //removes previous listener

            if (objectp->isInventoryDirty())
            {
                objectp->requestInventory();
            }
        }
        else
        {
            fail = true;
        }
    }
    else
    {
        fail = true;
    }

    if (fail)
    {
        editor->setText(LLStringUtil::null);
        editor->makePristine();
        editor->setEnabled(true);
        // Don't set asset status here; we may not have set the item id yet
        // (e.g. when this gets called initially)
        //mAssetStatus = PREVIEW_ASSET_LOADED;
    }
}

// static
void LLPreviewNotecard::onLoadComplete(const LLUUID& asset_uuid,
                                       LLAssetType::EType type,
                                       void* user_data, S32 status, LLExtStat ext_status)
{
    LL_INFOS() << "LLPreviewNotecard::onLoadComplete()" << LL_ENDL;
    LLSD* floater_key = (LLSD*)user_data;
    LLPreviewNotecard* preview = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", *floater_key);
    if( preview )
    {
        if(0 == status)
        {
            LLFileSystem file(asset_uuid, type, LLFileSystem::READ);

            S32 file_length = file.getSize();

            std::vector<char> buffer(file_length+1);
            file.read((U8*)&buffer[0], file_length);

            // put a EOS at the end
            buffer[file_length] = 0;


            LLViewerTextEditor* previewEditor = preview->getChild<LLViewerTextEditor>("Notecard Editor");

            if( (file_length > 19) && !strncmp( &buffer[0], "Linden text version", 19 ) )
            {
                if( !previewEditor->importBuffer( &buffer[0], file_length+1 ) )
                {
                    LL_WARNS() << "Problem importing notecard" << LL_ENDL;
                }
            }
            else
            {
                // Version 0 (just text, doesn't include version number)
                previewEditor->setText(LLStringExplicit(&buffer[0]));
            }

            previewEditor->makePristine();
            bool modifiable = preview->canModify(preview->mObjectID, preview->getItem());
            // <FS:Ansariel> Force spell checker to check again after saving a NC,
            //               or misspelled words wouldn't be shown
            previewEditor->onSpellCheckSettingsChange();

            preview->setEnabled(modifiable);
            preview->syncExternal();
            preview->mAssetStatus = PREVIEW_ASSET_LOADED;

            // <FS:Ansariel> FIRE-24306: Retain cursor position when saving notecards
            preview->mEditor->setCursorPos(preview->mCursorPos);
            preview->mEditor->getScrollContainer()->getScrollbar(LLScrollContainer::VERTICAL)->setDocPos(preview->mScrollPos);
        }
        else
        {
            if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
                LL_ERR_FILE_EMPTY == status)
            {
                LLNotificationsUtil::add("NotecardMissing");
            }
            else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
            {
                LLNotificationsUtil::add("NotecardNoPermissions");
            }
            else
            {
                LLNotificationsUtil::add("UnableToLoadNotecard");
            }

            LL_WARNS() << "Problem loading notecard: " << status << LL_ENDL;
            preview->mAssetStatus = PREVIEW_ASSET_ERROR;
        }
    }
    delete floater_key;
}

// static
void LLPreviewNotecard::onClickSave(void* user_data)
{
    //LL_INFOS() << "LLPreviewNotecard::onBtnSave()" << LL_ENDL;
    LLPreviewNotecard* preview = (LLPreviewNotecard*)user_data;
    if(preview)
    {
        preview->saveIfNeeded();
    }
}


// static
void LLPreviewNotecard::onClickDelete(void* user_data)
{
    LLPreviewNotecard* preview = (LLPreviewNotecard*)user_data;
    if(preview)
    {
        preview->deleteNotecard();
    }
}

// static
void LLPreviewNotecard::onClickEdit(void* user_data)
{
    LLPreviewNotecard* preview = (LLPreviewNotecard*)user_data;
    if (preview)
    {
        preview->openInExternalEditor();
    }
}

struct LLSaveNotecardInfo
{
    LLPreviewNotecard* mSelf;
    LLUUID mItemUUID;
    LLUUID mObjectUUID;
    LLTransactionID mTransactionID;
    LLPointer<LLInventoryItem> mCopyItem;
    LLSaveNotecardInfo(LLPreviewNotecard* self, const LLUUID& item_id, const LLUUID& object_id,
                       const LLTransactionID& transaction_id, LLInventoryItem* copyitem) :
        mSelf(self), mItemUUID(item_id), mObjectUUID(object_id), mTransactionID(transaction_id), mCopyItem(copyitem)
    {
    }
};

void LLPreviewNotecard::finishInventoryUpload(LLUUID itemId, LLUUID newAssetId, LLUUID newItemId)
{
    // Update the UI with the new asset.
    LLPreviewNotecard* nc = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", LLSD(itemId));
    if (nc)
    {
        // *HACK: we have to delete the asset in the cache so
        // that the viewer will redownload it. This is only
        // really necessary if the asset had to be modified by
        // the uploader, so this can be optimized away in some
        // cases. A better design is to have a new uuid if the
        // script actually changed the asset.
        if (nc->hasEmbeddedInventory())
        {
            LLFileSystem::removeFile(newAssetId, LLAssetType::AT_NOTECARD);
        }
        if (newItemId.isNull())
        {
            nc->setAssetId(newAssetId);
            nc->refreshFromInventory();
        }
        else
        {
            nc->refreshFromInventory(newItemId);
        }
        // <FS:Ansariel> FIRE-9039: Close notecard after choosing "Save" in close confirmation
        nc->checkCloseAfterSave();
    }
}

void LLPreviewNotecard::finishTaskUpload(LLUUID itemId, LLUUID newAssetId, LLUUID taskId)
{

    LLSD floater_key;
    floater_key["taskid"] = taskId;
    floater_key["itemid"] = itemId;
    LLPreviewNotecard* nc = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", floater_key);
    if (nc)
    {
        if (nc->hasEmbeddedInventory())
        {
            LLFileSystem::removeFile(newAssetId, LLAssetType::AT_NOTECARD);
        }
        nc->setAssetId(newAssetId);
        nc->refreshFromInventory();
        // <FS:Ansariel> FIRE-9039: Close notecard after choosing "Save" in close confirmation
        nc->checkCloseAfterSave();
    }
}

bool LLPreviewNotecard::saveIfNeeded(LLInventoryItem* copyitem, bool sync)
{
    LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

    if(!editor)
    {
        LL_WARNS() << "Cannot get handle to the notecard editor." << LL_ENDL;
        return false;
    }

    if(!editor->isPristine())
    {
        std::string buffer;
        if (!editor->exportBuffer(buffer))
        {
            return false;
        }

        // <FS:Ansariel> FIRE-24306: Retain cursor position when saving notecards
        mCursorPos = editor->getCursorPos();
        mScrollPos = editor->getScrollContainer()->getScrollbar(LLScrollContainer::VERTICAL)->getDocPos();

        editor->makePristine();
        const LLInventoryItem* item = getItem();
        // save it out to database
        if (item)
        {
            const LLViewerRegion* region = gAgent.getRegion();
            if (!region)
            {
                LL_WARNS() << "Not connected to a region, cannot save notecard." << LL_ENDL;
                return false;
            }
            std::string agent_url = region->getCapability("UpdateNotecardAgentInventory");
            std::string task_url = region->getCapability("UpdateNotecardTaskInventory");

            if (!agent_url.empty() && !task_url.empty())
            {
                std::string url;
                LLResourceUploadInfo::ptr_t uploadInfo;

                if (mObjectUUID.isNull() && !agent_url.empty())
                {
                    uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(mItemUUID, LLAssetType::AT_NOTECARD, buffer,
                        [](LLUUID itemId, LLUUID newAssetId, LLUUID newItemId, LLSD) {
                            LLPreviewNotecard::finishInventoryUpload(itemId, newAssetId, newItemId);
                        },
                        nullptr);
                    url = agent_url;
                }
                else if (!mObjectUUID.isNull() && !task_url.empty())
                {
                    LLUUID object_uuid(mObjectUUID);
                    uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(mObjectUUID, mItemUUID, LLAssetType::AT_NOTECARD, buffer,
                        [object_uuid](LLUUID itemId, LLUUID, LLUUID newAssetId, LLSD) {
                            LLPreviewNotecard::finishTaskUpload(itemId, newAssetId, object_uuid);
                        },
                        nullptr);
                    url = task_url;
                }

                if (!url.empty() && uploadInfo)
                {
                    mAssetStatus = PREVIEW_ASSET_LOADING;
                    setEnabled(false);

                    LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
                }

            }
            else if (gAssetStorage)
            {
                // We need to update the asset information
                LLTransactionID tid;
                LLAssetID asset_id;
                tid.generate();
                asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

                LLFileSystem file(asset_id, LLAssetType::AT_NOTECARD, LLFileSystem::APPEND);


                LLSaveNotecardInfo* info = new LLSaveNotecardInfo(this, mItemUUID, mObjectUUID,
                                                                tid, copyitem);

                S32 size = static_cast<S32>(buffer.length()) + 1;
                file.write((U8*)buffer.c_str(), size);

                gAssetStorage->storeAssetData(tid, LLAssetType::AT_NOTECARD,
                                                &onSaveComplete,
                                                (void*)info,
                                                false);
                // <FS:Ansariel> FIRE-9039: Close notecard after choosing "Save" in close confirmation
                //return true;
            }
            else // !gAssetStorage
            {
                LL_WARNS() << "Not connected to an asset storage system." << LL_ENDL;
                return false;
            }
            // <FS:Ansariel> FIRE-9039: Close notecard after choosing "Save" in close confirmation
            //if(mCloseAfterSave)
            //{
            //  closeFloater();
            //}
            // </FS:Ansariel>
        }
    }
    return true;
}

void LLPreviewNotecard::syncExternal()
{
    // Sync with external editor.
    std::string tmp_file = getTmpFileName();
    llstat s;
    if (LLFile::stat(tmp_file, &s) == 0) // file exists
    {
        if (mLiveFile) mLiveFile->ignoreNextUpdate();
        writeToFile(tmp_file);
    }
}

/*virtual*/
void LLPreviewNotecard::inventoryChanged(LLViewerObject* object,
    LLInventoryObject::object_list_t* inventory,
    S32 serial_num,
    void* user_data)
{
    removeVOInventoryListener();
    loadAsset();
}


void LLPreviewNotecard::deleteNotecard()
{
    LLNotificationsUtil::add("DeleteNotecard", LLSD(), LLSD(), boost::bind(&LLPreviewNotecard::handleConfirmDeleteDialog,this, _1, _2));
}

// static
void LLPreviewNotecard::onSaveComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
    LLSaveNotecardInfo* info = (LLSaveNotecardInfo*)user_data;
    if(info && (0 == status))
    {
        if(info->mObjectUUID.isNull())
        {
            LLViewerInventoryItem* item;
            item = (LLViewerInventoryItem*)gInventory.getItem(info->mItemUUID);
            if(item)
            {
                LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
                new_item->setAssetUUID(asset_uuid);
                new_item->setTransactionID(info->mTransactionID);
                new_item->updateServer(false);
                gInventory.updateItem(new_item);
                gInventory.notifyObservers();
            }
            else
            {
                LL_WARNS() << "Inventory item for script " << info->mItemUUID
                        << " is no longer in agent inventory." << LL_ENDL;
            }
        }
        else
        {
            LLViewerObject* object = gObjectList.findObject(info->mObjectUUID);
            LLViewerInventoryItem* item = NULL;
            if(object)
            {
                item = (LLViewerInventoryItem*)object->getInventoryObject(info->mItemUUID);
            }
            if(object && item)
            {
                item->setAssetUUID(asset_uuid);
                item->setTransactionID(info->mTransactionID);
                object->updateInventory(item, TASK_INVENTORY_ITEM_KEY, false);
                dialog_refresh_all();
            }
            else
            {
                LLNotificationsUtil::add("SaveNotecardFailObjectNotFound");
            }
        }
        // Perform item copy to inventory
        if (info->mCopyItem.notNull())
        {
            LLViewerTextEditor* editor = info->mSelf->getChild<LLViewerTextEditor>("Notecard Editor");
            if (editor)
            {
                editor->copyInventory(info->mCopyItem);
            }
        }

        // Find our window and close it if requested.

        LLPreviewNotecard* previewp = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", info->mItemUUID);
        if (previewp && previewp->mCloseAfterSave)
        {
            previewp->closeFloater();
        }
    }
    else
    {
        LL_WARNS() << "Problem saving notecard: " << status << LL_ENDL;
        LLSD args;
        args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
        LLNotificationsUtil::add("SaveNotecardFailReason", args);
    }

    std::string uuid_string;
    asset_uuid.toString(uuid_string);
    std::string filename;
    filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_string) + ".tmp";
    LLFile::remove(filename);
    delete info;
}

bool LLPreviewNotecard::handleSaveChangesDialog(const LLSD& notification, const LLSD& response)
{
    mSaveDialogShown = false;
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch(option)
    {
    case 0:  // "Yes"
        mCloseAfterSave = true;
        LLPreviewNotecard::onClickSave((void*)this);
        break;

    case 1:  // "No"
        mForceClose = true;
        closeFloater();
        break;

    case 2: // "Cancel"
    default:
        // If we were quitting, we didn't really mean it.
        LLAppViewer::instance()->abortQuit();
        break;
    }
    return false;
}

bool LLPreviewNotecard::handleConfirmDeleteDialog(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0)
    {
        // canceled
        return false;
    }

    if (mObjectUUID.isNull())
    {
        // move item from agent's inventory into trash
        LLViewerInventoryItem* item = gInventory.getItem(mItemUUID);
        if (item != NULL)
        {
            const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
            gInventory.changeItemParent(item, trash_id, false);
        }
    }
    else
    {
        // delete item from inventory of in-world object
        LLViewerObject* object = gObjectList.findObject(mObjectUUID);
        if(object)
        {
            LLViewerInventoryItem* item = dynamic_cast<LLViewerInventoryItem*>(object->getInventoryObject(mItemUUID));
            if (item != NULL)
            {
                object->removeInventory(mItemUUID);
            }
        }
    }

    // close floater, ignore unsaved changes
    mForceClose = true;
    closeFloater();
    return false;
}

void LLPreviewNotecard::openInExternalEditor()
{
    delete mLiveFile; // deletes file

    // Save the notecard to a temporary file.
    std::string filename = getTmpFileName();
    writeToFile(filename);

    // Start watching file changes.
    mLiveFile = new LLLiveLSLFile(filename, boost::bind(&LLPreviewNotecard::onExternalChange, this, _1));
    mLiveFile->ignoreNextUpdate();
    mLiveFile->addToEventTimer();

    // Open it in external editor.
    {
        LLExternalEditor ed;
        LLExternalEditor::EErrorCode status;
        std::string msg;

        status = ed.setCommand("LL_SCRIPT_EDITOR");
        if (status != LLExternalEditor::EC_SUCCESS)
        {
            if (status == LLExternalEditor::EC_NOT_SPECIFIED) // Use custom message for this error.
            {
                msg = LLTrans::getString("ExternalEditorNotSet");
            }
            else
            {
                msg = LLExternalEditor::getErrorMessage(status);
            }

            LLNotificationsUtil::add("GenericAlert", LLSD().with("MESSAGE", msg));
            return;
        }

        status = ed.run(filename);
        if (status != LLExternalEditor::EC_SUCCESS)
        {
            msg = LLExternalEditor::getErrorMessage(status);
            LLNotificationsUtil::add("GenericAlert", LLSD().with("MESSAGE", msg));
        }
    }
}

bool LLPreviewNotecard::onExternalChange(const std::string& filename)
{
    if (!loadNotecardText(filename))
    {
        return false;
    }

    // Disable sync to avoid recursive load->save->load calls.
    saveIfNeeded(NULL, false);
    return true;
}

bool LLPreviewNotecard::loadNotecardText(const std::string& filename)
{
    if (filename.empty())
    {
        LL_WARNS() << "Empty file name" << LL_ENDL;
        return false;
    }

    LLFILE* file = LLFile::fopen(filename, "rb");       /*Flawfinder: ignore*/
    if (!file)
    {
        LL_WARNS() << "Error opening " << filename << LL_ENDL;
        return false;
    }

    // read in the whole file
    fseek(file, 0L, SEEK_END);
    size_t file_length = (size_t)ftell(file);
    fseek(file, 0L, SEEK_SET);
    char* buffer = new char[file_length + 1];
    size_t nread = fread(buffer, 1, file_length, file);
    if (nread < file_length)
    {
        LL_WARNS() << "Short read" << LL_ENDL;
    }
    buffer[nread] = '\0';
    fclose(file);

    std::string text = std::string(buffer);
    LLStringUtil::replaceTabsWithSpaces(text, LLTextEditor::spacesPerTab());

    mEditor->setText(text);
    delete[] buffer;

    return true;
}

bool LLPreviewNotecard::writeToFile(const std::string& filename)
{
    LLFILE* fp = LLFile::fopen(filename, "wb");
    if (!fp)
    {
        LL_WARNS() << "Unable to write to " << filename << LL_ENDL;
        return false;
    }

    std::string utf8text = mEditor->getText();

    if (utf8text.size() == 0)
    {
        utf8text = " ";
    }

    fputs(utf8text.c_str(), fp);
    fclose(fp);
    return true;
}


std::string LLPreviewNotecard::getTmpFileName()
{
    std::string notecard_id = mObjectID.asString() + "_" + mItemUUID.asString();

    // Use MD5 sum to make the file name shorter and not exceed maximum path length.
    char notecard_id_hash_str[33];             /* Flawfinder: ignore */
    LLMD5 notecard_id_hash((const U8 *)notecard_id.c_str());
    notecard_id_hash.hex_digest(notecard_id_hash_str);

    return std::string(LLFile::tmpdir()) + "sl_notecard_" + notecard_id_hash_str + ".txt";
}

// <FS:Ansariel> FIRE-13969: Search button
void LLPreviewNotecard::onSearchButtonClicked()
{
    LLFloaterSearchReplace::show(getEditor());
}
// </FS:Ansariel>

// <FS:Ansariel> FIRE-9039: Close notecard after choosing "Save" in close confirmation
void LLPreviewNotecard::checkCloseAfterSave()
{
    if (mCloseAfterSave)
    {
        closeFloater();
    }
}
// </FS:Ansariel>

// <FS:Ansariel> FIRE-29425: User-selectable font and size for notecards
void LLPreviewNotecard::onFontChanged()
{
    LLFontGL* font = LLFontGL::getFont(LLFontDescriptor(gSavedSettings.getString("FSNotecardFontName"), gSavedSettings.getString("FSNotecardFontSize"), LLFontGL::NORMAL));
    if (font)
    {
        mEditor->setFont(font);
    }
}
// </FS:Ansariel>

// EOF
