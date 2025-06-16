cmake_minimum_required(VERSION 3.10)
project(login_app)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

find_package(SQLite3 REQUIRED)

include(FetchContent)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

if(WIN32)
  add_definitions(-DNOMINMAX)
  if(MINGW)
    add_definitions(-DCURL_STATICLIB=0)
  endif()
endif()

add_executable(ss4us 
    main.cpp 
    login.cpp 
    setting.cpp
    list.cpp
    config.cpp
    FileWatcher.cpp
)

if(WIN32)
  target_link_libraries(ss4us PRIVATE ${CURL_LIBRARIES} SQLite::SQLite3 nlohmann_json::nlohmann_json ws2_32)
else()
  target_link_libraries(ss4us PRIVATE ${CURL_LIBRARIES} SQLite::SQLite3 nlohmann_json::nlohmann_json)
endif()