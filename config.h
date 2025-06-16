#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include "list.h"
#include "FileWatcher.h"

bool isFirstRun();

void initDatabase();

void saveUserCharts(const std::string &username, const std::vector<ChartData> &charts);

std::vector<ChartData> loadUserCharts(const std::string &username);

void createLevelsFolderStructure(const std::vector<ChartData> &charts);

void initChartInfoDatabase(const std::string &folderPath, const std::string &chartId);

std::string createSafeFolderName(const std::string &title);

void initFileWatcher();

void stopFileWatcher();

void updateChartFileInfo(const std::string &chartFolder, const std::string &fileName, const std::string &filePath, FileStatus status);

void saveToken(const std::string& token);

std::string getToken();

std::string getChartInfoById(const std::string& chartId);

bool updateChartFile(const std::string& chartId, const std::string& filePath);

#endif // CONFIG_H