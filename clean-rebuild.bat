@echo off
echo ScoreSync4US clean rebuild script
echo ==================================

REM Dockerのキャッシュもクリア
echo Stopping and removing Docker containers...
FOR /F "tokens=*" %%i IN ('docker ps -a -q --filter "name=temp-linux" --filter "name=temp-windows"') DO (
    docker rm -f %%i 2>nul
)

echo Removing Docker images...
docker rmi scoresync4us-cpp-linux-build scoresync4us-cpp-windows-build 2>nul

echo Cleaning build directories...
if exist build rmdir /s /q build
if exist build-win rmdir /s /q build-win
if exist release rmdir /s /q release
mkdir release

echo.
echo キャッシュをクリアしました。ビルドを開始します...
echo.

REM CMakeListsのチェック - パスを修正
echo Checking CMakeLists.txt for unsupported parameters...
findstr /C:"DOWNLOAD_EXTRACT_TIMESTAMP" CMakeLists.txt >nul 2>&1
if %errorlevel% equ 0 (
  echo [warning] DOWNLOAD_EXTRACT_TIMESTAMP found in CMakeLists.txt, this may cause issues with the build.
  pause
)

echo.
echo Building Docker images...
REM Docker buildのみを実行（コンテナ実行はスキップ）
docker-compose build

echo.
echo Extracting binaries from the built images...

REM Linuxビルドの抽出
echo Extracting Linux build...
docker create --name temp-linux scoresync4us-cpp-linux-build
if %errorlevel% neq 0 (
  echo [Error] Failed to create Linux container.
  goto check_results
)
docker cp temp-linux:/app/release/ss4us-linux ./release/
docker rm temp-linux

REM Windowsビルドの抽出
echo Extracting Windows build...

REM コンテナ作成時にコマンドを追加（Alpineではlsは動作する）
echo Creating temporary Windows container...
docker create --name temp-windows scoresync4us-cpp-windows-build ls -la /
if %errorlevel% neq 0 (
  echo [Error] Failed to create Windows container.
  goto check_results
)

REM ファイルのコピー（実行ファイル）
echo Copying Windows executable...
docker cp temp-windows:/ss4us-windows.exe ./release/
if %errorlevel% neq 0 (
  echo [Warning] Failed to copy Windows executable from root. Trying alternative path...
  docker cp temp-windows:/app/release/ss4us-windows.exe ./release/
  if %errorlevel% neq 0 (
    echo [Error] Failed to copy Windows executable.
    goto cleanup_windows
  )
)

REM DLLファイルのコピー - 改良版
echo Copying Windows DLL files...
set DLL_COPIED=0

REM 複数の場所を順番に試す（fingコマンドを使わない）
echo Trying known DLL locations...

REM 1. アプリリリースディレクトリから直接コピー
echo Trying /app/release/*.dll...
docker cp temp-windows:/app/release/*.dll ./release/ >nul 2>&1
if %errorlevel% equ 0 (
  echo [Success] Copied DLLs from /app/release/
  set DLL_COPIED=1
)

REM 2. outputディレクトリからコピー
if %DLL_COPIED% equ 0 (
  echo Trying /output/*.dll...
  docker cp temp-windows:/output/*.dll ./release/ >nul 2>&1
  if %errorlevel% equ 0 (
    echo [Success] Copied DLLs from /output/
    set DLL_COPIED=1
  )
)

REM 3. ルートディレクトリからコピー
if %DLL_COPIED% equ 0 (
  echo Trying root directory /*.dll...
  docker cp temp-windows:/*.dll ./release/ >nul 2>&1
  if %errorlevel% equ 0 (
    echo [Success] Copied DLLs from root directory
    set DLL_COPIED=1
  )
)

REM 4. 個別のよく使われるDLL（一つずつ試す）
if %DLL_COPIED% equ 0 (
  echo [Warning] Trying individual DLL files...
  
  REM よく使用される基本的なDLLファイルをリストアップ（libnghttp2-14.dllを追加）
  for %%d in (libcurl-4.dll libsqlite3-0.dll libstdc++-6.dll libwinpthread-1.dll libgcc_s_seh-1.dll libnghttp2-14.dll libidn2-0.dll libssh2-1.dll libssl-1_1.dll libcrypto-1_1.dll zlib1.dll) do (
    echo Trying to copy %%d...
    docker cp temp-windows:/app/release/%%d ./release/ >nul 2>&1
    if %errorlevel% equ 0 (
      echo [Success] Copied %%d from /app/release/
      set DLL_COPIED=1
    ) else (
      docker cp temp-windows:/%%d ./release/ >nul 2>&1
      if %errorlevel% equ 0 (
        echo [Success] Copied %%d from root
        set DLL_COPIED=1
      ) else (
        docker cp temp-windows:/output/%%d ./release/ >nul 2>&1
        if %errorlevel% equ 0 (
          echo [Success] Copied %%d from /output/
          set DLL_COPIED=1
        ) else (
          docker cp temp-windows:/opt/mxe/usr/x86_64-w64-mingw32.shared.posix/bin/%%d ./release/ >nul 2>&1
          if %errorlevel% equ 0 (
            echo [Success] Copied %%d from MXE directory
            set DLL_COPIED=1
          )
        )
      )
    )
  )
)

:cleanup_windows
docker rm temp-windows

:check_results
echo.
echo completed
echo.

REM check build results
if exist release\ss4us-linux (
  echo [successful] Completed Linux build!
) else (
  echo [Failed] Failed to build Linux.
)

if exist release\ss4us-windows.exe (
  echo [successful] Completed Windows build!
) else (
  echo [Failed] Failed to build Windows.
)

REM DLLファイルの確認を追加
set DLL_COUNT=0
for %%f in (release\*.dll) do set /a DLL_COUNT+=1
if %DLL_COUNT% gtr 0 (
  echo [successful] Found %DLL_COUNT% DLL files for Windows.
) else (
  echo [Warning] No DLL files found for Windows. Application may not run correctly.
)

REM 必須DLLの確認
echo.
echo Checking for required DLLs:
set MISSING_DLLS=0

for %%d in (libcurl-4.dll libsqlite3-0.dll libstdc++-6.dll libwinpthread-1.dll libgcc_s_seh-1.dll libnghttp2-14.dll) do (
  if exist release\%%d (
    echo [OK] %%d found
  ) else (
    echo [Missing] %%d not found. Application may have issues.
    set /a MISSING_DLLS+=1
  )
)

if %MISSING_DLLS% gtr 0 (
  echo [Warning] %MISSING_DLLS% required DLLs are missing.
)

echo.
echo ==================================
echo Release files:
echo - ss4us-linux: Linux
echo - ss4us-windows.exe: Windows
echo - *.dll: Windows依存ライブラリ (%DLL_COUNT%個)
echo ==================================