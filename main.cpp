#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <vector>
#define NOMINMAX
#include <windows.h>
#include "MusicPlayer.h"

static const std::string SAVE_FILE = "rythmify_data.txt";

#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"
#define FG_RED      "\033[31m"
#define FG_GREEN    "\033[32m"
#define FG_MAGENTA  "\033[35m"
#define FG_CYAN     "\033[36m"
#define FG_WHITE    "\033[37m"

static const int UI_WIDTH = 80;

static void enableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(CP_UTF8);
}

static void clearScreen() { std::cout << "\033[2J\033[H"; }
static void hideCursor()   { std::cout << "\033[?25l"; }
static void showCursor()   { std::cout << "\033[?25h"; }

static void printLine() {
    std::cout << DIM FG_CYAN;
    for (int i = 0; i < UI_WIDTH; ++i) std::cout << "\xe2\x94\x80";
    std::cout << RESET "\n";
}

static void printCentered(const std::string& text, const std::string& color = "") {
    int pad = (UI_WIDTH - (int)text.size()) / 2;
    if (pad < 0) pad = 0;
    std::cout << color;
    for (int i = 0; i < pad; ++i) std::cout << ' ';
    std::cout << text << RESET "\n";
}

static void drawHeader() {
    printLine();
    printCentered("R Y T H M I F Y", BOLD FG_MAGENTA);
    printCentered("Your Personal Music Player", DIM FG_CYAN);
    printLine();
}

static void drawNowPlaying(const MusicPlayer& player) {
    const Song* s = player.getCurrentSong();
    printLine();
    if (!s) {
        printCentered("[ No song playing ]", DIM FG_WHITE);
    } else {
        std::string status = player.isPlaying() ? "\xe2\x96\xb6  PLAYING" :
                             player.isPaused()  ? "\xe2\x8f\xb8  PAUSED"  : "\xe2\x8f\xb9  STOPPED";
        std::string statusColor = player.isPlaying() ? FG_GREEN :
                                  player.isPaused()  ? FG_GREEN : FG_RED;

        printCentered(status, BOLD + std::string(statusColor));
        printCentered(s->getTitle(),  BOLD FG_WHITE);
        printCentered(s->getArtist() + "  -  " + s->getAlbum(), DIM FG_CYAN);
        printCentered("[" + s->getFormattedTime() + "]", FG_MAGENTA);
    }
    printLine();
}

static void drawPlaylist(const MusicPlayer& player) {
    const Playlist* pl = player.getCurrentPlaylist();
    const Song*     cs = player.getCurrentSong();

    if (!pl) {
        std::cout << DIM FG_WHITE "  No playlist selected. Use 'switch <name>' or 'create <name>'.\n" RESET;
        return;
    }

    std::cout << BOLD FG_CYAN "  Playlist: " RESET
              << BOLD FG_WHITE << pl->getName() << RESET
              << DIM FG_WHITE "  (" << pl->getSongs().size() << " songs)\n" RESET;
    printLine();

    const auto& songs = pl->getSongs();
    if (songs.empty()) {
        std::cout << DIM "  Empty playlist. Use 'add' to add songs.\n" RESET;
        return;
    }

    for (int i = 0; i < (int)songs.size(); ++i) {
        bool current = cs && (songs[i] == *cs);
        if (current) {
            std::cout << BOLD FG_GREEN "  \xe2\x96\xb6 " << i + 1 << ". "
                      << songs[i].getTitle() << "  "
                      << DIM FG_GREEN << songs[i].getArtist()
                      << "  [" << songs[i].getFormattedTime() << "]"
                      << RESET "\n";
        } else {
            std::cout << FG_WHITE "    " << i + 1 << ". "
                      << songs[i].getTitle() << "  "
                      << DIM FG_WHITE << songs[i].getArtist()
                      << "  [" << songs[i].getFormattedTime() << "]"
                      << RESET "\n";
        }
    }
}

