/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#pragma once

#include "structs.h"
#include "dialogs/dialogs_common.h"
#include "ui/effects/send_action_animations.h"
#include "base/observer.h"

void historyInit();

class HistoryItem;
using SelectedItemSet = QMap<int32, HistoryItem*>;

enum NewMessageType {
	NewMessageUnread,
	NewMessageLast,
	NewMessageExisting,
};

class History;
class Histories {
public:
	using Map = QHash<PeerId, History*>;
	Map map;

	Histories() : _a_typings(animation(this, &Histories::step_typings)) {
	}

	void regSendAction(History *history, UserData *user, const MTPSendMessageAction &action, TimeId when);
	void step_typings(TimeMs ms, bool timer);

	History *find(const PeerId &peerId);
	History *findOrInsert(const PeerId &peerId);
	History *findOrInsert(const PeerId &peerId, int32 unreadCount, int32 maxInboxRead, int32 maxOutboxRead);

	void clear();
	void remove(const PeerId &peer);

	HistoryItem *addNewMessage(const MTPMessage &msg, NewMessageType type);

	typedef QMap<History*, TimeMs> TypingHistories; // when typing in this history started
	TypingHistories typing;
	BasicAnimation _a_typings;

	int unreadBadge() const;
	int unreadMutedCount() const {
		return _unreadMuted;
	}
	bool unreadOnlyMuted() const;
	void unreadIncrement(int32 count, bool muted) {
		_unreadFull += count;
		if (muted) {
			_unreadMuted += count;
		}
	}
	void unreadMuteChanged(int32 count, bool muted) {
		if (muted) {
			_unreadMuted += count;
		} else {
			_unreadMuted -= count;
		}
	}

	void setIsPinned(History *history, bool isPinned);
	void clearPinned();
	int pinnedCount() const;
	QList<History*> getPinnedOrder() const;
	void savePinnedToServer() const;

	struct SendActionAnimationUpdate {
		History *history;
		int width;
		int height;
		bool textUpdated;
	};
	base::Observable<SendActionAnimationUpdate> &sendActionAnimationUpdated() {
		return _sendActionAnimationUpdated;
	}

private:
	int _unreadFull = 0;
	int _unreadMuted = 0;
	base::Observable<SendActionAnimationUpdate> _sendActionAnimationUpdated;
	OrderedSet<History*> _pinnedDialogs;

};

class HistoryBlock;

enum HistoryMediaType {
	MediaTypePhoto,
	MediaTypeVideo,
	MediaTypeContact,
	MediaTypeCall,
	MediaTypeFile,
	MediaTypeGif,
	MediaTypeSticker,
	MediaTypeLocation,
	MediaTypeWebPage,
	MediaTypeMusicFile,
	MediaTypeVoiceFile,
	MediaTypeGame,
	MediaTypeInvoice,

	MediaTypeCount
};

enum MediaOverviewType {
	OverviewPhotos          = 0,
	OverviewVideos          = 1,
	OverviewMusicFiles      = 2,
	OverviewFiles           = 3,
	OverviewVoiceFiles      = 4,
	OverviewLinks           = 5,
	OverviewChatPhotos      = 6,
	OverviewRoundVoiceFiles = 7,
	OverviewGIFs            = 8,

	OverviewCount
};

struct TextWithTags {
	struct Tag {
		int offset, length;
		QString id;
	};
	using Tags = QVector<Tag>;

	QString text;
	Tags tags;
};

inline bool operator==(const TextWithTags::Tag &a, const TextWithTags::Tag &b) {
	return (a.offset == b.offset) && (a.length == b.length) && (a.id == b.id);
}
inline bool operator!=(const TextWithTags::Tag &a, const TextWithTags::Tag &b) {
	return !(a == b);
}

inline bool operator==(const TextWithTags &a, const TextWithTags &b) {
	return (a.text == b.text) && (a.tags == b.tags);
}
inline bool operator!=(const TextWithTags &a, const TextWithTags &b) {
	return !(a == b);
}

namespace Data {
struct Draft;
} // namespace Data

class HistoryMedia;
class HistoryMessage;

enum AddToOverviewMethod {
	AddToOverviewNew, // when new message is added to history
	AddToOverviewFront, // when old messages slice was received
	AddToOverviewBack, // when new messages slice was received and it is the last one, we index all media
};

namespace Dialogs {
class Row;
class IndexedList;
} // namespace Dialogs

class ChannelHistory;
class History {
public:
	History(const PeerId &peerId);
	History(const History &) = delete;
	History &operator=(const History &) = delete;

