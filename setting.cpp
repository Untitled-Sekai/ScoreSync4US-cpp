#include "setting.h"

namespace Settings {
    const std::string BASE_URL = "https://us.pim4n-net.com";
    const std::string API_LOGIN_PATH = "/api/login";
    
    std::string getLoginUrl() {
        return BASE_URL + API_LOGIN_PATH;
    }
}