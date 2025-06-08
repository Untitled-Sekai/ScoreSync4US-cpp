#ifndef SETTING_H
#define SETTING_H

#include <string>

namespace Settings {
    extern const std::string BASE_URL;
    extern const std::string API_LOGIN_PATH;
    
    std::string getLoginUrl();
}

#endif // SETTING_H