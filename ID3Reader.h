#pragma once

#include <string>
#include <fstream>
#include <cstdint>
#include <algorithm>

struct ID3Tags {
    std::string title;
    std::string artist;
    std::string album;
    int durationSeconds = 0;
    bool found = false;
};

namespace id3detail {

    inline uint32_t readSynchsafe(const unsigned char* b) {
        return (uint32_t(b[0]) << 21) | (uint32_t(b[1]) << 14) |
               (uint32_t(b[2]) << 7)  |  uint32_t(b[3]);
    }

    inline std::string trim(std::string s) {
        while (!s.empty() && (s.back() == '\0' || s.back() == ' ' || s.back() == '\r' || s.back() == '\n'))
            s.pop_back();
        size_t start = 0;
        while (start < s.size() && (s[start] == '\0' || s[start] == ' ')) ++start;
        return s.substr(start);
    }

    inline std::string decodeTextFrame(const std::string& raw) {
        if (raw.empty()) return "";
        unsigned char enc = (unsigned char)raw[0];
        std::string body = raw.substr(1);
        if (enc == 0 || enc == 3) return trim(body);
        std::string out;
        size_t i = 0;
        if (body.size() >= 2 && ((unsigned char)body[0] == 0xFF || (unsigned char)body[0] == 0xFE))
            i = 2;
        for (; i + 1 < body.size(); i += 2) {
            char c = body[i];
            if (c != '\0') out += c;
        }
        return trim(out);
    }

    inline uint32_t getID3v2Size(std::ifstream& f) {
        f.seekg(0);
        char header[10];
        f.read(header, 10);
        if (f.gcount() < 10) return 0;
        if (!(header[0] == 'I' && header[1] == 'D' && header[2] == '3')) return 0;
        return 10 + readSynchsafe((const unsigned char*)header + 6);
    }

    inline std::string readID3v2Frame(std::ifstream& f, const std::string& frameWanted) {
        f.seekg(0);
        char header[10];
        f.read(header, 10);
        if (f.gcount() < 10) return "";
        if (!(header[0] == 'I' && header[1] == 'D' && header[2] == '3')) return "";

        uint32_t tagSize = readSynchsafe((const unsigned char*)header + 6);
        size_t pos = 10;
        size_t end = 10 + tagSize;

        while (pos + 10 <= end) {
            char fh[10];
            f.seekg(pos);
            f.read(fh, 10);
            if (f.gcount() < 10) break;

            std::string frameId(fh, 4);
            if (frameId == "\0\0\0\0" || frameId.empty()) break;

            uint32_t frameSize =
                (uint32_t((unsigned char)fh[4]) << 24) |
                (uint32_t((unsigned char)fh[5]) << 16) |
                (uint32_t((unsigned char)fh[6]) << 8)  |
                 uint32_t((unsigned char)fh[7]);

            if (frameSize == 0 || frameSize > tagSize) break;

            if (frameId == frameWanted) {
                std::string raw(frameSize, '\0');
                f.read(&raw[0], frameSize);
                return decodeTextFrame(raw);
            }

            pos += 10 + frameSize;
        }
        return "";
    }

    inline ID3Tags readID3v1(std::ifstream& f) {
        ID3Tags tags;
        f.seekg(0, std::ios::end);
        std::streamoff fileSize = f.tellg();
        if (fileSize < 128) return tags;

        f.seekg(fileSize - 128);
        char buf[128];
        f.read(buf, 128);
        if (f.gcount() < 128) return tags;
        if (!(buf[0] == 'T' && buf[1] == 'A' && buf[2] == 'G')) return tags;

        tags.title  = trim(std::string(buf + 3,  30));
        tags.artist = trim(std::string(buf + 33, 30));
        tags.album  = trim(std::string(buf + 63, 30));
        tags.found  = !tags.title.empty() || !tags.artist.empty();
        return tags;
    }