static void drawPlaylistList(const MusicPlayer& player) {
    const auto& pls = player.getPlaylists();
    const Playlist* cur = player.getCurrentPlaylist();

    printLine();
    std::cout << BOLD FG_CYAN "  ALL PLAYLISTS\n" RESET;
    printLine();

    if (pls.empty()) {
        std::cout << DIM "  No playlists yet. Use 'create <name>'.\n" RESET;
    } else {
        for (int i = 0; i < (int)pls.size(); ++i) {
            bool active = cur && cur->getName() == pls[i].getName();
            if (active)
                std::cout << BOLD FG_MAGENTA "  * " << i+1 << ". " << pls[i].getName()
                          << DIM "  (" << pls[i].getSongs().size() << " songs)" RESET "\n";
            else
                std::cout << FG_WHITE "    " << i+1 << ". " << pls[i].getName()
                          << DIM "  (" << pls[i].getSongs().size() << " songs)" RESET "\n";
        }
    }
    printLine();
}

static void drawHelp() {
    printLine();
    std::cout << BOLD FG_CYAN "  COMMANDS\n" RESET;
    printLine();

    auto row = [](const std::string& cmd, const std::string& desc) {
        std::cout << "  " BOLD FG_GREEN << cmd << RESET;
        int pad = 22 - (int)cmd.size();
        for (int i = 0; i < pad; ++i) std::cout << ' ';
        std::cout << FG_WHITE << desc << RESET "\n";
    };

    std::cout << DIM FG_CYAN "  Playback\n" RESET;
    row("play <n>",     "Play song number n from current playlist");
    row("pause",        "Pause playback");
    row("resume",       "Resume playback");
    row("stop",         "Stop playback");
    row("next",         "Next song");
    row("prev",         "Previous song");

    std::cout << DIM FG_CYAN "\n  Playlists\n" RESET;
    row("list",         "Show all playlists");
    row("create <n>",   "Create a new playlist");
    row("delete <n>",   "Delete a playlist");
    row("switch <n>",   "Switch active playlist");
    row("show",         "Show songs in current playlist");
    row("add",          "Add a song");
    row("import <dir>", "Import all mp3/wav from a folder");
    row("remove <n>",   "Remove song number n");

    std::cout << DIM FG_CYAN "\n  Other\n" RESET;
    row("save",         "Save data to file");
    row("load",         "Load data from file");
    row("help",         "Show this screen");
    row("exit",         "Save & quit");
    printLine();
}

static void redraw(const MusicPlayer& player) {
    clearScreen();
    drawHeader();
    drawNowPlaying(player);
    std::cout << "\n";
    drawPlaylist(player);
    std::cout << "\n";
    std::cout << DIM FG_CYAN "  Type 'help' for all commands.\n" RESET;
    printLine();
    std::cout << BOLD FG_MAGENTA "  rythmify" RESET FG_CYAN " > " RESET;
    std::cout.flush();
}

