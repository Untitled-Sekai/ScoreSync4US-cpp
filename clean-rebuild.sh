export LANG=ja_JP.UTF-8
set -e

echo "ScoreSync4US clean rebuild script"
echo "=================================="

# Dockerのキャッシュもクリア
echo "Stopping and removing Docker containers..."
for container_id in $(docker ps -a -q --filter "name=temp-linux" --filter "name=temp-windows"); do
    docker rm -f $container_id 2>/dev/null || true
done

echo "Removing Docker images..."
docker rmi scoresync4us-cpp-linux-build scoresync4us-cpp-windows-build 2>/dev/null || true

echo "Cleaning build directories..."
rm -rf build build-win release
mkdir -p release

echo
echo "キャッシュをクリアしました。ビルドを開始します..."
echo

# CMakeListsのチェック
echo "Checking CMakeLists.txt for unsupported parameters..."
if grep -q "DOWNLOAD_EXTRACT_TIMESTAMP" CMakeLists.txt; then
  echo "[warning] DOWNLOAD_EXTRACT_TIMESTAMP found in CMakeLists.txt, this may cause issues with the build."
  read -p "Press Enter to continue..."
fi

echo
echo "Building Docker images..."
# Docker buildのみを実行（コンテナ実行はスキップ）
docker-compose build

echo
echo "Extracting binaries from the built images..."

# Linuxビルドの抽出
echo "Extracting Linux build..."
if ! docker create --name temp-linux scoresync4us-cpp-linux-build; then
  echo "[Error] Failed to create Linux container."
  exit 1
fi
docker cp temp-linux:/app/release/ss4us-linux ./release/
docker rm temp-linux

# Windowsビルドの抽出
echo "Extracting Windows build..."

# コンテナ作成時にコマンドを追加（Alpineではlsは動作する）
echo "Creating temporary Windows container..."
if ! docker create --name temp-windows scoresync4us-cpp-windows-build ls -la /; then
  echo "[Error] Failed to create Windows container."
  exit 1
fi

# ファイルのコピー（実行ファイル）
echo "Copying Windows executable..."
if ! docker cp temp-windows:/ss4us-windows.exe ./release/; then
  echo "[Warning] Failed to copy Windows executable from root. Trying alternative path..."
  if ! docker cp temp-windows:/app/release/ss4us-windows.exe ./release/; then
    echo "[Error] Failed to copy Windows executable."
    docker rm temp-windows
    exit 1
  fi
fi

# DLLファイルのコピー - 改良版
echo "Copying Windows DLL files..."
DLL_COPIED=0

# 複数の場所を順番に試す
echo "Trying known DLL locations..."

# 1. アプリリリースディレクトリから直接コピー
echo "Trying /app/release/*.dll..."
if docker cp temp-windows:/app/release/*.dll ./release/ 2>/dev/null; then
  echo "[Success] Copied DLLs from /app/release/"
  DLL_COPIED=1
fi

# 2. outputディレクトリからコピー
if [ $DLL_COPIED -eq 0 ]; then
  echo "Trying /output/*.dll..."
  if docker cp temp-windows:/output/*.dll ./release/ 2>/dev/null; then
    echo "[Success] Copied DLLs from /output/"
    DLL_COPIED=1
  fi
fi

# 3. ルートディレクトリからコピー
if [ $DLL_COPIED -eq 0 ]; then
  echo "Trying root directory /*.dll..."
  if docker cp temp-windows:/*.dll ./release/ 2>/dev/null; then
    echo "[Success] Copied DLLs from root directory"
    DLL_COPIED=1
  fi
fi

# 4. 個別のよく使われるDLL（一つずつ試す）
if [ $DLL_COPIED -eq 0 ]; then
  echo "[Warning] Trying individual DLL files..."
  
  # よく使用される基本的なDLLファイルをリストアップ
  for dll in libcurl-4.dll libsqlite3-0.dll libstdc++-6.dll libwinpthread-1.dll libgcc_s_seh-1.dll libnghttp2-14.dll libidn2-0.dll libssh2-1.dll libssl-1_1.dll libcrypto-1_1.dll zlib1.dll; do
    echo "Trying to copy $dll..."
    if docker cp temp-windows:/app/release/$dll ./release/ 2>/dev/null; then
      echo "[Success] Copied $dll from /app/release/"
      DLL_COPIED=1
    elif docker cp temp-windows:/$dll ./release/ 2>/dev/null; then
      echo "[Success] Copied $dll from root"
      DLL_COPIED=1
    elif docker cp temp-windows:/output/$dll ./release/ 2>/dev/null; then
      echo "[Success] Copied $dll from /output/"
      DLL_COPIED=1
    elif docker cp temp-windows:/opt/mxe/usr/x86_64-w64-mingw32.shared.posix/bin/$dll ./release/ 2>/dev/null; then
      echo "[Success] Copied $dll from MXE directory"
      DLL_COPIED=1
    fi
  done
fi

# 一時コンテナを削除
docker rm temp-windows

echo
echo "completed"
echo

# ビルド結果の確認
if [ -f "release/ss4us-linux" ]; then
  echo "[successful] Completed Linux build!"
else
  echo "[Failed] Failed to build Linux."
fi

if [ -f "release/ss4us-windows.exe" ]; then
  echo "[successful] Completed Windows build!"
else
  echo "[Failed] Failed to build Windows."
fi

# DLLファイルの確認を追加
DLL_COUNT=$(find release -name "*.dll" | wc -l)
if [ $DLL_COUNT -gt 0 ]; then
  echo "[successful] Found $DLL_COUNT DLL files for Windows."
else
  echo "[Warning] No DLL files found for Windows. Application may not run correctly."
fi

# 必須DLLの確認
echo
echo "Checking for required DLLs:"
MISSING_DLLS=0

for dll in libcurl-4.dll libsqlite3-0.dll libstdc++-6.dll libwinpthread-1.dll libgcc_s_seh-1.dll libnghttp2-14.dll; do
  if [ -f "release/$dll" ]; then
    echo "[OK] $dll found"
  else
    echo "[Missing] $dll not found. Application may have issues."
    MISSING_DLLS=$((MISSING_DLLS + 1))
  fi
done

if [ $MISSING_DLLS -gt 0 ]; then
  echo "[Warning] $MISSING_DLLS required DLLs are missing."
fi

echo
echo "=================================="
echo "Release files:"
echo "- ss4us-linux: Linux"
echo "- ss4us-windows.exe: Windows"
echo "- *.dll: Windows依存ライブラリ (${DLL_COUNT}個)"
echo "=================================="