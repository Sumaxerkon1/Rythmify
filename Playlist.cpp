#include "Playlist.h"
#include <algorithm>

Playlist::Playlist(const std::string& name) : name(name) {}

const std::string& Playlist::getName() const { return name; }
const std::vector<Song>& Playlist::getSongs() const { return songs; }

void Playlist::addSong(const Song& song) {
    songs.push_back(song);
}

void Playlist::removeSong(const Song& song) {
    auto it = std::find(songs.begin(), songs.end(), song);
    if (it != songs.end())
        songs.erase(it);
}