static void addSongWizard(MusicPlayer& player) {
    clearScreen();
    drawHeader();
    printLine();
    printCentered("ADD NEW SONG", BOLD FG_CYAN);
    printLine();

    auto ask = [](const std::string& label) -> std::string {
        std::cout << "  " BOLD FG_GREEN << label << RESET FG_WHITE ": " RESET;
        std::string s;
        std::getline(std::cin, s);
        return s;
    };

    std::string path   = ask("File path (.mp3 / .wav)");
    std::string title  = ask("Title");
    std::string artist = ask("Artist");
    std::string album  = ask("Album");
    std::string genre  = ask("Genre");

    int dur = 0;
    while (true) {
        std::string d = ask("Duration (seconds)");
        try { dur = std::stoi(d); break; }
        catch (...) { std::cout << "  " FG_RED "Please enter a valid number.\n" RESET; }
    }

    Song s(path, title, artist, album, genre, dur);
    player.addSongToCurrentPlaylist(s);

    std::cout << "\n  " FG_GREEN "Song added! Press Enter to continue..." RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int main() {
    enableANSI();
    hideCursor();

    MusicPlayer player;

    {
        std::ifstream check(SAVE_FILE);
        if (check.good()) {
            check.close();
            player.loadFromFile(SAVE_FILE);
        }
    }

    bool showingHelp = false;
    bool showingList = false;

    std::string cmd;
    while (true) {
        if (player.checkIfSongEnded())
            player.next();

        if (showingHelp) {
            clearScreen();
            drawHeader();
            drawHelp();
            std::cout << BOLD FG_MAGENTA "  rythmify" RESET FG_CYAN " > " RESET;
            showingHelp = false;
        } else if (showingList) {
            clearScreen();
            drawHeader();
            drawPlaylistList(player);
            std::cout << BOLD FG_MAGENTA "  rythmify" RESET FG_CYAN " > " RESET;
            showingList = false;
        } else {
            redraw(player);
        }

        if (!std::getline(std::cin, cmd)) break;

        while (!cmd.empty() && cmd[0] == ' ') cmd.erase(0, 1);
        if (cmd.empty()) continue;

        std::string verb, arg;
        auto space = cmd.find(' ');
        if (space == std::string::npos) { verb = cmd; }
        else { verb = cmd.substr(0, space); arg = cmd.substr(space + 1); }

        if (verb == "play") {
            if (arg.empty()) {
                const Playlist* pl = player.getCurrentPlaylist();
                if (pl && !pl->getSongs().empty())
                    player.play(pl->getSongs()[0]);
                else
                    std::cout << FG_RED "  No playlist/song selected.\n" RESET;
            } else {
                try {
                    int idx = std::stoi(arg) - 1;
                    const Playlist* pl = player.getCurrentPlaylist();
                    if (!pl || (int)pl->getSongs().size() == 0)
                        std::cout << FG_RED "  No playlist selected.\n" RESET;
                    else if (idx < 0 || idx >= (int)pl->getSongs().size())
                        std::cout << FG_RED "  Invalid song number.\n" RESET;
                    else
                        player.play(pl->getSongs()[idx]);
                } catch (...) { std::cout << FG_RED "  Invalid number.\n" RESET; }
            }
        }
        else if (verb == "pause")  { player.pause(); }
        else if (verb == "resume") { player.resume(); }
        else if (verb == "stop")   { player.stop(); }
        else if (verb == "next")   { player.next(); }
        else if (verb == "prev")   { player.previous(); }
        else if (verb == "list")   { showingList = true; }
        else if (verb == "create") {
            std::string name = arg;
            if (name.empty()) { std::cout << "  Name: "; std::getline(std::cin, name); }
            player.createPlaylist(name);
        }
        else if (verb == "delete") {
            std::string name = arg;
            if (name.empty()) { std::cout << "  Name to delete: "; std::getline(std::cin, name); }
            player.deletePlaylist(name);
        }
        else if (verb == "switch") {
            std::string name = arg;
            if (name.empty()) { std::cout << "  Playlist name: "; std::getline(std::cin, name); }
            player.switchPlaylist(name);
        }
        else if (verb == "show")   { }
        else if (verb == "add")    { addSongWizard(player); }
        else if (verb == "import") {
            std::string folder = arg;
            if (folder.empty()) {
                std::cout << "  Folder path: ";
                std::getline(std::cin, folder);
            }
            player.importFolder(folder);
            std::cout << "\n  Press Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else if (verb == "remove") {
            try {
                int idx = std::stoi(arg) - 1;
                const Playlist* pl = player.getCurrentPlaylist();
                if (pl && idx >= 0 && idx < (int)pl->getSongs().size())
                    player.removeSongFromCurrentPlaylist(pl->getSongs()[idx]);
                else
                    std::cout << FG_RED "  Invalid number.\n" RESET;
            } catch (...) { std::cout << FG_RED "  Usage: remove <number>\n" RESET; }
        }
        else if (verb == "save") { player.saveToFile(SAVE_FILE); Sleep(600); }
        else if (verb == "load") { player.loadFromFile(SAVE_FILE); Sleep(600); }
        else if (verb == "help")  { showingHelp = true; }
        else if (verb == "exit" || verb == "quit") {
            player.saveToFile(SAVE_FILE);
            clearScreen();
            printCentered("Goodbye!", BOLD FG_MAGENTA);
            showCursor();
            break;
        }
        else {
            std::cout << FG_RED "  Unknown command: '" << verb << "'. Type 'help'.\n" RESET;
            Sleep(800);
        }
    }

    showCursor();
    return 0;
}
