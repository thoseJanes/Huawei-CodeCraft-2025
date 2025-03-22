
#include "LogFile.h"
#include "Logger.h"
#include <time.h>
#include <iostream>
#include <cstdio>
#include <cstring>

int main(int argc, char* argv[])
{
  LogFileManager::setLogFilePath("./log");
  LogFileManager::addLogFile("test");
  LogFileManager::addLogFile("test2");
  LogFileManager::addLogFile("test3");
  
  string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  Logger::setLogLevel(Logger::DEBUG);
  for (int i = 0; i < 10000; ++i)
  {
    LOG_FILE("test") << line << i;
    LOG_FILE("test2") << line << i;
    LOG_FILE("test4") << line << i;
    LOG_FILE("test3") << line << i;
  }
  LogFileManager::flushAll();
}
