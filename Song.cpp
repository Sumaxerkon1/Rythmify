#include "Song.h"
#include <sstream>
#include <iomanip>

Song::Song(const std::string& path, const std::string& title,
           const std::string& artist, const std::string& album,
           const std::string& genre, int duration)
    : path(path), title(title), artist(artist),
      album(album), genre(genre), duration(duration) {}

const std::string& Song::getPath()   const { return path; }
const std::string& Song::getTitle()  const { return title; }
const std::string& Song::getArtist() const { return artist; }
const std::string& Song::getAlbum()  const { return album; }
const std::string& Song::getGenre()  const { return genre; }
int Song::getDurationInSeconds()      const { return duration; }

bool Song::operator==(const Song& other) const {
    return path == other.path;
}

std::string Song::getFormattedTime() const {
    int h = duration / 3600;
    int m = (duration % 3600) / 60;
    int s = duration % 60;

    std::ostringstream ss;
    if (h > 0)
        ss << std::setw(2) << std::setfill('0') << h << ":";
    ss << std::setw(2) << std::setfill('0') << m << ":"
       << std::setw(2) << std::setfill('0') << s;
    return ss.str();
}
