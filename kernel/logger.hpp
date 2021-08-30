#pragma once

enum LogLevel {
    kError = 3,
    kWarn = 4,
    kInfo = 6,
    kDebug = 7,
};

// Set global minimum log level to be logged
void SetLogLevel(LogLevel level);
LogLevel GetLogLevel();

// Output a log with specified log level
int Log(LogLevel level, const char* format, ...);

template<typename Func>
void DebugLog(Func execute) {
    LogLevel original_level = GetLogLevel();
    SetLogLevel(kDebug);
    execute();
    SetLogLevel(original_level);
}
