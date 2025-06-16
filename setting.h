#ifndef SETTING_H
#define SETTING_H

#include <string>

namespace Settings {
    extern const std::string BASE_URL;
    extern const std::string API_LOGIN_PATH;
    extern const std::string API_CHART_EDIT_PATH;  
    
    std::string getLoginUrl();
    std::string getChartEditUrl(const std::string& chartId);  
}

#endif // SETTING_H