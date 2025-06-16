#include <iostream>
#include <string>
#include <curl/curl.h>
#include "login.h"
#include "setting.h"
#include "list.h"
#include "config.h"

#ifdef _WIN32
#include <windows.h>
void setConcoleUTF8()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}
#endif

int main()
{
#ifdef _WIN32
    setConcoleUTF8();
#endif

    curl_global_init(CURL_GLOBAL_ALL);

    bool firstRun = isFirstRun();
    if (firstRun)
    {
        initDatabase();
        std::cout << "init database\n";
    }

    initFileWatcher();

    std::string username, password;

    std::cout << "Enter username: ";
    std::getline(std::cin, username);

    std::cout << "Enter password: ";
    std::getline(std::cin, password);

    std::string token = login(username, password);

    if (!token.empty())
    {
        std::cout << "Login successful!" << std::endl;

        saveToken(token);

        if (firstRun)
        {
            std::cout << "\nloading\n";
            std::vector<ChartData> charts = getUserCharts(token, username);

            if (!charts.empty())
            {

                saveUserCharts(username, charts);
                std::cout << "\n"
                          << charts.size() << "saved databse\n";

                json formattedData = formatChartsData(charts);
                std::cout << "\nCharts: " << charts.size() << "\n\n";

                for (size_t i = 0; i < charts.size(); ++i)
                {
                    std::cout << i + 1 << ". " << charts[i].title
                              << " / " << charts[i].artist
                              << " (author: " << charts[i].author << ")\n";
                }

                int chartIndex = -1;
                std::cout << "\nEnter chart number to view details (0 to continue): ";
                std::cin >> chartIndex;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                if (chartIndex > 0 && chartIndex <= static_cast<int>(charts.size()))
                {
                    const ChartData &selectedChart = charts[chartIndex - 1];

                    std::cout << "\n==== Details ====\n";
                    std::cout << "ID: " << selectedChart.name << "\n";
                    std::cout << "Title: " << selectedChart.title << "\n";
                    std::cout << "Artist: " << selectedChart.artist << "\n";
                    std::cout << "Charter: " << selectedChart.author << "\n";
                    std::cout << "Difficulty: " << selectedChart.rating << "\n";
                    std::cout << "Upload date: " << selectedChart.uploadDate << "\n";
                    std::cout << "Description: " << selectedChart.description << "\n";

                    std::cout << "Tags: ";
                    for (const auto &tag : selectedChart.tags)
                    {
                        std::cout << tag << " ";
                    }
                    std::cout << "\n";

                    std::cout << "Public status: " << (selectedChart.isPublic ? "Public" : "Private") << "\n";
                    std::cout << "Collaboration: " << (selectedChart.isCollaboration ? "Yes" : "No") << "\n";
                    std::cout << "Private sharing: " << (selectedChart.isPrivateShare ? "Yes" : "No") << "\n";
                }
            }
            else
            {
                std::cout << "Chart is not found\n";
            }
        }

        int choice = 0;
        do
        {
            std::cout << "\n==== menu ====\n";
            std::cout << "1. Show my charts\n";
            std::cout << "2. Show another user's charts\n";
            std::cout << "0. Quit\n";
            std::cout << "Select: ";
            std::cin >> choice;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::string targetUsername;

            switch (choice)
            {
            case 1:
            {
                std::cout << "\nGetting my charts...\n";
                std::vector<ChartData> charts = getUserCharts(token, username);

                if (!charts.empty())
                {
                    json formattedData = formatChartsData(charts);
                    std::cout << "\nCharts: " << charts.size() << "\n\n";

                    for (size_t i = 0; i < charts.size(); ++i)
                    {
                        std::cout << i + 1 << ". " << charts[i].title
                                  << " / " << charts[i].artist
                                  << " (author: " << charts[i].author << ")\n";
                    }

                    int chartIndex = -1;
                    std::cout << "\nEnter chart number to view details (0 to go back): ";
                    std::cin >> chartIndex;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                    if (chartIndex > 0 && chartIndex <= static_cast<int>(charts.size()))
                    {
                        const ChartData &selectedChart = charts[chartIndex - 1];

                        std::cout << "\n==== Details ====\n";
                        std::cout << "ID: " << selectedChart.name << "\n";
                        std::cout << "Title: " << selectedChart.title << "\n";
                        std::cout << "Artist: " << selectedChart.artist << "\n";
                        std::cout << "Charter: " << selectedChart.author << "\n";
                        std::cout << "Difficulty: " << selectedChart.rating << "\n";
                        std::cout << "Upload date: " << selectedChart.uploadDate << "\n";
                        std::cout << "Description: " << selectedChart.description << "\n";

                        std::cout << "Tags: ";
                        for (const auto &tag : selectedChart.tags)
                        {
                            std::cout << tag << " ";
                        }
                        std::cout << "\n";

                        std::cout << "Public status: " << (selectedChart.isPublic ? "Public" : "Private") << "\n";
                        std::cout << "Collaboration: " << (selectedChart.isCollaboration ? "Yes" : "No") << "\n";
                        std::cout << "Private sharing: " << (selectedChart.isPrivateShare ? "Yes" : "No") << "\n";
                    }
                }
                else
                {
                    std::cout << "No charts found.\n";
                }
            }
            break;

            case 2:
                std::cout << "\nEnter username to display their charts: ";
                std::getline(std::cin, targetUsername);

                if (!targetUsername.empty())
                {
                    std::cout << "Retrieving " << targetUsername << "'s chart list...\n";
                    std::vector<ChartData> charts = getUserCharts(token, targetUsername);

                    if (!charts.empty())
                    {
                        json formattedData = formatChartsData(charts);
                        std::cout << "\nNumber of charts retrieved: " << charts.size() << "\n\n";

                        for (size_t i = 0; i < charts.size(); ++i)
                        {
                            std::cout << i + 1 << ". " << charts[i].title
                                      << " / " << charts[i].artist
                                      << " (author: " << charts[i].author << ")\n";
                        }

                        int chartIndex = -1;
                        std::cout << "\nEnter chart number to view details (0 to go back): ";
                        std::cin >> chartIndex;
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                        if (chartIndex > 0 && chartIndex <= static_cast<int>(charts.size()))
                        {
                            const ChartData &selectedChart = charts[chartIndex - 1];

                            std::cout << "\n==== Chart Details ====\n";
                            std::cout << "ID: " << selectedChart.name << "\n";
                            std::cout << "Title: " << selectedChart.title << "\n";
                            std::cout << "Artist: " << selectedChart.artist << "\n";
                            std::cout << "Charter: " << selectedChart.author << "\n";
                            std::cout << "Difficulty: " << selectedChart.rating << "\n";
                            std::cout << "Upload date: " << selectedChart.uploadDate << "\n";
                            std::cout << "Description: " << selectedChart.description << "\n";

                            std::cout << "Tags: ";
                            for (const auto &tag : selectedChart.tags)
                            {
                                std::cout << tag << " ";
                            }
                            std::cout << "\n";

                            std::cout << "Public status: " << (selectedChart.isPublic ? "Public" : "Private") << "\n";
                            std::cout << "Collaboration: " << (selectedChart.isCollaboration ? "Yes" : "No") << "\n";
                            std::cout << "Private sharing: " << (selectedChart.isPrivateShare ? "Yes" : "No") << "\n";
                        }
                    }
                    else
                    {
                        std::cout << "No charts found.\n";
                    }
                }
                else
                {
                    std::cout << "No username entered.\n";
                }
                break;

            case 0:
                std::cout << "Exiting.\n";
                break;

            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
            }
        } while (choice != 0);
    }
    else
    {
        std::cout << "Login failed." << std::endl;
    }

    stopFileWatcher();

    curl_global_cleanup();

    return 0;
}