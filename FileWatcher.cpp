#include "FileWatcher.h"
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

FileWatcher::FileWatcher(const std::string &path_to_watch)
    : watch_path_(path_to_watch)
{

    extensions_to_watch_ = {".sus", ".usc", ".mp3", ".png"};
}

FileWatcher::~FileWatcher()
{
    stop();
}

void FileWatcher::start(const FileCallback &callback)
{
    if (running_)
        return;

    running_ = true;

    initFiles();

    watch_thread_ = std::thread(&FileWatcher::watch, this, callback);
}

void FileWatcher::stop()
{
    if (!running_)
        return;

    running_ = false;

    if (watch_thread_.joinable())
    {
        watch_thread_.join();
    }
}

void FileWatcher::addExtensionToWatch(const std::string &extension)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (std::find(extensions_to_watch_.begin(), extensions_to_watch_.end(), extension) == extensions_to_watch_.end())
    {
        extensions_to_watch_.push_back(extension);
    }
}

void FileWatcher::initFiles()
{
    std::lock_guard<std::mutex> lock(mutex_);
    files_.clear();

    if (!fs::exists(watch_path_))
        return;

    for (const auto &entry : fs::recursive_directory_iterator(watch_path_))
    {
        if (fs::is_regular_file(entry.status()))
        {
            std::string path = entry.path().string();

            if (isWatchedExtension(path))
            {

                files_[path] = fs::last_write_time(entry.path());
            }
        }
    }
}

bool FileWatcher::isWatchedExtension(const std::string &path)
{
    for (const auto &ext : extensions_to_watch_)
    {
        if (path.size() >= ext.size() &&
            path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
        {
            return true;
        }
    }
    return false;
}

void FileWatcher::watch(const FileCallback &callback)
{
    while (running_)
    {
        try
        {

            std::unordered_map<std::string, fs::file_time_type> current_files;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                current_files = files_;
            }

            if (!fs::exists(watch_path_))
            {
                std::this_thread::sleep_for(delay_);
                continue;
            }

            std::vector<std::string> to_remove;
            for (const auto &[path, last_write_time] : current_files)
            {
                if (!fs::exists(path))
                {

                    to_remove.push_back(path);

                    fs::path filepath(path);
                    std::string filename = filepath.filename().string();
                    std::string parent_dir = filepath.parent_path().filename().string();

                    callback(filename, path, FileStatus::Deleted, parent_dir);
                }
                else
                {

                    auto current_write_time = fs::last_write_time(path);
                    if (current_write_time != last_write_time)
                    {

                        fs::path filepath(path);
                        std::string filename = filepath.filename().string();
                        std::string parent_dir = filepath.parent_path().filename().string();

                        callback(filename, path, FileStatus::Modified, parent_dir);

                        std::lock_guard<std::mutex> lock(mutex_);
                        files_[path] = current_write_time;
                    }
                }
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (const auto &path : to_remove)
                {
                    files_.erase(path);
                }
            }

            for (const auto &entry : fs::recursive_directory_iterator(watch_path_))
            {
                if (fs::is_regular_file(entry.status()))
                {
                    std::string path = entry.path().string();

                    if (!isWatchedExtension(path))
                        continue;

                    if (current_files.find(path) == current_files.end())
                    {

                        fs::path filepath(path);
                        std::string filename = filepath.filename().string();
                        std::string parent_dir = filepath.parent_path().filename().string();

                        callback(filename, path, FileStatus::Created, parent_dir);

                        std::lock_guard<std::mutex> lock(mutex_);
                        files_[path] = fs::last_write_time(entry.path());
                    }
                }
            }

            std::this_thread::sleep_for(delay_);
        }
        catch (const std::exception &e)
        {
            std::cerr << "!error watch: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}