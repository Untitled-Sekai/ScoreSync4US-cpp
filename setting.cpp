#include "setting.h"

namespace Settings
{
    const std::string BASE_URL = "https://us.pim4n-net.com";
    const std::string API_LOGIN_PATH = "/api/login";
    const std::string API_CHART_EDIT_PATH = "/api/chart/edit/";

    std::string getLoginUrl()
    {
        return BASE_URL + API_LOGIN_PATH;
    }

    std::string getChartEditUrl(const std::string &chartId)
    {
        return BASE_URL + API_CHART_EDIT_PATH + chartId;
    }
}