    static const int MP3_BITRATES[16] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
    static const int MP3_SAMPLERATES[4] = {44100,48000,32000,0};

    inline int calcMP3Duration(std::ifstream& f, uint32_t id3Size) {
        f.seekg(0, std::ios::end);
        std::streamoff fileSize = f.tellg();

        // check for ID3v1 tag at end (128 bytes)
        std::streamoff audioEnd = fileSize;
        {
            f.seekg(fileSize - 128);
            char tag[3];
            f.read(tag, 3);
            if (tag[0]=='T' && tag[1]=='A' && tag[2]=='G')
                audioEnd = fileSize - 128;
        }

        // scan for first valid mp3 frame after the ID3v2 tag
        f.seekg(id3Size);
        unsigned char buf[4];
        int attempts = 0;
        while (attempts < 8192) {
            f.read((char*)buf, 1);
            if (f.gcount() < 1 || f.eof()) return 0;
            if (buf[0] != 0xFF) { attempts++; continue; }
            f.read((char*)buf + 1, 3);
            if (f.gcount() < 3) return 0;

            // sync word check (0xFFE0 or higher)
            if ((buf[1] & 0xE0) != 0xE0) { attempts++; f.seekg(-3, std::ios::cur); continue; }

            int bitrateIdx  = (buf[2] >> 4) & 0x0F;
            int srateIdx    = (buf[2] >> 2) & 0x03;
            int padding     = (buf[2] >> 1) & 0x01;

            int bitrate    = MP3_BITRATES[bitrateIdx];   // kbps
            int samplerate = MP3_SAMPLERATES[srateIdx];  // Hz

            if (bitrate == 0 || samplerate == 0) { attempts++; continue; }

            // check for Xing/Info VBR header (36 bytes into this frame)
            std::streamoff frameStart = (std::streamoff)f.tellg() - 4;
            f.seekg(frameStart + 36);
            char xing[4];
            f.read(xing, 4);
            if (f.gcount() == 4) {
                bool isXing = (xing[0]=='X'&&xing[1]=='i'&&xing[2]=='n'&&xing[3]=='g') ||
                              (xing[0]=='I'&&xing[1]=='n'&&xing[2]=='f'&&xing[3]=='o');
                if (isXing) {
                    unsigned char flags[4];
                    f.read((char*)flags, 4);
                    if (f.gcount() == 4 && (flags[3] & 0x01)) {
                        unsigned char fc[4];
                        f.read((char*)fc, 4);
                        if (f.gcount() == 4) {
                            uint32_t frames = (uint32_t(fc[0])<<24)|(uint32_t(fc[1])<<16)|
                                              (uint32_t(fc[2])<<8)|fc[3];
                            return (int)((double)frames * 1152.0 / samplerate);
                        }
                    }
                }
            }

            // CBR: estimate from file size and bitrate
            std::streamoff audioSize = audioEnd - id3Size;
            return (int)((double)audioSize * 8.0 / (bitrate * 1000.0));
        }
        return 0;
    }
}

inline ID3Tags readID3Tags(const std::string& filepath) {
    ID3Tags tags;
    std::ifstream f(filepath, std::ios::binary);
    if (!f) return tags;

    std::string title  = id3detail::readID3v2Frame(f, "TIT2");
    std::string artist = id3detail::readID3v2Frame(f, "TPE1");
    std::string album  = id3detail::readID3v2Frame(f, "TALB");

    if (!title.empty() || !artist.empty() || !album.empty()) {
        tags.title  = title;
        tags.artist = artist;
        tags.album  = album;
        tags.found  = true;
    } else {
        ID3Tags v1 = id3detail::readID3v1(f);
        tags.title  = v1.title;
        tags.artist = v1.artist;
        tags.album  = v1.album;
        tags.found  = v1.found;
    }

    uint32_t id3Size = id3detail::getID3v2Size(f);
    tags.durationSeconds = id3detail::calcMP3Duration(f, id3Size);

    return tags;
}
