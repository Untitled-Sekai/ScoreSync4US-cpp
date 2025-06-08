#ifndef LIST_H
#define LIST_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ChartData {
    std::string name;
    std::string title;
    std::string artist;
    std::string author;
    int rating;
    std::string uploadDate;
    std::string coverUrl;
    std::string description;
    std::vector<std::string> tags;
    bool isPublic;
    bool isCollaboration;
    bool isPrivateShare;
    bool isCollab;
    bool isPrivateShared;
};

std::vector<ChartData> getUserCharts(const std::string& token, const std::string& username);

json formatChartsData(const std::vector<ChartData>& charts);

size_t ChartWriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);

#endif // LIST_H