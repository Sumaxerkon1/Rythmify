#pragma once
#include "Playlist.h"
#include <string>
#include <vector>
#define NOMINMAX
#include <windows.h>

class MusicPlayer {
public:
    MusicPlayer();
    ~MusicPlayer();

    const Song* getCurrentSong() const;
    const Playlist* getCurrentPlaylist() const;
    bool isPlaying() const;
    bool isPaused() const;
    bool checkIfSongEnded();

    void play(const Song& song);
    void resume();
    void pause();
    void stop();
    void next();
    void previous();

    void createPlaylist(const std::string& name);
    void deletePlaylist(const std::string& name);
    void switchPlaylist(const std::string& name);
    void addSongToCurrentPlaylist(const Song& song);
    void removeSongFromCurrentPlaylist(const Song& song);
    const std::vector<Playlist>& getPlaylists() const;

    int importFolder(const std::string& folderPath);

    void saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);

    void printCurrentSong() const;
    void printCurrentPlaylist() const;

private:
    Song* currentSong = nullptr;
    Playlist* currentPlaylist = nullptr;
    std::vector<Playlist> playlists;

    bool playing = false;
    bool paused  = false;
    int currentIndex = -1;

    void openAndPlay(const std::string& path);
    void closeDevice();
};
