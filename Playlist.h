#pragma once
#include <vector>
#include <string>
#include "Song.h"

class Playlist {
public:
    Playlist(const std::string& name);

    const std::string& getName() const;
    const std::vector<Song>& getSongs() const;
    void addSong(const Song& song);
    void removeSong(const Song& song);

private:
    std::string name;
    std::vector<Song> songs;
};
