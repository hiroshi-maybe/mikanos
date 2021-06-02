#pragma once

enum LogLevel {
    kError = 3,
    kWarn = 4,
    kInfo = 6,
    kDebug = 7,
};

// Set global minimum log level to be logged
void SetLogLevel(LogLevel level);

// Output a log with specified log level
int Log(LogLevel level, const char* format, ...);