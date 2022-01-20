/**
 * Serlio - Esri CityEngine Plugin for Autodesk Maya
 *
 * See https://github.com/esri/serlio for build and usage instructions.
 *
 * Copyright (c) 2012-2019 Esri R&D Center Zurich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AssetCache.h"

#include "utils/LogHandler.h"
#include "utils/stduuid/uuid.h"

#include <cassert>
#include <fstream>
#include <functional>
#include <ostream>
#include <string_view>

namespace {

bool writeCacheEntry(const std::filesystem::path& assetPath, const uint8_t* buffer, size_t size) noexcept {
	std::ofstream stream(assetPath, std::ofstream::binary | std::ofstream::trunc);
	if (!stream)
		return false;
	stream.write(reinterpret_cast<const char*>(buffer), size);
	if (!stream)
		return false;
	stream.close();
	return true;
}

void removeCacheEntry(const std::filesystem::path& expiredAssetPath) {
	if (std::error_code removeError; !std::filesystem::remove(expiredAssetPath, removeError))
		LOG_WRN << "Failed to delete expired asset cache entry " << expiredAssetPath << ": " << removeError.message();
}

} // namespace

AssetCache::AssetCache(const std::filesystem::path& cacheRootPath) : mCacheRootPath(cacheRootPath) {}

std::filesystem::path AssetCache::put(const wchar_t* uri, const wchar_t* fileName, const uint8_t* buffer, size_t size) {
	assert(uri != nullptr);

	const std::string_view bufferView(reinterpret_cast<const char*>(buffer), size);
	const size_t hash = std::hash<std::string_view>{}(bufferView);

	const auto it = std::find_if(mCache.begin(), mCache.end(),
	                             [&uri](const auto& p) { return (std::wcscmp(p.first.c_str(), uri) == 0); });

	// reuse cached asset if uri and hash match
	if ((it != mCache.end()) && (it->second.second == hash)) {
		const std::filesystem::path& assetPath = it->second.first;
		return assetPath;
	}

	const std::filesystem::path newAssetPath = getCachedPath(fileName);
	if (newAssetPath.empty()) {
		LOG_ERR << "Invalid URI, cannot cache the asset: " << uri;
		return {};		
	}

	if (!writeCacheEntry(newAssetPath, buffer, size)) {
		LOG_ERR << "Failed to put asset into cache, skipping asset: " << newAssetPath;
		return {};
	}

	if (it == mCache.end()) {
		mCache.emplace(uri, std::make_pair(newAssetPath, hash));
	}
	else {
		// handle hash mismatch
		removeCacheEntry(it->second.first);
		it->second = std::make_pair(newAssetPath, hash);
	}

	return newAssetPath;
}

std::filesystem::path AssetCache::getCachedPath(const wchar_t* fileName) const {
	// we use an UUID to have unique entries (filenames might clash across different RPKs)
	const uuids::uuid uuid = uuids::uuid_system_generator{}();
	std::wstring cachedAssetName = uuids::to_wstring(uuid);

	// we append the filename constructed by the encoder from the URI
	assert(fileName != nullptr);
	cachedAssetName.append(L"_").append(fileName);

	const std::filesystem::path cachedAssetPath = mCacheRootPath / cachedAssetName;
	return cachedAssetPath;
}
