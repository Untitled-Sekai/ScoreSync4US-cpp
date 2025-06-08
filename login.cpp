#include "login.h"
#include "setting.h"
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string login(const std::string& username, const std::string& password) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    
    curl = curl_easy_init();
    if (!curl) {
        std::cerr << "error: curl initialization failed" << std::endl;
        return "";
    }
    
    std::string loginUrl = Settings::getLoginUrl();
    curl_easy_setopt(curl, CURLOPT_URL, loginUrl.c_str());
    
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    json payload = {
        {"username", username},
        {"password", password}
    };
    std::string payloadStr = payload.dump();
    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    
    res = curl_easy_perform(curl);
    
    std::string token = "";
    
    if (res != CURLE_OK) {
        std::cerr << "error: " << curl_easy_strerror(res) << std::endl;
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code == 200) {
            try {
                json response = json::parse(readBuffer);
                if (response.contains("token")) {
                    token = response["token"];
                    std::cout << "token: " << token << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "error: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "failed: " << http_code << std::endl;
            std::cerr << "error: " << readBuffer << std::endl;
        }
    }
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    return token;
}