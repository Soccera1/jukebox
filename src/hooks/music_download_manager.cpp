#include "music_download_manager.hpp"

#include <optional>

#include "../../include/nong.hpp"
#include "../events/get_song_info_event.hpp"
#include "../managers/nong_manager.hpp"
#include "Geode/binding/GameLevelManager.hpp"
#include "Geode/binding/MusicDownloadManager.hpp"
#include "Geode/binding/SongInfoObject.hpp"
#include "Geode/cocos/cocoa/CCString.h"
#include "Geode/loader/Log.hpp"
#include "Geode/utils/cocos.hpp"
#include "Geode/utils/string.hpp"

using namespace jukebox;

gd::string JBMusicDownloadManager::pathForSong(int id) {
    NongManager::get()->m_currentlyPreparingNong = std::nullopt;
    std::optional<Nongs*> nongs = NongManager::get()->getNongs(id);
    if (!nongs.has_value()) {
        return MusicDownloadManager::pathForSong(id);
    }
    auto value = nongs.value();
    auto active = value->activeNong();
    if (!std::filesystem::exists(active.path().value())) {
        return MusicDownloadManager::pathForSong(id);
    }
    NongManager::get()->m_currentlyPreparingNong = value;
#ifdef GEODE_IS_WINDOWS
    return geode::utils::string::wideToUtf8(active.path().value().c_str());
#else
    return active.path().value().string();
#endif
}

void JBMusicDownloadManager::onGetSongInfoCompleted(gd::string p1,
                                                    gd::string p2) {
    MusicDownloadManager::onGetSongInfoCompleted(p1, p2);
    int songID = std::stoi(p2);

    constexpr size_t SONG_NAME_INDEX = 3;
    constexpr size_t ARTIST_NAME_INDEX = 7;

    CCDictionary* dict =
        GameLevelManager::sharedState()->responseToDict(p1, true);

    CCArrayExt<CCString*> keys = CCArrayExt<CCString*>(dict->allKeys());

    // Size 0 -> got an invalid response from the servers
    if (keys.size() == 0 || keys.size() < ARTIST_NAME_INDEX) {
        return;
    }

    CCString* songName = keys[SONG_NAME_INDEX];
    CCString* artistName = keys[ARTIST_NAME_INDEX];

    if (!songName || !artistName) {
        return;
    }

    GetSongInfoEvent(songName->getCString(), artistName->getCString(), songID)
        .post();
}

SongInfoObject* JBMusicDownloadManager::getSongInfoObject(int id) {
    auto og = MusicDownloadManager::getSongInfoObject(id);
    if (og == nullptr) {
        return og;
    }
    auto res = NongManager::get()->getNongs(id);
    if (res.has_value()) {
        auto active = res.value()->activeNong();
        og->m_songName = active.metadata()->m_name;
        og->m_artistName = active.metadata()->m_artist;
    }
    return og;
}
