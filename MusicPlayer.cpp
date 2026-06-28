#include "MusicPlayer.h"
#include "ID3Reader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

static void mci(const std::string& cmd) {
    MCIERROR err = mciSendStringA(cmd.c_str(), nullptr, 0, nullptr);
    if (err != 0) {
        char buf[256];
        mciGetErrorStringA(err, buf, sizeof(buf));
        std::cerr << "[MCI] " << buf << "\n";
    }
}

MusicPlayer::MusicPlayer() {}

MusicPlayer::~MusicPlayer() {
    closeDevice();
    delete currentSong;
}

void MusicPlayer::openAndPlay(const std::string& path) {
    closeDevice();
    mci("open \"" + path + "\" type mpegvideo alias track");
    mci("play track");
}

void MusicPlayer::closeDevice() {
    mci("stop track");
    mci("close track");
}

bool MusicPlayer::checkIfSongEnded() {
    if (!playing || paused) return false;

    char status[64] = {};
    mciSendStringA("status track mode", status, sizeof(status), nullptr);
    return std::string(status) == "stopped";
}

const Song* MusicPlayer::getCurrentSong() const { return currentSong; }
const Playlist* MusicPlayer::getCurrentPlaylist() const { return currentPlaylist; }
bool MusicPlayer::isPlaying() const { return playing && !paused; }
bool MusicPlayer::isPaused()  const { return paused; }

void MusicPlayer::play(const Song& song) {
    delete currentSong;
    currentSong = new Song(song);

    currentIndex = -1;
    if (currentPlaylist) {
        const auto& songs = currentPlaylist->getSongs();
        for (int i = 0; i < (int)songs.size(); i++)
            if (songs[i] == song) { currentIndex = i; break; }
    }

    playing = true;
    paused  = false;
    openAndPlay(song.getPath());
}

void MusicPlayer::resume() {
    if (!currentSong) { std::cout << "No song selected.\n"; return; }
    if (!paused) { std::cout << "Already playing.\n"; return; }
    mci("resume track");
    paused = false;
}

void MusicPlayer::pause() {
    if (!playing || paused) { std::cout << "Nothing is playing.\n"; return; }
    mci("pause track");
    paused = true;
}

void MusicPlayer::stop() {
    closeDevice();
    playing = false;
    paused  = false;
}

void MusicPlayer::next() {
    if (!currentPlaylist || currentPlaylist->getSongs().empty()) return;
    const auto& songs = currentPlaylist->getSongs();
    currentIndex = (currentIndex + 1) % (int)songs.size();
    play(songs[currentIndex]);
}

void MusicPlayer::previous() {
    if (!currentPlaylist || currentPlaylist->getSongs().empty()) return;
    const auto& songs = currentPlaylist->getSongs();
    currentIndex = (currentIndex - 1 + (int)songs.size()) % (int)songs.size();
    play(songs[currentIndex]);
}

const std::vector<Playlist>& MusicPlayer::getPlaylists() const { return playlists; }

void MusicPlayer::createPlaylist(const std::string& name) {
    for (const auto& p : playlists)
        if (p.getName() == name) { std::cout << "Already exists.\n"; return; }
    playlists.emplace_back(name);
    std::cout << "Created: " << name << "\n";
}

void MusicPlayer::deletePlaylist(const std::string& name) {
    auto it = std::find_if(playlists.begin(), playlists.end(),
        [&](const Playlist& p) { return p.getName() == name; });
    if (it == playlists.end()) { std::cout << "Not found.\n"; return; }
    if (currentPlaylist && currentPlaylist->getName() == name) {
        stop();
        currentPlaylist = nullptr;
        currentIndex = -1;
    }
    playlists.erase(it);
}

void MusicPlayer::switchPlaylist(const std::string& name) {
    for (auto& p : playlists) {
        if (p.getName() == name) {
            currentPlaylist = &p;
            currentIndex = -1;
            std::cout << "Switched to: " << name << " (" << p.getSongs().size() << " songs)\n";
            return;
        }
    }
    std::cout << "Playlist not found.\n";
}

