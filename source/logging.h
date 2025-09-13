#pragma once
void Log(const char* msg, ...);
void CloseLog();
void ClearLogFile();
extern bool Logging;
extern uint64_t maxBytesInLog;