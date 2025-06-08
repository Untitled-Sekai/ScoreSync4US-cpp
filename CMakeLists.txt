cmake_minimum_required(VERSION 3.10)
project(login_app)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

include(FetchContent)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

add_executable(login 
    main.cpp 
    login.cpp 
    setting.cpp
    list.cpp
)

target_link_libraries(login PRIVATE ${CURL_LIBRARIES} nlohmann_json::nlohmann_json)