void MusicPlayer::addSongToCurrentPlaylist(const Song& song) {
    if (!currentPlaylist) { std::cout << "No playlist selected.\n"; return; }
    currentPlaylist->addSong(song);
    std::cout << "Added: " << song.getTitle() << "\n";
}

void MusicPlayer::removeSongFromCurrentPlaylist(const Song& song) {
    if (!currentPlaylist) { std::cout << "No playlist selected.\n"; return; }
    currentPlaylist->removeSong(song);
    std::cout << "Removed: " << song.getTitle() << "\n";
}

void MusicPlayer::printCurrentSong() const {
    if (!currentSong) { std::cout << "Nothing playing.\n"; return; }
    std::cout << currentSong->getTitle() << " - " << currentSong->getArtist()
              << " [" << currentSong->getFormattedTime() << "]\n";
}

void MusicPlayer::printCurrentPlaylist() const {
    if (!currentPlaylist) { std::cout << "No playlist selected.\n"; return; }
    const auto& songs = currentPlaylist->getSongs();
    for (int i = 0; i < (int)songs.size(); i++)
        std::cout << (i == currentIndex ? " > " : "   ")
                  << i + 1 << ". " << songs[i].getTitle()
                  << " [" << songs[i].getFormattedTime() << "]\n";
}

void MusicPlayer::saveToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out) { std::cerr << "Can't open file.\n"; return; }
    for (const auto& pl : playlists) {
        out << "PLAYLIST " << pl.getName() << "\n";
        for (const auto& s : pl.getSongs())
            out << "SONG " << s.getPath() << "|" << s.getTitle() << "|"
                << s.getArtist() << "|" << s.getAlbum() << "|"
                << s.getGenre() << "|" << s.getDurationInSeconds() << "\n";
    }
    if (currentPlaylist)
        out << "CURRENT_PLAYLIST " << currentPlaylist->getName() << "\n";
}

void MusicPlayer::loadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) return;

    playlists.clear();
    currentPlaylist = nullptr;
    currentIndex = -1;

    std::string line, activePL;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        if (line.rfind("PLAYLIST ", 0) == 0) {
            playlists.emplace_back(line.substr(9));
        } else if (line.rfind("SONG ", 0) == 0 && !playlists.empty()) {
            std::istringstream ss(line.substr(5));
            std::vector<std::string> parts;
            std::string tok;
            while (std::getline(ss, tok, '|')) parts.push_back(tok);
            if (parts.size() == 6)
                playlists.back().addSong(Song(parts[0], parts[1], parts[2],
                                              parts[3], parts[4], std::stoi(parts[5])));
        } else if (line.rfind("CURRENT_PLAYLIST ", 0) == 0) {
            activePL = line.substr(17);
        }
    }

    if (!activePL.empty()) switchPlaylist(activePL);
}

int MusicPlayer::importFolder(const std::string& folderPath) {
    if (!currentPlaylist) { std::cout << "No playlist selected.\n"; return 0; }
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        std::cout << "Folder not found.\n"; return 0;
    }

    int count = 0;
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".mp3" && ext != ".wav") continue;

        std::string path  = entry.path().string();
        std::string title = entry.path().stem().string();
        std::string artist = "Unknown", album = "";
        int duration = 0;

        if (ext == ".mp3") {
            auto tags = readID3Tags(path);
            if (!tags.title.empty())  title  = tags.title;
            if (!tags.artist.empty()) artist = tags.artist;
            if (!tags.album.empty())  album  = tags.album;
            duration = tags.durationSeconds;
        }

        currentPlaylist->addSong(Song(path, title, artist, album, "", duration));
        std::cout << "  + " << title << "\n";
        count++;
    }

    std::cout << "Imported " << count << " songs.\n";
    return count;
}
