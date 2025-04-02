#ifndef LOGFILEMANAGER_H
#define LOGFILEMANAGER_H
#include <filesystem>

#include "LogFile.h"


namespace fs = std::filesystem;
//用来管理各种文件的Log。
class LogFileManager {
public:
    static void addLogFile(const string fileName) {
        if (!existFile(fileName)) {
            string filePath = logFilePath + "\\" + fileName;
            std::unique_ptr<FileRoller> fileRoller = std::make_unique<FileRoller>(filePath);
            fileNameToLogFile[fileName] = new LogFile(fileRoller);
        }
    };
    static LogFile* getLogFile(const string fileName) {
        if (existFile(fileName)) {
            return fileNameToLogFile[fileName];
        }
        return nullptr;
    }
    static string getLogFilePath() { return logFilePath; }
    static void setLogFilePath(const string path) { 
        logFilePath = path;
        if (!fs::exists(path)) {
            fs::create_directories(path);
        }
    }
    static bool existFile(const string fileName) {
        return fileNameToLogFile.find(fileName) != fileNameToLogFile.end();
    };
    static void flushAll() {
        for (auto it = fileNameToLogFile.begin(); it != fileNameToLogFile.end(); it++) {
            it->second->flush();
        }
    }
    ~LogFileManager() {
        this->flushAll();
    }
private:
    static map<string, LogFile*> fileNameToLogFile;
    static string logFilePath;
};


#endif // !LOGFILEMANAGER_H