	ChannelId channelId() const {
		return peerToChannel(peer->id);
	}
	bool isChannel() const {
		return peerIsChannel(peer->id);
	}
	bool isMegagroup() const {
		return peer->isMegagroup();
	}
	ChannelHistory *asChannelHistory();
	const ChannelHistory *asChannelHistory() const;

	bool isEmpty() const {
		return blocks.isEmpty();
	}
	bool isDisplayedEmpty() const;

	void clear(bool leaveItems = false);

	virtual ~History();

	HistoryItem *addNewService(MsgId msgId, QDateTime date, const QString &text, MTPDmessage::Flags flags = 0, bool newMsg = true);
	HistoryItem *addNewMessage(const MTPMessage &msg, NewMessageType type);
	HistoryItem *addToHistory(const MTPMessage &msg);
	HistoryItem *addNewForwarded(MsgId id, MTPDmessage::Flags flags, QDateTime date, int32 from, HistoryMessage *item);
	HistoryItem *addNewDocument(MsgId id, MTPDmessage::Flags flags, int32 viaBotId, MsgId replyTo, QDateTime date, int32 from, DocumentData *doc, const QString &caption, const MTPReplyMarkup &markup);
	HistoryItem *addNewPhoto(MsgId id, MTPDmessage::Flags flags, int32 viaBotId, MsgId replyTo, QDateTime date, int32 from, PhotoData *photo, const QString &caption, const MTPReplyMarkup &markup);
	HistoryItem *addNewGame(MsgId id, MTPDmessage::Flags flags, int32 viaBotId, MsgId replyTo, QDateTime date, int32 from, GameData *game, const MTPReplyMarkup &markup);

	void addOlderSlice(const QVector<MTPMessage> &slice);
	void addNewerSlice(const QVector<MTPMessage> &slice);
	bool addToOverview(MediaOverviewType type, MsgId msgId, AddToOverviewMethod method);
	void eraseFromOverview(MediaOverviewType type, MsgId msgId);

	void newItemAdded(HistoryItem *item);

	int countUnread(MsgId upTo);
	void updateShowFrom();
	MsgId inboxRead(MsgId upTo);
	MsgId inboxRead(HistoryItem *wasRead);
	MsgId outboxRead(MsgId upTo);
	MsgId outboxRead(HistoryItem *wasRead);

	HistoryItem *lastAvailableMessage() const;

	int unreadCount() const {
		return _unreadCount;
	}
	void setUnreadCount(int newUnreadCount);
	bool mute() const {
		return _mute;
	}
	void setMute(bool newMute);
	void getNextShowFrom(HistoryBlock *block, int i);
	void addUnreadBar();
	void destroyUnreadBar();
	void clearNotifications();

	bool loadedAtBottom() const; // last message is in the list
	void setNotLoadedAtBottom();
	bool loadedAtTop() const; // nothing was added after loading history back
	bool isReadyFor(MsgId msgId); // has messages for showing history at msgId
	void getReadyFor(MsgId msgId);

	void setLastMessage(HistoryItem *msg);
	void fixLastMessage(bool wasAtBottom);

	bool needUpdateInChatList() const;
	void updateChatListSortPosition();
	void setChatsListDate(const QDateTime &date);
	uint64 sortKeyInChatList() const {
		return _sortKeyInChatList;
	}
	struct PositionInChatListChange {
		int movedFrom;
		int movedTo;
	};
	PositionInChatListChange adjustByPosInChatList(Dialogs::Mode list, Dialogs::IndexedList *indexed);
	bool inChatList(Dialogs::Mode list) const {
		return !chatListLinks(list).isEmpty();
	}
	int posInChatList(Dialogs::Mode list) const;
	Dialogs::Row *addToChatList(Dialogs::Mode list, Dialogs::IndexedList *indexed);
	void removeFromChatList(Dialogs::Mode list, Dialogs::IndexedList *indexed);
	void removeChatListEntryByLetter(Dialogs::Mode list, QChar letter);
	void addChatListEntryByLetter(Dialogs::Mode list, QChar letter, Dialogs::Row *row);
	void updateChatListEntry() const;

	bool isPinnedDialog() const {
		return (_pinnedIndex > 0);
	}
	void setPinnedDialog(bool isPinned);
	void setPinnedIndex(int newPinnedIndex);
	int getPinnedIndex() const {
		return _pinnedIndex;
	}

	MsgId minMsgId() const;
	MsgId maxMsgId() const;
	MsgId msgIdForRead() const;

	int resizeGetHeight(int newWidth);

