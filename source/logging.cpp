#include "plugin.h"
#include "logging.h"
using namespace plugin;
bool Logging = false;
static FILE* LogFile = 0;
uint64_t maxBytesInLog = 900000; // Maximum number of bytes that will be written in the log file
uint64_t numBytesInLog; // Number of bytes currently written into thelog file
void Log(const char* msg, ...) {
    // If log file ain't open yet, open it
    if (LogFile == nullptr && Logging) {
        LogFile = fopen(PLUGIN_PATH("EarShotOpenAL.log"), "a");
        if (LogFile == nullptr) {
            Error("Failed to open the log file! Reason: %s.", strerror(errno));
            return;
        }
    }

    if (!Logging) {
        // If logging is disabled, write a farewell message, and close for good...
        if (LogFile != nullptr) {
            SYSTEMTIME systemTime;
            GetLocalTime(&systemTime);
            fprintf(LogFile, "[%02d/%02d/%d %02d:%02d:%02d] [LOG]: Logging is disabled. Closing log file...\n",
                systemTime.wDay, systemTime.wMonth, systemTime.wYear,
                systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
            CloseLog();
        }
        return;
    }


    // Logging is on, writing sum logging stuff
    va_list args;
    va_start(args, msg);
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    fprintf(LogFile, "[%02d/%02d/%d %02d:%02d:%02d]",
        systemTime.wDay, systemTime.wMonth, systemTime.wYear,
        systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
    numBytesInLog += vfprintf(LogFile, msg, args) + 2;
    #ifdef DEBUG
    std::string message;
    va_list ap_copy;
    va_copy(ap_copy, args);
    size_t len = vsnprintf(0, 0, msg, ap_copy);
    message.resize(len + 1);  // need space for NUL
    vsnprintf(&message[0], len + 1, msg, args);
    message.resize(len);  // remove the NUL

    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
    #endif
    // Truncate the log if it's too big
    if (numBytesInLog >= maxBytesInLog) {
        ClearLogFile();
    }
    fprintf(LogFile, "\n");
    //CloseLog();
    fflush(LogFile);
    va_end(args);
}

void CloseLog()
{
    // If the log file is open...
    if (LogFile)
    {
        // ...then close it
        fclose(LogFile);
        LogFile = 0;
    }
}

void ClearLogFile()
{
    // Clears the amount of bytes written...
    auto path = PLUGIN_PATH("EarShotOpenAL.log");
    numBytesInLog = 0;
    if (LogFile == nullptr)
    {
        // Log file hadn't been open before, open it now.
        LogFile = fopen(path, "w");
        if (!LogFile) {
            Error("Failed to open log file for clearing. Reason: %s.", strerror(errno));
        }
    }
    else
    {
        // Reopen the file for truncation
        if ((LogFile = freopen(path, "w", LogFile)) == 0)
        {
            // Wuut, we couldn't do it?
            Error("Failed to clear the log file! Reason: %s.", strerror(errno));
            CloseLog();
        }

    }
}