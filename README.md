# ScoreSync4US
ScoreSybc4USは、UntitledSekaiでアップロードされている譜面のデータの同期と管理を行うためのクロスプラットフォームです。
Webサービスとローカル環境での譜面ファイルの自動同期により、制作の効率化を実現しました。

## 主な機能
- 譜面データの同期：ローカルファイルの変更を検知し、自動的にUntitled Sekaiサーバーへ同期
- 譜面リスト管理：ユーザー自身および他ユーザーの譜面一覧と詳細表示の閲覧
- クロスプラットフォーム対応：Windows/Linux両環境で動作

## 必要環境
- Windows/Linux対応OS
- インターネット接続環境
- Docker(ビルド時のみ必要)
- Untitled Sekaiのアカウント(Webから作成できます)

## インストール方法
ビルド済みのバイナリを使用する場合

1. [リリースページ](https://github.com/Untitled-Sekai/ScoreSync4US-cpp/releases)から最新のバイナリをダウンロード
2. ダウンロードしたファイルを任意のフォルダに展開

ソースからビルドする場合

必要なもの
- Docker
- docker-compose

ビルド手順

1. リポジトリのクローン
```bash
git clone https://github.com/Untitled-Sekai/ScoreSync4US-cpp.git

cd ScoreSync4Us-cpp
```

2. Dockerを使用してビルド
```bash
# windowsの場合
.\clean-rebuild.bat

# Linuxの場合
chmod +x clean-rebuild.sh
./clean-rebuild.sh
```

3. ビルドが完了すると`./release`フォルダに実行ファイルが生成されます。

- Windows:ss4us-windows.exe
- Linux:ss4us-linux

## 使用方法
1. アプリケーションの起動

```bash
# windows
.\ss4us-windows.exe

# Linux
./ss4us-linux
```
2. ユーザー名とパスワードを入力し、ログイン

3. メニュー画面から以下の操作が可能
- 自分の譜面一覧表示
- 他ユーザーの譜面一覧表示
- 譜面の詳細情報表示

4. `levels`フォルダ内にあるファイルを編集すると、変更が自動的に検出され同期されます

## プロジェクト構造
```
ScoreSync4US-cpp/
├── main.cpp                # メインプログラム
├── setting.h/.cpp          # アプリケーション設定
├── login.h/.cpp            # ログイン処理
├── list.h/.cpp             # 譜面リスト取得・表示
├── config.h/.cpp           # 設定・データベース管理
├── FileWatcher.h/.cpp      # ファイル変更監視
├── CMakeLists.txt          # CMakeビルド設定
├── Dockerfile.linux        # Linuxビルド用Dockerfile
├── Dockerfile.windows      # Windowsビルド用Dockerfile
├── docker-compose.yml      # Dockerコンテナ設定
├── clean-rebuild.bat       # Windows用ビルドスクリプト
└── clean-rebuild.sh        # Linux用ビルドスクリプト
```

## 依存ライブラリ

- libcurl - HTTP通信

- SQLite3 - ローカルデータベース

- nlohmann/json - JSON処理

- C++17標準ライブラリ - ファイルシステム等