	void removeNotification(HistoryItem *item) {
		if (!notifies.isEmpty()) {
			for (auto i = notifies.begin(), e = notifies.end(); i != e; ++i) {
				if ((*i) == item) {
					notifies.erase(i);
					break;
				}
			}
		}
	}
	HistoryItem *currentNotification() {
		return notifies.isEmpty() ? 0 : notifies.front();
	}
	bool hasNotification() const {
		return !notifies.isEmpty();
	}
	void skipNotification() {
		if (!notifies.isEmpty()) {
			notifies.pop_front();
		}
	}
	void popNotification(HistoryItem *item) {
		if (!notifies.isEmpty() && notifies.back() == item) notifies.pop_back();
	}

	bool hasPendingResizedItems() const {
		return _flags & Flag::f_has_pending_resized_items;
	}
	void setHasPendingResizedItems();
	void setPendingResize() {
		_flags |= Flag::f_pending_resize;
		setHasPendingResizedItems();
	}

	void paintDialog(Painter &p, int32 w, bool sel) const;
	bool updateSendActionNeedsAnimating(TimeMs ms, bool force = false);
	void unregSendAction(UserData *from);
	bool updateSendActionNeedsAnimating(UserData *user, const MTPSendMessageAction &action);
	bool mySendActionUpdated(SendAction::Type type, bool doing);
	bool paintSendAction(Painter &p, int x, int y, int availableWidth, int outerWidth, style::color color, TimeMs ms);

	void clearLastKeyboard();

	// optimization for userpics displayed on the left
	// if this returns false there is no need to even try to handle them
	bool canHaveFromPhotos() const;

	typedef QList<HistoryBlock*> Blocks;
	Blocks blocks;

	int width = 0;
	int height = 0;
	int32 msgCount = 0;
	MsgId inboxReadBefore = 1;
	MsgId outboxReadBefore = 1;
	HistoryItem *showFrom = nullptr;
	HistoryItem *unreadBar = nullptr;

	PeerData *peer;
	bool oldLoaded = false;
	bool newLoaded = true;
	HistoryItem *lastMsg = nullptr;
	HistoryItem *lastSentMsg = nullptr;
	QDateTime lastMsgDate;

	typedef QList<HistoryItem*> NotifyQueue;
	NotifyQueue notifies;

	Data::Draft *localDraft() {
		return _localDraft.get();
	}
	Data::Draft *cloudDraft() {
		return _cloudDraft.get();
	}
	Data::Draft *editDraft() {
		return _editDraft.get();
	}
	void setLocalDraft(std::unique_ptr<Data::Draft> &&draft);
	void takeLocalDraft(History *from);
	void createLocalDraftFromCloud();
	void setCloudDraft(std::unique_ptr<Data::Draft> &&draft);
	Data::Draft *createCloudDraft(Data::Draft *fromDraft);
	void setEditDraft(std::unique_ptr<Data::Draft> &&draft);
	void clearLocalDraft();
	void clearCloudDraft();
	void clearEditDraft();
	void draftSavedToCloud();
	Data::Draft *draft() {
		return _editDraft ? editDraft() : localDraft();
	}

	// some fields below are a property of a currently displayed instance of this
	// conversation history not a property of the conversation history itself
public:
	// we save the last showAtMsgId to restore the state when switching
	// between different conversation histories
	MsgId showAtMsgId = ShowAtUnreadMsgId;

	// we save a pointer of the history item at the top of the displayed window
	// together with an offset from the window top to the top of this message
	// resulting scrollTop = top(scrollTopItem) + scrollTopOffset
	HistoryItem *scrollTopItem = nullptr;
	int scrollTopOffset = 0;
	void forgetScrollState() {
		scrollTopItem = nullptr;
	}

	// find the correct scrollTopItem and scrollTopOffset using given top
	// of the displayed window relative to the history start coord
	void countScrollState(int top);

protected:
	// when this item is destroyed scrollTopItem just points to the next one
	// and scrollTopOffset remains the same
	// if we are at the bottom of the window scrollTopItem == nullptr and
	// scrollTopOffset is undefined
	void getNextScrollTopItem(HistoryBlock *block, int32 i);

	// helper method for countScrollState(int top)
	void countScrollTopItem(int top);

public:
	bool lastKeyboardInited = false;
	bool lastKeyboardUsed = false;
	MsgId lastKeyboardId = 0;
	MsgId lastKeyboardHiddenId = 0;
	PeerId lastKeyboardFrom = 0;

	mtpRequestId sendRequestId = 0;

	mutable const HistoryItem *textCachedFor = nullptr; // cache
	mutable Text lastItemTextCache;

	typedef QList<MsgId> MediaOverview;
	MediaOverview overview[OverviewCount];

