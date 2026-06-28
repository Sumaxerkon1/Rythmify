#pragma once
#include <string>

class Song {
public:
    Song(const std::string& path, const std::string& title,
         const std::string& artist, const std::string& album,
         const std::string& genre, int duration);

    const std::string& getPath() const;
    const std::string& getTitle() const;
    const std::string& getArtist() const;
    const std::string& getAlbum() const;
    const std::string& getGenre() const;
    int getDurationInSeconds() const;
    std::string getFormattedTime() const;
    bool operator==(const Song& other) const;

private:
    std::string path;
    std::string title;
    std::string artist;
    std::string album;
    std::string genre;
    int duration;
};
