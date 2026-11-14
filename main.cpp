#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

struct Channel {
    std::string name;
    std::string url;
};

bool isRemote(std::string_view value) {
    return value.starts_with("http://") || value.starts_with("https://");
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::optional<std::string> downloadPlaylist(const std::string& url) {
    int pipeFds[2];
    if (pipe(pipeFds) == -1) {
        std::cerr << "Could not create download pipe: " << std::strerror(errno) << '\n';
        return std::nullopt;
    }

    const pid_t child = fork();
    if (child == -1) {
        std::cerr << "Could not start curl: " << std::strerror(errno) << '\n';
        close(pipeFds[0]);
        close(pipeFds[1]);
        return std::nullopt;
    }
    if (child == 0) {
        close(pipeFds[0]);
        dup2(pipeFds[1], STDOUT_FILENO);
        close(pipeFds[1]);
        execlp("curl", "curl", "--fail", "--silent", "--show-error", "--location",
               "--", url.c_str(), static_cast<char*>(nullptr));
        std::cerr << "Could not start curl: " << std::strerror(errno) << '\n';
        _exit(127);
    }

    close(pipeFds[1]);
    std::string content;
    char buffer[8192];
    ssize_t bytesRead;
    while ((bytesRead = read(pipeFds[0], buffer, sizeof(buffer))) > 0) {
        content.append(buffer, static_cast<std::size_t>(bytesRead));
    }
    close(pipeFds[0]);

    int status = 0;
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            std::cerr << "Could not wait for curl: " << std::strerror(errno) << '\n';
            return std::nullopt;
        }
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::cerr << "Playlist download failed.\n";
        return std::nullopt;
    }
    return content;
}

std::string resolveRemoteEntry(const std::string& playlistUrl, const std::string& entry) {
    if (entry.starts_with("//")) {
        const auto schemeEnd = playlistUrl.find("://");
        return (schemeEnd == std::string::npos ? "https:" : playlistUrl.substr(0, schemeEnd + 1)) + entry;
    }
    if (entry.find("://") != std::string::npos) {
        return entry;
    }

    const auto queryStart = playlistUrl.find_first_of("?#");
    const std::string baseUrl = playlistUrl.substr(0, queryStart);
    const auto authorityStart = baseUrl.find("://");
    if (authorityStart == std::string::npos) {
        return entry;
    }
    const auto pathStart = baseUrl.find('/', authorityStart + 3);
    const std::string origin = pathStart == std::string::npos
                                   ? baseUrl
                                   : baseUrl.substr(0, pathStart);
    if (entry.starts_with('/')) {
        return origin + entry;
    }
    const auto lastSlash = baseUrl.find_last_of('/');
    const std::string directory = lastSlash == std::string::npos || lastSlash < authorityStart + 3
                                     ? baseUrl + "/"
                                     : baseUrl.substr(0, lastSlash + 1);
    return directory + entry;
}

std::string channelNameFromUrl(const std::string& url) {
    const auto end = url.find_first_of("?#");
    const std::string withoutQuery = url.substr(0, end);
    const auto slash = withoutQuery.find_last_of('/');
    if (slash != std::string::npos && slash + 1 < withoutQuery.size()) {
        return withoutQuery.substr(slash + 1);
    }
    return url;
}

std::vector<Channel> parsePlaylist(const std::string& content, const std::string& source) {
    std::vector<Channel> channels;
    std::string pendingName;
    std::istringstream lines(content);
    std::string line;

    while (std::getline(lines, line)) {
        line = trim(std::move(line));
        if (line.empty()) {
            continue;
        }
        if (line.starts_with("#EXTINF:")) {
            bool insideQuotes = false;
            std::size_t comma = std::string::npos;
            for (std::size_t index = 0; index < line.size(); ++index) {
                if (line[index] == '"') {
                    insideQuotes = !insideQuotes;
                } else if (line[index] == ',' && !insideQuotes) {
                    comma = index;
                    break;
                }
            }
            pendingName = comma == std::string::npos ? "Unnamed channel" : trim(line.substr(comma + 1));
            if (pendingName.empty()) {
                pendingName = "Unnamed channel";
            }
            continue;
        }
        if (line.front() == '#') {
            continue;
        }

        std::string resolvedUrl = line;
        if (isRemote(source)) {
            resolvedUrl = resolveRemoteEntry(source, line);
        } else if (!line.starts_with("/") && line.find("://") == std::string::npos) {
            resolvedUrl = (std::filesystem::path(source).parent_path() / line).lexically_normal().string();
        }
        channels.push_back({pendingName.empty() ? channelNameFromUrl(line) : pendingName,
                            std::move(resolvedUrl)});
        pendingName.clear();
    }
    return channels;
}

bool runProgram(const std::string& program, const std::vector<std::string>& arguments) {
    const pid_t child = fork();
    if (child == -1) {
        std::cerr << "Could not start " << program << ": " << std::strerror(errno) << '\n';
        return false;
    }
    if (child == 0) {
        std::vector<char*> argv;
        argv.reserve(arguments.size() + 2);
        argv.push_back(const_cast<char*>(program.c_str()));
        for (const auto& argument : arguments) {
            argv.push_back(const_cast<char*>(argument.c_str()));
        }
        argv.push_back(nullptr);
        execvp(program.c_str(), argv.data());
        std::cerr << "Could not run " << program << ": " << std::strerror(errno) << '\n';
        _exit(127);
    }

    int status = 0;
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            std::cerr << "Could not wait for " << program << ": " << std::strerror(errno) << '\n';
            return false;
        }
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
        return false;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::cerr << program << " exited unsuccessfully.\n";
        return false;
    }
    return true;
}

