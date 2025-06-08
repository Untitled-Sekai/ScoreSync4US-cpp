#include "list.h"
#include "setting.h"
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

size_t ChartWriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp)
{
    userp->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::vector<ChartData> getUserCharts(const std::string &token, const std::string &username)
{
    std::vector<ChartData> charts;
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    
    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "error: curl initialization failed" << std::endl;
        return charts;
    }

    
    std::string url = Settings::BASE_URL + "/api/charts/user/" + username;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    
    std::string authHeader = "Authorization: Bearer " + token;
    headers = curl_slist_append(headers, authHeader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ChartWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    
    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "error: " << curl_easy_strerror(res) << std::endl;
    }
    else
    {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == 200)
        {
            try
            {
                json response = json::parse(readBuffer);

                if (response.contains("data") && response["data"].is_array())
                {
                    json allCharts = response["data"];

                    
                    for (const auto &level : allCharts)
                    {
                        ChartData chart;

                        
                        chart.name = level.value("name", "");

                        
                        if (level.contains("title"))
                        {
                            if (level["title"].is_object())
                            {
                                
                                if (level["title"].contains("ja") && !level["title"]["ja"].is_null())
                                {
                                    chart.title = level["title"]["ja"].get<std::string>();
                                }
                                else if (level["title"].contains("en") && !level["title"]["en"].is_null())
                                {
                                    chart.title = level["title"]["en"].get<std::string>();
                                }
                                else
                                {
                                    chart.title = "No Title";
                                }
                            }
                            else if (level["title"].is_string())
                            {
                                
                                chart.title = level["title"].get<std::string>();
                            }
                            else
                            {
                                chart.title = "No Title";
                            }
                        }
                        else
                        {
                            chart.title = "No Title";
                        }

                        
                        if (level.contains("artists"))
                        {
                            if (level["artists"].is_object())
                            {
                                if (level["artists"].contains("ja") && !level["artists"]["ja"].is_null())
                                {
                                    chart.artist = level["artists"]["ja"].get<std::string>();
                                }
                                else if (level["artists"].contains("en") && !level["artists"]["en"].is_null())
                                {
                                    chart.artist = level["artists"]["en"].get<std::string>();
                                }
                                else
                                {
                                    chart.artist = "Unknown";
                                }
                            }
                            else if (level["artists"].is_string())
                            {
                                chart.artist = level["artists"].get<std::string>();
                            }
                            else
                            {
                                chart.artist = "Unknown";
                            }
                        }
                        else if (level.contains("artist"))
                        { 
                            if (level["artist"].is_object())
                            {
                                if (level["artist"].contains("ja") && !level["artist"]["ja"].is_null())
                                {
                                    chart.artist = level["artist"]["ja"].get<std::string>();
                                }
                                else if (level["artist"].contains("en") && !level["artist"]["en"].is_null())
                                {
                                    chart.artist = level["artist"]["en"].get<std::string>();
                                }
                                else
                                {
                                    chart.artist = "Unknown";
                                }
                            }
                            else if (level["artist"].is_string())
                            {
                                chart.artist = level["artist"].get<std::string>();
                            }
                            else
                            {
                                chart.artist = "Unknown";
                            }
                        }
                        else
                        {
                            chart.artist = "Unknown";
                        }

                        
                        chart.rating = level.value("rating", 0);

                        
                        chart.uploadDate = level.value("createdAt", "");

                        
                        if (level.contains("cover") && level["cover"].is_object() && level["cover"].contains("url"))
                        {
                            chart.coverUrl = level["cover"]["url"];
                        }
                        else
                        {
                            chart.coverUrl = "";
                        }

                        
                        if (level.contains("description") && level["description"].is_object())
                        {
                            if (level["description"].contains("ja") && !level["description"]["ja"].is_null())
                            {
                                chart.description = level["description"]["ja"];
                            }
                            else if (level["description"].contains("en") && !level["description"]["en"].is_null())
                            {
                                chart.description = level["description"]["en"];
                            }
                            else
                            {
                                chart.description = "";
                            }
                        }
                        else
                        {
                            chart.description = "";
                        }

                        if (level.contains("tags"))
                        {
                            if (level["tags"].is_array())
                            {
                                for (const auto &tag : level["tags"])
                                {
                                    try
                                    {
                                        if (tag.is_string())
                                        {
                                            chart.tags.push_back(tag.get<std::string>());
                                        }
                                        else if (tag.is_object())
                                        {
                                            if (tag.contains("name") && tag["name"].is_string())
                                            {
                                                chart.tags.push_back(tag["name"].get<std::string>());
                                            }
                                            else
                                            {
                                                
                                                for (auto &[key, value] : tag.items())
                                                {
                                                    if (value.is_string())
                                                    {
                                                        chart.tags.push_back(value.get<std::string>());
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    catch (const std::exception &e)
                                    {
                                        std::cerr << "タグ処理中のエラー: " << e.what() << std::endl;
                                    }
                                }
                            }
                            else if (level["tags"].is_string())
                            {
                                
                                chart.tags.push_back(level["tags"].get<std::string>());
                            }
                        }

                        
                        if (level.contains("meta") && level["meta"].is_object())
                        {
                            
                            chart.isPublic = level["meta"].value("isPublic", false);

                            
                            if (level["meta"].contains("collaboration") && level["meta"]["collaboration"].is_object())
                            {
                                chart.isCollaboration = level["meta"]["collaboration"].value("iscollaboration", false);
                            }
                            else
                            {
                                chart.isCollaboration = false;
                            }

                            
                            if (level["meta"].contains("privateShare") && level["meta"]["privateShare"].is_object())
                            {
                                chart.isPrivateShare = level["meta"]["privateShare"].value("isPrivateShare", false);
                            }
                            else
                            {
                                chart.isPrivateShare = false;
                            }
                        }
                        else
                        {
                            chart.isPublic = false;
                            chart.isCollaboration = false;
                            chart.isPrivateShare = false;
                        }

                        
                        
                        
                        chart.isCollab = false;
                        chart.isPrivateShared = false;

                        
                        charts.push_back(chart);
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "error parsing response: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cerr << "ユーザー譜面取得失敗: " << http_code << std::endl;
            std::cerr << "エラー: " << readBuffer << std::endl;
        }
    }

    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return charts;
}

json formatChartsData(const std::vector<ChartData> &charts)
{
    json charts_data = json::array();

    for (const auto &chart : charts)
    {
        json chartJson = {
            {"name", chart.name},
            {"title", chart.title},
            {"artist", chart.artist},
            {"author", chart.author},
            {"rating", chart.rating},
            {"uploadDate", chart.uploadDate},
            {"coverUrl", chart.coverUrl},
            {"description", chart.description},
            {"tags", chart.tags},
            {"meta", {{"isPublic", chart.isPublic}, {"collaboration", {{"iscollaboration", chart.isCollaboration}}}, {"privateShare", {{"isPrivateShare", chart.isPrivateShare}}}}},
            {"isCollab", chart.isCollab},
            {"isPrivateShared", chart.isPrivateShared}};

        charts_data.push_back(chartJson);
    }

    
    json response = {
        {"success", true},
        {"data", charts_data}};

    return response;
}