	bool overviewCountLoaded(int32 overviewIndex) const {
		return overviewCountData[overviewIndex] >= 0;
	}
	bool overviewLoaded(int32 overviewIndex) const {
		return overviewCount(overviewIndex) == overview[overviewIndex].size();
	}
	int32 overviewCount(int32 overviewIndex, int32 defaultValue = -1) const {
		int32 result = overviewCountData[overviewIndex], loaded = overview[overviewIndex].size();
		if (result < 0) return defaultValue;
		if (result < loaded) {
			if (result > 0) {
				const_cast<History*>(this)->overviewCountData[overviewIndex] = 0;
			}
			return loaded;
		}
		return result;
	}
	MsgId overviewMinId(int32 overviewIndex) const {
		for_const (auto msgId, overviewIds[overviewIndex]) {
			if (msgId > 0) {
				return msgId;
			}
		}
		return 0;
	}
	void overviewSliceDone(int32 overviewIndex, const MTPmessages_Messages &result, bool onlyCounts = false);
	bool overviewHasMsgId(int32 overviewIndex, MsgId msgId) const {
		return overviewIds[overviewIndex].constFind(msgId) != overviewIds[overviewIndex].cend();
	}

	void changeMsgId(MsgId oldId, MsgId newId);

	Text cloudDraftTextCache;

protected:
	void clearOnDestroy();
	HistoryItem *addNewToLastBlock(const MTPMessage &msg, NewMessageType type);

	friend class HistoryBlock;

	// this method just removes a block from the blocks list
	// when the last item from this block was detached and
	// calls the required previousItemChanged()
	void removeBlock(HistoryBlock *block);

	void clearBlocks(bool leaveItems);

	HistoryItem *createItem(const MTPMessage &msg, bool applyServiceAction, bool detachExistingItem);
	HistoryItem *createItemForwarded(MsgId id, MTPDmessage::Flags flags, QDateTime date, int32 from, HistoryMessage *msg);
	HistoryItem *createItemDocument(MsgId id, MTPDmessage::Flags flags, int32 viaBotId, MsgId replyTo, QDateTime date, int32 from, DocumentData *doc, const QString &caption, const MTPReplyMarkup &markup);
	HistoryItem *createItemPhoto(MsgId id, MTPDmessage::Flags flags, int32 viaBotId, MsgId replyTo, QDateTime date, int32 from, PhotoData *photo, const QString &caption, const MTPReplyMarkup &markup);
	HistoryItem *createItemGame(MsgId id, MTPDmessage::Flags flags, int32 viaBotId, MsgId replyTo, QDateTime date, int32 from, GameData *game, const MTPReplyMarkup &markup);

	HistoryItem *addNewItem(HistoryItem *adding, bool newMsg);
	HistoryItem *addNewInTheMiddle(HistoryItem *newItem, int32 blockIndex, int32 itemIndex);

	// All this methods add a new item to the first or last block
	// depending on if we are in isBuildingFronBlock() state.
	// The last block is created on the go if it is needed.

	// Adds the item to the back or front block, depending on
	// isBuildingFrontBlock(), creating the block if necessary.
	void addItemToBlock(HistoryItem *item);

	// Usually all new items are added to the last block.
	// Only when we scroll up and add a new slice to the
	// front we want to create a new front block.
	void startBuildingFrontBlock(int expectedItemsCount = 1);
	HistoryBlock *finishBuildingFrontBlock(); // Returns the built block or nullptr if nothing was added.
	bool isBuildingFrontBlock() const {
		return _buildingFrontBlock != nullptr;
	}

private:
	// After adding a new history slice check the lastMsg and newLoaded.
	void checkLastMsg();

	// Add all items to the media overview if we were not loaded at bottom and now are.
	void checkAddAllToOverview();

	enum class Flag {
		f_has_pending_resized_items = (1 << 0),
		f_pending_resize            = (1 << 1),
	};
	Q_DECLARE_FLAGS(Flags, Flag);
	Q_DECL_CONSTEXPR friend inline QFlags<Flags::enum_type> operator|(Flags::enum_type f1, Flags::enum_type f2) noexcept {
		return QFlags<Flags::enum_type>(f1) | f2;
	}
	Q_DECL_CONSTEXPR friend inline QFlags<Flags::enum_type> operator|(Flags::enum_type f1, QFlags<Flags::enum_type> f2) noexcept {
		return f2 | f1;
	}
	Q_DECL_CONSTEXPR friend inline QFlags<Flags::enum_type> operator~(Flags::enum_type f) noexcept {
		return ~QFlags<Flags::enum_type>(f);
	}
	Flags _flags;
	bool _mute;
	int32 _unreadCount = 0;

