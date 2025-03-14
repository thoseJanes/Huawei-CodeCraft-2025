
#include "LogFile.h"
#include "Logger.h"
#include <time.h>
#include <iostream>
#include <cstdio>
#include <cstring>

int main(int argc, char* argv[])
{
  
  LogFileManager::addLogFile("test");
  string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  for (int i = 0; i < 10000; ++i)
  {
    LOG_FILE("test") << line << i;
  }
  LogFileManager::flushAll();

}
