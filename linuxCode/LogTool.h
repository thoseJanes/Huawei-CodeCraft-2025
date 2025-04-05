#ifndef LOGTOOL_H
#define LOGTOOL_H

#include <string>

#if true
#include "../tools/Logger.h"

#else
using namespace std;
class LogStream {
public:
	LogStream() {};
	template<typename T>
    LogStream& operator<<(T k) { return *this; }
};

class LogFileManager {
public:
    static void addLogFile(const string fileName) {
    };
    static string getLogFilePath() { return "";}
    static void setLogFilePath(const string path) {
    }
    static bool existFile(const string fileName) {
    	return false;
    };
    static void flushAll() {
    }
    ~LogFileManager() {
    }
};

#define LOG_FILE(x) if (false) \
  LogStream()
#endif


#endif
