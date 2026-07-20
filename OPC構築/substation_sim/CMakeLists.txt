cmake_minimum_required(VERSION 3.10)
project(substation_sim C)

# ==============================================================
#  libIEC61850 のパスを環境に合わせて設定
#  ビルド済みの libIEC61850 リポジトリのルートを指定する
# ==============================================================
# 方法1: 環境変数で指定
#   export LIBIEC61850_HOME=/path/to/libiec61850
#
# 方法2: cmake 引数で指定
#   cmake .. -DLIBIEC61850_HOME=/path/to/libiec61850
#
# 方法3: 指定なし（../libiec61850 を自動検出）

if(NOT DEFINED LIBIEC61850_HOME)
    if(DEFINED ENV{LIBIEC61850_HOME})
        set(LIBIEC61850_HOME $ENV{LIBIEC61850_HOME})
    else()
        set(LIBIEC61850_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../libiec61850")
    endif()
endif()

# 相対パスを絶対パスに変換
# ※ -D で渡された相対パスは build ディレクトリ基準なので、
#    CMAKE_BINARY_DIR を起点に結合してから絶対化する
if(NOT IS_ABSOLUTE "${LIBIEC61850_HOME}")
    set(LIBIEC61850_HOME "${CMAKE_BINARY_DIR}/${LIBIEC61850_HOME}")
endif()
get_filename_component(LIBIEC61850_HOME "${LIBIEC61850_HOME}" ABSOLUTE)

if(NOT EXISTS "${LIBIEC61850_HOME}")
    message(FATAL_ERROR
        "libIEC61850 not found at: ${LIBIEC61850_HOME}\n"
        "Specify the correct path:\n"
        "  cmake .. -DLIBIEC61850_HOME=/absolute/path/to/libiec61850")
endif()

message(STATUS "LIBIEC61850_HOME = ${LIBIEC61850_HOME}")

# ==============================================================
#  ヘッダファイルの探索
#  libIEC61850 のバージョンによりディレクトリ構造が異なるため
#  複数の候補パスを登録する
# ==============================================================
set(LIBIEC_INCLUDE_CANDIDATES
    # static_model.h（生成ファイル）
    ${CMAKE_CURRENT_SOURCE_DIR}

    # libIEC61850 標準レイアウト (v1.4+)
    ${LIBIEC61850_HOME}/src/inc
    ${LIBIEC61850_HOME}/src/common/inc
    ${LIBIEC61850_HOME}/src/mms/inc
    ${LIBIEC61850_HOME}/src/mms/inc_private
    ${LIBIEC61850_HOME}/src/iec61850/inc
    ${LIBIEC61850_HOME}/src/iec61850/inc_private
    ${LIBIEC61850_HOME}/src/logging
    ${LIBIEC61850_HOME}/hal/inc
    ${LIBIEC61850_HOME}/src/goose
    ${LIBIEC61850_HOME}/config

    # 古いレイアウト
    ${LIBIEC61850_HOME}/inc
    ${LIBIEC61850_HOME}/src/mms/iso_mms/common
    ${LIBIEC61850_HOME}/src/mms/iso_mms/client
    ${LIBIEC61850_HOME}/src/mms/iso_mms/server

    # make install 後のレイアウト
    ${LIBIEC61850_HOME}/include
    ${LIBIEC61850_HOME}/include/libiec61850
    ${LIBIEC61850_HOME}/include/iec61850
    ${LIBIEC61850_HOME}/include/hal

    # ビルドディレクトリ内に生成されるヘッダ
    ${LIBIEC61850_HOME}/build/config
    ${LIBIEC61850_HOME}/build/src/inc

    # hal のサブディレクトリ
    ${LIBIEC61850_HOME}/hal/inc
    ${LIBIEC61850_HOME}/hal/thread
    ${LIBIEC61850_HOME}/hal/socket
)

# 実在するディレクトリのみを登録
set(LIBIEC_INCLUDES "")
foreach(DIR ${LIBIEC_INCLUDE_CANDIDATES})
    if(EXISTS "${DIR}")
        list(APPEND LIBIEC_INCLUDES "${DIR}")
    endif()
endforeach()

if(NOT LIBIEC_INCLUDES)
    message(FATAL_ERROR
        "No valid include directories found under ${LIBIEC61850_HOME}\n"
        "Make sure libIEC61850 source tree is present.")
endif()

# ヘッダが見つかったか確認
find_file(IEC61850_SERVER_H "iec61850_server.h" PATHS ${LIBIEC_INCLUDES} NO_DEFAULT_PATH)
if(IEC61850_SERVER_H)
    message(STATUS "Found iec61850_server.h at ${IEC61850_SERVER_H}")
else()
    message(WARNING
        "iec61850_server.h not found in known paths.\n"
        "Searched: ${LIBIEC_INCLUDES}\n"
        "Build may fail. Run: find ${LIBIEC61850_HOME} -name iec61850_server.h")
endif()

include_directories(${LIBIEC_INCLUDES})
message(STATUS "Include dirs: ${LIBIEC_INCLUDES}")

# ==============================================================
#  ライブラリパス
# ==============================================================
set(LIBIEC_LIB_CANDIDATES
    ${LIBIEC61850_HOME}/build/src
    ${LIBIEC61850_HOME}/build/hal
    ${LIBIEC61850_HOME}/build
    ${LIBIEC61850_HOME}/build/src/Release
    ${LIBIEC61850_HOME}/build/src/Debug
    ${LIBIEC61850_HOME}/build/hal/Release
    ${LIBIEC61850_HOME}/build/hal/Debug
    ${LIBIEC61850_HOME}/lib
)

set(LIBIEC_LIBDIRS "")
foreach(DIR ${LIBIEC_LIB_CANDIDATES})
    if(EXISTS "${DIR}")
        list(APPEND LIBIEC_LIBDIRS "${DIR}")
    endif()
endforeach()

link_directories(${LIBIEC_LIBDIRS})
message(STATUS "Library dirs: ${LIBIEC_LIBDIRS}")

# ==============================================================
#  ビルドターゲット
# ==============================================================
add_executable(substation_server
    substation_server.c
    static_model.c
)

if(WIN32)
    target_link_libraries(substation_server
        iec61850
        hal
        ws2_32
    )
else()
    target_link_libraries(substation_server
        iec61850
        hal
        pthread
        m
    )
endif()
