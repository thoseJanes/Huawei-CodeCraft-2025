
#include "LogFile.h"
#include "Logger.h"
#include <time.h>
#include <iostream>
#include <cstdio>
#include <cstring>

std::unique_ptr<LogFile> g_logFile;

void outputFunc(const char* msg, int len)
{
  g_logFile->append(msg, len);
}

void flushFunc()
{
  g_logFile->flush();
}

int main(int argc, char* argv[])
{
  char name[256] = {"test.txt"};
  strncpy(name, argv[0], sizeof name - 1);
  g_logFile.reset(new LogFile(Logger::SourceFile(name).data_, 200*1000));
  Logger::setGlobalOutput(outputFunc);
  Logger::setGlobalFlush(flushFunc);

  string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  for (int i = 0; i < 10000; ++i)
  {
    LOG_INFO << line << i;

    //Sleep(1);
  }
  g_logFile->flush();


  // 定义文件名
  const char* filename = "E:\\work\\study\\affair_AfterUndergraduate\\competitions\\2025huawei\\code\\example.txt";

  // 打开文件（写入模式）
  FILE* file = fopen(filename, "w");

  // 检查文件是否成功打开
  if (file == nullptr) {
      std::cerr << "Failed to open file: " << filename << std::endl;
      return 1;
  }

  // 定义要写入的内容
  const char* content = "Hello, World!\nThis is an example of writing to a file in C++ using fwrite.\n";

  // 写入内容到文件
  size_t length = strlen(content);
  size_t written = _fwrite_nolock(content, sizeof(char), length, file);

  // 检查写入是否成功
  if (written != length) {
      std::cerr << "Failed to write to file: " << filename << std::endl;
      fclose(file);
      return 1;
  }

  // 关闭文件
  fclose(file);

  std::cout << "File written successfully: " << filename << std::endl;

}
