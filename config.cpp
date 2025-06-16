#include "config.h"
#include "FileWatcher.h"
#include "setting.h"
#include <iostream>
#include <sqlite3.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <chrono>
#include <regex>
#include <algorithm>
#include <thread>
#include <fstream>
#include <curl/curl.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

const std::string DB_FILE = "config.db";
const std::string LEVELS_FOLDER = "levels";
const std::string TOKEN_FILE = "token.txt";

bool isFirstRun()
{
    return !fs::exists(DB_FILE);
}

void initDatabase()
{
    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "can not open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    const char *sqlCreateUserTable =
        "CREATE TABLE IF NOT EXISTS users ("
        "username TEXT PRIMARY KEY,"
        "last_login TEXT"
        ");";

    rc = sqlite3_exec(db, sqlCreateUserTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    const char *sqlCreateChartTable =
        "CREATE TABLE IF NOT EXISTS charts ("
        "username TEXT,"
        "name TEXT,"
        "title TEXT,"
        "PRIMARY KEY (username, name),"
        "FOREIGN KEY (username) REFERENCES users(username)"
        ");";

    rc = sqlite3_exec(db, sqlCreateChartTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    sqlite3_close(db);
}

std::string createSafeFolderName(const std::string &title)
{
    std::string safeName = title;

    std::regex invalid("[\\\\/:*?\"<>|.%$&+=;,]");
    safeName = std::regex_replace(safeName, invalid, "_");

    safeName.erase(0, safeName.find_first_not_of(" \t\n\r"));
    safeName.erase(safeName.find_last_not_of(" \t\n\r") + 1);

    std::regex multipleUnderscores("_+");
    safeName = std::regex_replace(safeName, multipleUnderscores, "_");

    if (!safeName.empty() && safeName.front() == '_')
        safeName.erase(0, 1);
    if (!safeName.empty() && safeName.back() == '_')
        safeName.pop_back();

    if (safeName.empty())
    {
        safeName = "unnamed_chart";
    }

    if (safeName.length() > 50)
    {
        safeName = safeName.substr(0, 47) + "...";
    }

    static const std::vector<std::string> reservedNames = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

    std::string upperName = safeName;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

    for (const auto &reserved : reservedNames)
    {
        if (upperName == reserved || upperName.substr(0, reserved.length() + 1) == reserved + ".")
        {
            safeName = "chart_" + safeName;
            break;
        }
    }

    return safeName;
}

void initChartInfoDatabase(const std::string &folderPath, const std::string &chartId)
{
    std::string dbPath = folderPath + "/info.db";

    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "can not open info.db: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    const char *sqlCreateInfoTable =
        "CREATE TABLE IF NOT EXISTS chart_info ("
        "id TEXT PRIMARY KEY,"
        "mp3_file TEXT,"
        "png_file TEXT,"
        "sus_file TEXT,"
        "usc_file TEXT"
        ");";

    rc = sqlite3_exec(db, sqlCreateInfoTable, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "chart info table error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    std::string sqlInsertInfo = "INSERT OR REPLACE INTO chart_info (id, mp3_file, png_file, sus_file, usc_file) "
                                "VALUES ('" +
                                chartId + "', '', '', '', '');";

    rc = sqlite3_exec(db, sqlInsertInfo.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "chart info error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    sqlite3_close(db);
    std::cout << "create chart info: " << dbPath << std::endl;
}

void createLevelsFolderStructure(const std::vector<ChartData> &charts)
{

    if (!fs::exists(LEVELS_FOLDER))
    {
        try
        {
            fs::create_directory(LEVELS_FOLDER);
            std::cout << "create levels folder" << std::endl;
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << "failed create levels folder: " << e.what() << std::endl;
            return;
        }
    }

    for (const auto &chart : charts)
    {
        std::string safeTitleName = createSafeFolderName(chart.title);
        std::string chartFolder = LEVELS_FOLDER + "/" + safeTitleName;

        try
        {

            if (!fs::exists(chartFolder))
            {
                fs::create_directory(chartFolder);
                std::cout << "create chart folder: " << chartFolder << std::endl;

                initChartInfoDatabase(chartFolder, chart.name);
            }
            else
            {

                if (!fs::exists(chartFolder + "/info.db"))
                {
                    initChartInfoDatabase(chartFolder, chart.name);
                }
            }
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << "failed create chart folder: " << e.what() << std::endl;
        }
    }
}

void saveUserCharts(const std::string &username, const std::vector<ChartData> &charts)
{
    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open(DB_FILE.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "can not open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    std::string currentTime = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::string sqlInsertUser = "INSERT OR REPLACE INTO users (username, last_login) VALUES ('" +
                                username + "', '" + currentTime + "');";

    rc = sqlite3_exec(db, sqlInsertUser.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    std::string sqlDeleteCharts = "DELETE FROM charts WHERE username = '" + username + "';";
    rc = sqlite3_exec(db, sqlDeleteCharts.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "failed delete chart: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    sqlite3_stmt *stmt;
    const char *sqlInsertChart = "INSERT INTO charts (username, name, title) VALUES (?, ?, ?);";

    rc = sqlite3_prepare_v2(db, sqlInsertChart, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        std::cerr << "faild statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    for (const auto &chart : charts)
    {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, chart.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, chart.title.c_str(), -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cerr << "failed input chart info: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    createLevelsFolderStructure(charts);
}

std::vector<ChartData> loadUserCharts(const std::string &username)
{
    std::vector<ChartData> charts;
    sqlite3 *db;
    int rc;

    rc = sqlite3_open(DB_FILE.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "can not open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return charts;
    }

    sqlite3_stmt *stmt;
    const char *sqlSelectCharts = "SELECT name, title FROM charts WHERE username = ?;";

    rc = sqlite3_prepare_v2(db, sqlSelectCharts, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        std::cerr << "failed statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return charts;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ChartData chart;
        const char *name = (const char *)sqlite3_column_text(stmt, 0);
        const char *title = (const char *)sqlite3_column_text(stmt, 1);

        if (name)
            chart.name = name;
        if (title)
            chart.title = title;

        chart.artist = "Unknown";
        chart.author = username;
        chart.rating = 0;
        chart.uploadDate = "";
        chart.coverUrl = "";
        chart.description = "";
        chart.isPublic = false;
        chart.isCollaboration = false;
        chart.isPrivateShare = false;
        chart.isCollab = false;
        chart.isPrivateShared = false;

        charts.push_back(chart);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return charts;
}

void updateChartFileInfo(const std::string &chartFolder, const std::string &fileName, const std::string &filePath, FileStatus status)
{
    std::string dbPath = LEVELS_FOLDER + "/" + chartFolder + "/info.db";

    if (!fs::exists(dbPath))
    {
        std::cerr << "database file is not found: " << dbPath << std::endl;
        return;
    }

    std::string fileType;
    std::string fileColumn;
    bool isChartFile = false;

    std::string extension = fileName.substr(fileName.find_last_of("."));
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == ".mp3")
    {
        fileColumn = "mp3_file";
    }
    else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
    {
        fileColumn = "png_file";
    }
    else if (extension == ".sus")
    {
        fileColumn = "sus_file";
        isChartFile = true;
    }
    else if (extension == ".usc")
    {
        fileColumn = "usc_file";
        isChartFile = true;
    }
    else
    {
        std::cerr << "This file is not support: " << extension << std::endl;
        return;
    }

    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "can not open database file: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    std::string chartId;
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, "SELECT id FROM chart_info LIMIT 1;", -1, &stmt, NULL);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *id = (const char *)sqlite3_column_text(stmt, 0);
        if (id)
            chartId = id;
    }
    sqlite3_finalize(stmt);

    std::string valueToSet;

    if (status == FileStatus::Deleted)
    {
        valueToSet = "";
    }
    else
    {
        valueToSet = fileName;
    }

    std::string sql = "UPDATE chart_info SET " + fileColumn + " = '" + valueToSet + "' WHERE id IN (SELECT id FROM chart_info LIMIT 1);";

    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "database update error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else
    {
        std::cout << "update chart info: " << fileColumn << " = " << valueToSet << std::endl;

        if (isChartFile && (status == FileStatus::Modified || status == FileStatus::Created) && !chartId.empty())
        {
            std::string token = getToken();
            if (!token.empty())
            {
                std::cout << "Uploading chart file to server: " << fileName << std::endl;
                if (updateChartFile(chartId, filePath))
                {
                    std::cout << "Chart file uploaded successfully!" << std::endl;
                }
                else
                {
                    std::cerr << "Failed to upload chart file." << std::endl;
                }
            }
            else
            {
                std::cerr << "No token available. Please login first to enable chart uploads." << std::endl;
            }
        }
    }

    sqlite3_close(db);
}

FileWatcher *g_fileWatcher = nullptr;

void initFileWatcher()
{
    if (g_fileWatcher != nullptr)
    {
        return;
    }

    if (!fs::exists(LEVELS_FOLDER))
    {
        try
        {
            fs::create_directory(LEVELS_FOLDER);
            std::cout << "create levels folder" << std::endl;
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << "failed create levels folder: " << e.what() << std::endl;
            return;
        }
    }

    g_fileWatcher = new FileWatcher(LEVELS_FOLDER);

    g_fileWatcher->addExtensionToWatch(".sus");
    g_fileWatcher->addExtensionToWatch(".usc");
    g_fileWatcher->addExtensionToWatch(".mp3");
    g_fileWatcher->addExtensionToWatch(".png");
    g_fileWatcher->addExtensionToWatch(".jpg");
    g_fileWatcher->addExtensionToWatch(".jpeg");

    g_fileWatcher->start([](const std::string &fileName, const std::string &filePath, FileStatus status, const std::string &parentDir)
                         {
        std::string statusStr;
        switch (status) {
            case FileStatus::Created:  statusStr = "create"; break;
            case FileStatus::Modified: statusStr = "change"; break;
            case FileStatus::Deleted:  statusStr = "delete"; break;
        }
        
        std::cout << "file " << statusStr << ": " << fileName 
                  << " (folder: " << parentDir << ")" << std::endl;
        
        
        updateChartFileInfo(parentDir, fileName, filePath, status); });

    std::cout << "start file watching" << std::endl;
}

void stopFileWatcher()
{
    if (g_fileWatcher != nullptr)
    {
        g_fileWatcher->stop();
        delete g_fileWatcher;
        g_fileWatcher = nullptr;
        std::cout << "stop file watching" << std::endl;
    }
}

void saveToken(const std::string &token)
{
    try
    {
        std::ofstream tokenFile(TOKEN_FILE);
        if (tokenFile.is_open())
        {
            tokenFile << token;
            tokenFile.close();
            std::cout << "Token saved" << std::endl;
        }
        else
        {
            std::cerr << "Failed to open token file for writing" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error saving token: " << e.what() << std::endl;
    }
}

std::string getToken()
{
    std::string token;
    try
    {
        std::ifstream tokenFile(TOKEN_FILE);
        if (tokenFile.is_open())
        {
            std::getline(tokenFile, token);
            tokenFile.close();
        }
        else
        {
            std::cerr << "Failed to open token file for reading" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error reading token: " << e.what() << std::endl;
    }
    return token;
}

std::string getChartInfoById(const std::string &chartId)
{
    std::string result;

    if (!fs::exists(LEVELS_FOLDER))
    {
        return "";
    }

    for (const auto &entry : fs::directory_iterator(LEVELS_FOLDER))
    {
        if (fs::is_directory(entry))
        {
            std::string dbPath = entry.path().string() + "/info.db";

            if (fs::exists(dbPath))
            {
                sqlite3 *db;
                int rc = sqlite3_open(dbPath.c_str(), &db);

                if (rc == SQLITE_OK)
                {
                    sqlite3_stmt *stmt;
                    std::string sql = "SELECT * FROM chart_info WHERE id = ?;";

                    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
                    if (rc == SQLITE_OK)
                    {
                        sqlite3_bind_text(stmt, 1, chartId.c_str(), -1, SQLITE_STATIC);

                        if (sqlite3_step(stmt) == SQLITE_ROW)
                        {

                            result = entry.path().string();
                            sqlite3_finalize(stmt);
                            sqlite3_close(db);
                            return result;
                        }

                        sqlite3_finalize(stmt);
                    }

                    sqlite3_close(db);
                }
            }
        }
    }

    return "";
}

std::vector<char> readFileAsBinary(const std::string &filePath)
{
    std::vector<char> buffer;
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        buffer.resize(fileSize);
        file.read(buffer.data(), fileSize);
        file.close();
    }
    else
    {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
    }

    return buffer;
}

size_t readCallback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    struct curl_callback_data
    {
        const char *data;
        size_t length;
        size_t pos;
    };

    curl_callback_data *callbackData = static_cast<curl_callback_data *>(userdata);
    size_t remaining = callbackData->length - callbackData->pos;
    size_t buffer_size = size * nitems;

    if (remaining == 0)
    {
        return 0;
    }

    size_t to_copy = (remaining < buffer_size) ? remaining : buffer_size;
    memcpy(buffer, callbackData->data + callbackData->pos, to_copy);
    callbackData->pos += to_copy;

    return to_copy;
}

bool updateChartFile(const std::string &chartId, const std::string &filePath)
{
    std::string token = getToken();
    if (token.empty())
    {
        std::cerr << "Token not available. Please login first." << std::endl;
        return false;
    }

    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "curl initialization failed" << std::endl;
        return false;
    }

    std::string editUrl = Settings::getChartEditUrl(chartId);
    curl_easy_setopt(curl, CURLOPT_URL, editUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    struct curl_slist *headers = NULL;
    std::string authHeader = "Authorization: Bearer " + token;
    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, "isOwner: true");

    std::vector<char> fileData = readFileAsBinary(filePath);
    if (fileData.empty())
    {
        std::cerr << "Failed to read chart file: " << filePath << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    curl_mime *mime = curl_mime_init(curl);
    curl_mimepart *part = curl_mime_addpart(mime);

    curl_mime_name(part, "chart");
    curl_mime_filedata(part, filePath.c_str());

    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ChartWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);

    bool success = false;
    if (res != CURLE_OK)
    {
        std::cerr << "Failed to update chart: " << curl_easy_strerror(res) << std::endl;
    }
    else
    {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == 200)
        {
            std::cout << "Chart updated successfully: " << chartId << std::endl;
            success = true;
        }
        else
        {
            std::cerr << "Failed to update chart. Status code: " << http_code << std::endl;
            std::cerr << "Response: " << readBuffer << std::endl;
        }
    }

    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return success;
}