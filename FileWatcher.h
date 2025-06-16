#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <string>
#include <functional>
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <atomic>
#include <mutex>

enum class FileStatus
{
    Created,
    Modified,
    Deleted
};

using FileCallback = std::function<void(const std::string &, const std::string &, FileStatus, const std::string &)>;

class FileWatcher
{
public:
    FileWatcher(const std::string &path_to_watch);

    ~FileWatcher();

    void start(const FileCallback &callback);

    void stop();

    void addExtensionToWatch(const std::string &extension);

private:
    std::string watch_path_;

    std::unordered_map<std::string, std::filesystem::file_time_type> files_;

    std::vector<std::string> extensions_to_watch_;

    std::atomic<bool> running_{false};

    std::thread watch_thread_;

    std::mutex mutex_;

    const std::chrono::milliseconds delay_{1000};

    void initFiles();

    void watch(const FileCallback &callback);

    bool isWatchedExtension(const std::string &path);
};

#endif // FILE_WATCHER_H