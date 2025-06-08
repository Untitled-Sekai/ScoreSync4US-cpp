#ifndef LOGIN_H
#define LOGIN_H

#include <string>

size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);

std::string login(const std::string& username, const std::string& password);

#endif // LOGIN_H