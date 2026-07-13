cmake_minimum_required(VERSION 3.0)
project(substation_sim C)

# ==============================================================
#  libIEC61850 のパスを環境に合わせて設定
#  ビルド済みの libIEC61850 リポジトリのルートを指定する
# ==============================================================
# 方法1: 環境変数で指定
#   set LIBIEC61850_HOME=/path/to/libiec61850
#
# 方法2: cmake 引数で指定
#   cmake .. -DLIBIEC61850_HOME=/path/to/libiec61850

if(NOT DEFINED LIBIEC61850_HOME)
    if(DEFINED ENV{LIBIEC61850_HOME})
        set(LIBIEC61850_HOME $ENV{LIBIEC61850_HOME})
    else()
        # デフォルト: 同じディレクトリ階層にある libiec61850 を想定
        set(LIBIEC61850_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../libiec61850")
    endif()
endif()

message(STATUS "LIBIEC61850_HOME = ${LIBIEC61850_HOME}")

# ==============================================================
#  インクルードパス
# ==============================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}            # static_model.h
    ${LIBIEC61850_HOME}/src/inc
    ${LIBIEC61850_HOME}/src/common/inc
    ${LIBIEC61850_HOME}/src/mms/inc
    ${LIBIEC61850_HOME}/src/mms/inc_private
    ${LIBIEC61850_HOME}/src/iec61850/inc
    ${LIBIEC61850_HOME}/src/iec61850/inc_private
    ${LIBIEC61850_HOME}/hal/inc
    ${LIBIEC61850_HOME}/src/goose
    ${LIBIEC61850_HOME}/config
)

# ==============================================================
#  ライブラリパス
# ==============================================================
if(WIN32)
    # Windows: Visual Studio ビルドの場合
    link_directories(
        ${LIBIEC61850_HOME}/build/src/Release
        ${LIBIEC61850_HOME}/build/src/Debug
        ${LIBIEC61850_HOME}/build/hal/Release
        ${LIBIEC61850_HOME}/build/hal/Debug
        # CMake 単一構成の場合
        ${LIBIEC61850_HOME}/build/src
        ${LIBIEC61850_HOME}/build/hal
    )
else()
    # Linux
    link_directories(
        ${LIBIEC61850_HOME}/build/src
        ${LIBIEC61850_HOME}/build/hal
    )
endif()

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