	Dialogs::RowsByLetter _chatListLinks[2];
	Dialogs::RowsByLetter &chatListLinks(Dialogs::Mode list) {
		return _chatListLinks[static_cast<int>(list)];
	}
	const Dialogs::RowsByLetter &chatListLinks(Dialogs::Mode list) const {
		return _chatListLinks[static_cast<int>(list)];
	}
	Dialogs::Row *mainChatListLink(Dialogs::Mode list) const {
		auto it = chatListLinks(list).constFind(0);
		t_assert(it != chatListLinks(list).cend());
		return it.value();
	}
	uint64 _sortKeyInChatList = 0; // like ((unixtime) << 32) | (incremented counter)

	using MediaOverviewIds = OrderedSet<MsgId>;
	MediaOverviewIds overviewIds[OverviewCount];
	int32 overviewCountData[OverviewCount]; // -1 - not loaded, 0 - all loaded, > 0 - count, but not all loaded

	// A pointer to the block that is currently being built.
	// We hold this pointer so we can destroy it while building
	// and then create a new one if it is necessary.
	struct BuildingBlock {
		int expectedItemsCount = 0; // optimization for block->items.reserve() call
		HistoryBlock *block = nullptr;
	};
	std::unique_ptr<BuildingBlock> _buildingFrontBlock;

	// Creates if necessary a new block for adding item.
	// Depending on isBuildingFrontBlock() gets front or back block.
	HistoryBlock *prepareBlockForAddingItem();

	std::unique_ptr<Data::Draft> _localDraft, _cloudDraft;
	std::unique_ptr<Data::Draft> _editDraft;

	using TypingUsers = QMap<UserData*, TimeMs>;
	TypingUsers _typing;
	using SendActionUsers = QMap<UserData*, SendAction>;
	SendActionUsers _sendActions;
	QString _sendActionString;
	Text _sendActionText;
	Ui::SendActionAnimation _sendActionAnimation;
	QMap<SendAction::Type, TimeMs> _mySendActions;

	int _pinnedIndex = 0; // > 0 for pinned dialogs

 };

class HistoryJoined;
class ChannelHistory : public History {
public:
	using History::History;

	void messageDetached(HistoryItem *msg);

	void getRangeDifference();
	void getRangeDifferenceNext(int32 pts);

	HistoryJoined *insertJoinedMessage(bool unread);
	void checkJoinedMessage(bool createUnread = false);
	const QDateTime &maxReadMessageDate();

	~ChannelHistory();

private:
	friend class History;
	HistoryItem* addNewChannelMessage(const MTPMessage &msg, NewMessageType type);
	HistoryItem *addNewToBlocks(const MTPMessage &msg, NewMessageType type);

	void checkMaxReadMessageDate();

	void cleared(bool leaveItems);

	QDateTime _maxReadMessageDate;

	HistoryJoined *_joinedMessage = nullptr;

	MsgId _rangeDifferenceFromId, _rangeDifferenceToId;
	int32 _rangeDifferencePts;
	mtpRequestId _rangeDifferenceRequestId;

};

class HistoryBlock {
public:
	HistoryBlock(gsl::not_null<History*> history) : _history(history) {
	}

	HistoryBlock(const HistoryBlock &) = delete;
	HistoryBlock &operator=(const HistoryBlock &) = delete;

	QVector<HistoryItem*> items;

	void clear(bool leaveItems = false);
	~HistoryBlock() {
		clear();
	}
	void removeItem(HistoryItem *item);

	int resizeGetHeight(int newWidth, bool resizeAllItems);
	int y() const {
		return _y;
	}
	void setY(int y) {
		_y = y;
	}
	int height() const {
		return _height;
	}
	gsl::not_null<History*> history() const {
		return _history;
	}

	HistoryBlock *previousBlock() const {
		Expects(_indexInHistory >= 0);

		return (_indexInHistory > 0) ? _history->blocks.at(_indexInHistory - 1) : nullptr;
	}
	HistoryBlock *nextBlock() const {
		Expects(_indexInHistory >= 0);

		return (_indexInHistory + 1 < _history->blocks.size()) ? _history->blocks.at(_indexInHistory + 1) : nullptr;
	}
	void setIndexInHistory(int index) {
		_indexInHistory = index;
	}
	int indexInHistory() const {
		Expects(_indexInHistory >= 0);
		Expects(_indexInHistory < _history->blocks.size());
		Expects(_history->blocks[_indexInHistory] == this);

		return _indexInHistory;
	}

protected:
	const gsl::not_null<History*> _history;

	int _y = 0;
	int _height = 0;
	int _indexInHistory = -1;

};
