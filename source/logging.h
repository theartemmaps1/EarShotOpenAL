#pragma once
void Log(const char* func, int line, const char* msg, ...);
void CloseLog();
void ClearLogFile();
#define LOG(msg, ...) Log(__func__, __LINE__, msg, ##__VA_ARGS__)
extern bool Logging;
extern uint64_t maxBytesInLog;