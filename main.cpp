#include <iostream>
#include <string>
#include <curl/curl.h>
#include "login.h"
#include "setting.h"
#include "list.h"

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    
    std::string username, password;
    
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    
    std::cout << "Enter password: ";
    std::getline(std::cin, password);
    
    std::string token = login(username, password);
    
    if (!token.empty()) {
        std::cout << "Login successful!" << std::endl;
        
        int choice = 0;
        do {
            std::cout << "\n==== メニュー ====\n";
            std::cout << "1. 自分の譜面リストを表示\n";
            std::cout << "2. 別のユーザーの譜面リストを表示\n";
            std::cout << "0. 終了\n";
            std::cout << "選択: ";
            std::cin >> choice;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 改行文字を消費
            
            std::string targetUsername;
            
            switch(choice) {
                case 1:
                    {
                        std::cout << "\n自分の譜面リストを取得中...\n";
                        std::vector<ChartData> charts = getUserCharts(token, username);
                        
                        if (!charts.empty()) {
                            json formattedData = formatChartsData(charts);
                            std::cout << "\n取得した譜面数: " << charts.size() << "\n\n";
                            
                            for (size_t i = 0; i < charts.size(); ++i) {
                                std::cout << i+1 << ". " << charts[i].title 
                                          << " / " << charts[i].artist
                                          << " (作者: " << charts[i].author << ")\n";
                            }
                            
                            int chartIndex = -1;
                            std::cout << "\n詳細を表示する譜面番号を入力してください（0で戻る）: ";
                            std::cin >> chartIndex;
                            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            
                            if (chartIndex > 0 && chartIndex <= static_cast<int>(charts.size())) {
                                const ChartData& selectedChart = charts[chartIndex - 1];
                                
                                std::cout << "\n==== 譜面詳細 ====\n";
                                std::cout << "名前: " << selectedChart.name << "\n";
                                std::cout << "タイトル: " << selectedChart.title << "\n";
                                std::cout << "アーティスト: " << selectedChart.artist << "\n";
                                std::cout << "譜面作者: " << selectedChart.author << "\n";
                                std::cout << "難易度: " << selectedChart.rating << "\n";
                                std::cout << "アップロード日: " << selectedChart.uploadDate << "\n";
                                std::cout << "説明: " << selectedChart.description << "\n";
                                
                                std::cout << "タグ: ";
                                for (const auto& tag : selectedChart.tags) {
                                    std::cout << tag << " ";
                                }
                                std::cout << "\n";
                                
                                std::cout << "公開状態: " << (selectedChart.isPublic ? "公開" : "非公開") << "\n";
                                std::cout << "コラボレーション: " << (selectedChart.isCollaboration ? "あり" : "なし") << "\n";
                                std::cout << "プライベート共有: " << (selectedChart.isPrivateShare ? "あり" : "なし") << "\n";
                            }
                        } else {
                            std::cout << "譜面が見つかりませんでした。\n";
                        }
                    }
                    break;
                    
                case 2:
                    std::cout << "\n譜面を表示するユーザー名を入力: ";
                    std::getline(std::cin, targetUsername);
                    
                    if (!targetUsername.empty()) {
                        std::cout << targetUsername << " の譜面リストを取得中...\n";
                        std::vector<ChartData> charts = getUserCharts(token, targetUsername);
                        
                        if (!charts.empty()) {
                            json formattedData = formatChartsData(charts);
                            std::cout << "\n取得した譜面数: " << charts.size() << "\n\n";
                            
                            for (size_t i = 0; i < charts.size(); ++i) {
                                std::cout << i+1 << ". " << charts[i].title 
                                          << " / " << charts[i].artist
                                          << " (作者: " << charts[i].author << ")\n";
                            }

                            int chartIndex = -1;
                            std::cout << "\n詳細を表示する譜面番号を入力してください（0で戻る）: ";
                            std::cin >> chartIndex;
                            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            
                            if (chartIndex > 0 && chartIndex <= static_cast<int>(charts.size())) {
                                const ChartData& selectedChart = charts[chartIndex - 1];
                                
                                std::cout << "\n==== 譜面詳細 ====\n";
                                std::cout << "名前: " << selectedChart.name << "\n";
                                std::cout << "タイトル: " << selectedChart.title << "\n";
                                std::cout << "アーティスト: " << selectedChart.artist << "\n";
                                std::cout << "譜面作者: " << selectedChart.author << "\n";
                                std::cout << "難易度: " << selectedChart.rating << "\n";
                                std::cout << "アップロード日: " << selectedChart.uploadDate << "\n";
                                std::cout << "説明: " << selectedChart.description << "\n";
                                
                                std::cout << "タグ: ";
                                for (const auto& tag : selectedChart.tags) {
                                    std::cout << tag << " ";
                                }
                                std::cout << "\n";
                                
                                std::cout << "公開状態: " << (selectedChart.isPublic ? "公開" : "非公開") << "\n";
                                std::cout << "コラボレーション: " << (selectedChart.isCollaboration ? "あり" : "なし") << "\n";
                                std::cout << "プライベート共有: " << (selectedChart.isPrivateShare ? "あり" : "なし") << "\n";
                            }
                        } else {
                            std::cout << "譜面が見つかりませんでした。\n";
                        }
                    } else {
                        std::cout << "ユーザー名が入力されていません。\n";
                    }
                    break;
                    
                case 0:
                    std::cout << "終了します。\n";
                    break;
                    
                default:
                    std::cout << "無効な選択です。もう一度お試しください。\n";
                    break;
            }
        } while (choice != 0);
    } else {
        std::cout << "Login failed." << std::endl;
    }
    
    curl_global_cleanup();
    
    return 0;
}