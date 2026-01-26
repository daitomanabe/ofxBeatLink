# ofxBeatLink Windows Build Steps

Windows環境で`ofxBeatLink`をビルドするための手順書です。

## 前提条件

- Visual Studio 2022 (C++デスクトップ開発ワークロード)
- CMake 3.15以上
- openFrameworks 0.12.0
- vcpkg (オプション: iconv等の依存関係が必要な場合)

## ビルド手順

### 1. beat-link-cppライブラリのビルド

#### PowerShellで実行:

```powershell
cd P:\path\to\of_v0.12.0_vs_release\addons\ofxBeatLink\libs\beat-link-cpp

# ビルドディレクトリを作成
if (Test-Path build_vs) { Remove-Item -Recurse -Force build_vs }
mkdir build_vs
cd build_vs

# CMake設定（Visual Studio 2022）
cmake .. -G "Visual Studio 17 2022" -A x64 `
         -DCMAKE_BUILD_TYPE=Release `
         -DBEATLINK_BUILD_TESTS=OFF `
         -DBEATLINK_BUILD_PYTHON=OFF

# ビルド実行
cmake --build . --config Release -j8
```

### 2. 必要なソースコード修正

以下のファイルに修正が必要です（すでに修正済みの場合はスキップ）：

#### A. `libs/crate-digger-cpp/include/cratedigger/rekordbox_pdb.hpp`

`#include <filesystem>` を追加:

```cpp
#include "types.hpp"
#include <fstream>
#include <memory>
#include <cstdint>
#include <vector>
#include <filesystem>  // 追加
```

#### B. `libs/crate-digger-cpp/src/core/rekordbox_pdb.cpp`

文字列連結を修正 (87行目付近):

```cpp
// 修正前:
return make_error(
    ErrorCode::FileNotFound,
    "Cannot open file: " + path.string()
);

// 修正後:
return make_error(
    ErrorCode::FileNotFound,
    std::string("Cannot open file: ") + path.string()
);
```

#### C. `libs/beat-link-cpp/include/beatlink/dbserver/Message.hpp`

Windows APIマクロとの競合を回避:

```cpp
#undef COLOR_MENU  // この行を追加
enum class MenuItemType : uint32_t {
    FOLDER = 0x0001,
    // ...
    COLOR_MENU = 0x008e,
    // ...
};
```

`int64_t` を `std::int64_t` に置換（ファイル全体）

#### D. `libs/beat-link-cpp/src/dbserver/Message.cpp`

`int64_t` を `std::int64_t` に置換（ファイル全体）

### 3. addon_config.mk の設定確認

`addon_config.mk` の `vs:` セクションが以下のようになっていることを確認:

```makefile
vs:
	# Visual Studio settings - link prebuilt CMake libraries
	ADDON_FRAMEWORKS =
	# CMake dependency includes (Visual Studio build)
	ADDON_INCLUDES += libs/beat-link-cpp/build_vs/_deps/fmt-src/include
	ADDON_INCLUDES += libs/beat-link-cpp/build_vs/_deps/utf8proc-src
	ADDON_INCLUDES += libs/beat-link-cpp/build_vs/_deps/kaitai_runtime-src
	# Link beat-link-cpp and its dependencies (built with CMake)
	ADDON_LIBS = libs/beat-link-cpp/build_vs/Release/beatlink.lib
	ADDON_LIBS += libs/beat-link-cpp/build_vs/Release/sqlite3.lib
	ADDON_LIBS += libs/beat-link-cpp/build_vs/_deps/fmt-build/Release/fmt.lib
	ADDON_LIBS += libs/beat-link-cpp/build_vs/_deps/kaitai_runtime-build/Release/kaitai_struct_cpp_stl_runtime.lib
	ADDON_LIBS += libs/beat-link-cpp/build_vs/_deps/miniz-build/Release/miniz.lib
	ADDON_LIBS += libs/beat-link-cpp/build_vs/_deps/utf8proc-build/Release/utf8proc_static.lib
	# Winsock2 for network operations
	ADDON_LDFLAGS = ws2_32.lib
```

### 4. プロジェクトの生成/更新

#### コマンドラインから:

```powershell
cd P:\path\to\of_v0.12.0_vs_release\addons\ofxBeatLink\example-basic

P:\path\to\of_v0.12.0_vs_release\projectGenerator\resources\app\app\projectGenerator.exe `
    -r `
    -o"P:\path\to\of_v0.12.0_vs_release" `
    "P:\path\to\of_v0.12.0_vs_release\addons\ofxBeatLink\example-basic"
```

### 5. Visual Studioでビルド

1. `example-basic.sln` を Visual Studio で開く
2. ビルド構成を `Release` または `Debug` に設定
3. `example-basic` プロジェクトをビルド

## トラブルシューティング

### エラー: `fmt/format.h` が見つからない

- `addon_config.mk` の `vs:` セクションに `fmt` のインクルードパスが追加されているか確認
- プロジェクトジェネレーターで更新を実行

### エラー: `utf8proc.lib` が見つからない

- 正しいファイル名は `utf8proc_static.lib` です
- `addon_config.mk` を確認

### エラー: `std::filesystem::path` が認識されない

- `rekordbox_pdb.hpp` に `#include <filesystem>` を追加
- C++17が有効になっているか確認

### エラー: `COLOR_MENU` の識別子エラー

- `Message.hpp` の `MenuItemType` enum の前に `#undef COLOR_MENU` を追加

## 生成されるライブラリファイル一覧

ビルド成功後、以下のファイルが生成されます:

```
build_vs/Release/
├── beatlink.lib
├── sqlite3.lib
└── imgui.lib

build_vs/_deps/
├── fmt-build/Release/fmt.lib
├── kaitai_runtime-build/Release/kaitai_struct_cpp_stl_runtime.lib
├── miniz-build/Release/miniz.lib
└── utf8proc-build/Release/utf8proc_static.lib
```

## 注意事項

- ビルドディレクトリ名 `build_vs` は固定です（`addon_config.mk` で参照）
- Releaseビルドを推奨（Debugビルドの場合は `addon_config.mk` の調整が必要）
- CMakeのバージョンが古い場合は3.15以上に更新してください
- プロジェクトジェネレーターは `addon_config.mk` を変更するたびに実行が必要です

## MSYS2/MinGW GCC でのビルド（代替方法）

Visual Studioの代わりにMSYS2を使用する場合：

```bash
# MSYS2 MinGW 64-bit シェルで実行
cd /p/path/to/of_v0.12.0_vs_release/addons/ofxBeatLink/libs/beat-link-cpp
mkdir build_msys2 && cd build_msys2

cmake .. -G "MinGW Makefiles" \
         -DCMAKE_BUILD_TYPE=Release \
         -DBEATLINK_BUILD_TESTS=OFF \
         -DBEATLINK_BUILD_PYTHON=OFF

cmake --build . -j8
```

この場合、`addon_config.mk` の `msys2:` セクションの設定を使用します。

