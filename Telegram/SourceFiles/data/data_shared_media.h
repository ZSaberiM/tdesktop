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

#include "storage/storage_shared_media.h"
#include "base/weak_unique_ptr.h"
#include "data/data_sparse_ids.h"

base::optional<Storage::SharedMediaType> SharedMediaOverviewType(
	Storage::SharedMediaType type);
void SharedMediaShowOverview(
	Storage::SharedMediaType type,
	not_null<History*> history);
bool SharedMediaAllowSearch(Storage::SharedMediaType type);

rpl::producer<SparseIdsSlice> SharedMediaViewer(
	Storage::SharedMediaKey key,
	int limitBefore,
	int limitAfter);

struct SharedMediaMergedKey {
	using Type = Storage::SharedMediaType;

	SharedMediaMergedKey(
		SparseIdsMergedSlice::Key mergedKey,
		Type type)
	: mergedKey(mergedKey)
	, type(type) {
	}

	bool operator==(const SharedMediaMergedKey &other) const {
		return (mergedKey == other.mergedKey)
			&& (type == other.type);
	}

	SparseIdsMergedSlice::Key mergedKey;
	Type type = Type::kCount;

};

rpl::producer<SparseIdsMergedSlice> SharedMediaMergedViewer(
	SharedMediaMergedKey key,
	int limitBefore,
	int limitAfter);

class SharedMediaWithLastSlice {
public:
	using Type = Storage::SharedMediaType;

	using Value = base::variant<FullMsgId, not_null<PhotoData*>>;
	using MessageId = SparseIdsMergedSlice::UniversalMsgId;
	using UniversalMsgId = base::variant<
		MessageId,
		not_null<PhotoData*>>;

	struct Key {
		Key(
			PeerId peerId,
			PeerId migratedPeerId,
			Type type,
			UniversalMsgId universalId)
		: peerId(peerId)
		, migratedPeerId(migratedPeerId)
		, type(type)
		, universalId(universalId) {
			Expects(base::get_if<MessageId>(&universalId) != nullptr
				|| type == Type::ChatPhoto);
		}

		bool operator==(const Key &other) const {
			return (peerId == other.peerId)
				&& (migratedPeerId == other.migratedPeerId)
				&& (type == other.type)
				&& (universalId == other.universalId);
		}
		bool operator!=(const Key &other) const {
			return !(*this == other);
		}

		PeerId peerId = 0;
		PeerId migratedPeerId = 0;
		Type type = Type::kCount;
		UniversalMsgId universalId;

	};

	SharedMediaWithLastSlice(Key key);
	SharedMediaWithLastSlice(
		Key key,
		SparseIdsMergedSlice slice,
		base::optional<SparseIdsMergedSlice> ending);

	base::optional<int> fullCount() const;
	base::optional<int> skippedBefore() const;
	base::optional<int> skippedAfter() const;
	base::optional<int> indexOf(Value fullId) const;
	int size() const;
	Value operator[](int index) const;
	base::optional<int> distance(const Key &a, const Key &b) const;

	void reverse();

	static SparseIdsMergedSlice::Key ViewerKey(const Key &key) {
		return {
			key.peerId,
			key.migratedPeerId,
			base::get_if<MessageId>(&key.universalId)
				? (*base::get_if<MessageId>(&key.universalId))
				: ServerMaxMsgId - 1
		};
	}
	static SparseIdsMergedSlice::Key EndingKey(const Key &key) {
		return {
			key.peerId,
			key.migratedPeerId,
			ServerMaxMsgId - 1
		};
	}

private:
	static base::optional<SparseIdsMergedSlice> EndingSlice(const Key &key) {
		return base::get_if<MessageId>(&key.universalId)
			? base::make_optional(SparseIdsMergedSlice(EndingKey(key)))
			: base::none;
	}

	static PhotoId LastPeerPhotoId(PeerId peerId);
	static base::optional<bool> IsLastIsolated(
		const SparseIdsMergedSlice &slice,
		const base::optional<SparseIdsMergedSlice> &ending,
		PhotoId lastPeerPhotoId);
	static base::optional<FullMsgId> LastFullMsgId(
		const SparseIdsMergedSlice &slice);
	static base::optional<int> Add(
			const base::optional<int> &a,
			const base::optional<int> &b) {
		return (a && b) ? base::make_optional(*a + *b) : base::none;
	}
	static Value ComputeId(PeerId peerId, MsgId msgId) {
		return FullMsgId(
			peerIsChannel(peerId) ? peerToBareInt(peerId) : 0,
			msgId);
	}
	static Value ComputeId(const Key &key) {
		if (auto messageId = base::get_if<MessageId>(&key.universalId)) {
			return (*messageId >= 0)
				? ComputeId(key.peerId, *messageId)
				: ComputeId(key.migratedPeerId, ServerMaxMsgId + *messageId);
		}
		return *base::get_if<not_null<PhotoData*>>(&key.universalId);
	}

	bool isolatedInSlice() const {
		return (_slice.skippedAfter() != 0);
	}
	base::optional<int> lastPhotoSkip() const {
		return _isolatedLastPhoto
			| [](bool isolated) { return isolated ? 1 : 0; };
	}

	base::optional<int> skippedBeforeImpl() const;
	base::optional<int> skippedAfterImpl() const;
	base::optional<int> indexOfImpl(Value fullId) const;

	Key _key;
	SparseIdsMergedSlice _slice;
	base::optional<SparseIdsMergedSlice> _ending;
	PhotoId _lastPhotoId = 0;
	base::optional<bool> _isolatedLastPhoto;
	bool _reversed = false;

};

rpl::producer<SharedMediaWithLastSlice> SharedMediaWithLastViewer(
	SharedMediaWithLastSlice::Key key,
	int limitBefore,
	int limitAfter);

rpl::producer<SharedMediaWithLastSlice> SharedMediaWithLastReversedViewer(
	SharedMediaWithLastSlice::Key key,
	int limitBefore,
	int limitAfter);