std::optional<std::filesystem::path> findExecutablePath(const std::string& argv0) {
    const std::filesystem::path suppliedPath(argv0);
    std::error_code error;
    if (suppliedPath.has_parent_path() || suppliedPath.is_absolute()) {
        const auto absolutePath = std::filesystem::absolute(suppliedPath, error);
        if (!error && access(absolutePath.c_str(), X_OK) == 0) {
            return absolutePath;
        }
    } else if (const char* pathValue = std::getenv("PATH")) {
        std::istringstream paths(pathValue);
        std::string directory;
        while (std::getline(paths, directory, ':')) {
            const auto candidate = std::filesystem::path(directory.empty() ? "." : directory) / suppliedPath;
            if (access(candidate.c_str(), X_OK) == 0) {
                const auto absolutePath = std::filesystem::absolute(candidate, error);
                if (!error) {
                    return absolutePath;
                }
            }
        }
    }
    return std::nullopt;
}

std::string mediaToolPath(const char* overrideVariable, const char* toolName,
                          const std::optional<std::filesystem::path>& executablePath) {
    if (const char* overridePath = std::getenv(overrideVariable)) {
        return overridePath;
    }
    if (executablePath) {
        const auto bundledTool = executablePath->parent_path() / toolName;
        if (access(bundledTool.c_str(), X_OK) == 0) {
            return bundledTool.string();
        }
    }
    return toolName;
}

std::optional<std::size_t> parseChannelIndex(const std::string& value, std::size_t channelCount) {
    std::size_t number = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
        number == 0 || number > channelCount) {
        return std::nullopt;
    }
    return number - 1;
}

void printHelp() {
    std::cout << "Commands:\n"
                 "  list                         Show channels\n"
                 "  play <channel-number>        Play a channel (press q in ffplay to stop)\n"
                 "  record <number> <seconds> [file.ts]\n"
                 "                               Record a channel for a duration\n"
                 "  help                         Show commands\n"
                 "  quit                         Exit\n";
}

bool loadChannels(const std::string& source, std::vector<Channel>& channels) {
    std::string content;
    if (isRemote(source)) {
        auto downloaded = downloadPlaylist(source);
        if (!downloaded) {
            return false;
        }
        content = std::move(*downloaded);
    } else {
        std::ifstream file(source);
        if (!file) {
            std::cerr << "Could not open playlist: " << source << '\n';
            return false;
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();
    }

    channels = parsePlaylist(content, source);
    if (channels.empty()) {
        std::cerr << "No channels found. Use an extended M3U playlist with #EXTINF entries.\n";
        return false;
    }
    std::cout << "Loaded " << channels.size() << " channels.\n";
    return true;
}

} // namespace

#ifdef __APPLE__
int launchTerminalPlayer();
#endif

int main(int argc, char* argv[]) {
#ifdef __APPLE__
    if (argc == 1 || (argc == 2 && std::string_view(argv[1]).starts_with("-psn_"))) {
        return launchTerminalPlayer();
    }
#endif
    if (argc != 2) {
        std::cerr << "Usage: iptv-player <playlist.m3u|playlist.m3u8|https://...>\n";
        return 2;
    }

    const std::string source = argv[1];
    std::vector<Channel> channels;
    if (!loadChannels(source, channels)) {
        return 1;
    }

    const auto executablePath = findExecutablePath(argv[0]);
    const std::string ffplay = mediaToolPath("IPTV_FFPLAY", "ffplay", executablePath);
    const std::string ffmpeg = mediaToolPath("IPTV_FFMPEG", "ffmpeg", executablePath);

    printHelp();
    std::string commandLine;
    while (std::cout << "\niptv> " && std::getline(std::cin, commandLine)) {
        std::istringstream input(commandLine);
        std::string command;
        input >> command;

        if (command == "quit" || command == "exit") {
            break;
        }
        if (command == "help") {
            printHelp();
            continue;
        }
        if (command == "list") {
            for (std::size_t index = 0; index < channels.size(); ++index) {
                std::cout << index + 1 << ". " << channels[index].name << '\n';
            }
            continue;
        }
        if (command == "play" || command == "record") {
            std::string numberText;
            input >> numberText;
            const auto channelIndex = parseChannelIndex(numberText, channels.size());
            if (!channelIndex) {
                std::cerr << "Enter a channel number from 1 to " << channels.size() << ".\n";
                continue;
            }

            const Channel& channel = channels[*channelIndex];
            if (command == "play") {
                std::cout << "Playing " << channel.name << " (press q in ffplay to stop).\n";
                runProgram(ffplay, {"-window_title", channel.name, channel.url});
                continue;
            }

            std::string secondsText;
            input >> secondsText;
            unsigned seconds = 0;
            const auto parsedSeconds =
                std::from_chars(secondsText.data(), secondsText.data() + secondsText.size(), seconds);
            if (parsedSeconds.ec != std::errc{} ||
                parsedSeconds.ptr != secondsText.data() + secondsText.size() || seconds == 0) {
                std::cerr << "Recording duration must be a positive number of seconds.\n";
                continue;
            }

            std::string outputPath;
            if (!(input >> std::quoted(outputPath))) {
                outputPath = "recording.ts";
            }
            std::cout << "Recording " << channel.name << " to " << outputPath << " for "
                      << seconds << " seconds.\n";
            runProgram(ffmpeg, {"-hide_banner", "-n", "-i", channel.url, "-t",
                                std::to_string(seconds), "-c", "copy", outputPath});
            continue;
        }
        if (!command.empty()) {
            std::cerr << "Unknown command. Type 'help' for available commands.\n";
        }
    }
    return 0;
}
