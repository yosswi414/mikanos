/**
 * @file logger.hpp
 * @brief カーネルロガーの実装
 * @date 2023-03-20
 */

#pragma once

enum LogLevel {
    kError = 3,
    kWarn = 4,
    kInfo = 6,
    kDebug = 7,
    kVerbose = 2147483647,
};

/**
 * @brief グローバルなログ優先度の閾値を変更する
 * グローバルなログ優先度の閾値を level に設定
 * 以降の Log 呼び出しではここで設定した優先度以上のログのみ記録される
 */
void SetLogLevel(LogLevel level);

/**
 * @brief ログを指定された優先度で記録する
 * 指定された優先度が閾値以上なら記録する
 * 優先度が閾値未満ならログは捨てられる
 * @param level ログの優先度 閾値以上の優先度のログのみが記録される
 * @param format 書式文字列 printk と互換
 */
int Log(LogLevel level, const char* format, ...);
