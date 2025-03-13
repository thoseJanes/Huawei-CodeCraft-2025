
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


  // �����ļ���
  const char* filename = "E:\\work\\study\\affair_AfterUndergraduate\\competitions\\2025huawei\\code\\example.txt";

  // ���ļ���д��ģʽ��
  FILE* file = fopen(filename, "w");

  // ����ļ��Ƿ�ɹ���
  if (file == nullptr) {
      std::cerr << "Failed to open file: " << filename << std::endl;
      return 1;
  }

  // ����Ҫд�������
  const char* content = "Hello, World!\nThis is an example of writing to a file in C++ using fwrite.\n";

  // д�����ݵ��ļ�
  size_t length = strlen(content);
  size_t written = _fwrite_nolock(content, sizeof(char), length, file);

  // ���д���Ƿ�ɹ�
  if (written != length) {
      std::cerr << "Failed to write to file: " << filename << std::endl;
      fclose(file);
      return 1;
  }

  // �ر��ļ�
  fclose(file);

  std::cout << "File written successfully: " << filename << std::endl;

}
