#ifndef LOGFILE_H
#define LOGFILE_H
#include <string>
#include <memory>
#include "noncopyable.h"
#include <map>
#include <string>

using namespace std;


// For passing C-style string argument to a function.
class StringArg // copyable
{
 public:
  StringArg(const char* str)
    : str_(str)
  { }

  StringArg(const string& str)
    : str_(str.c_str())
  { }

  const char* c_str() const { return str_; }

 private:
  const char* str_;
};

class AppendFile : noncopyable
{
 public:
  explicit AppendFile(StringArg filename);
  ~AppendFile();
  void append(const char* logline, size_t len);
  void flush();
  off_t writtenBytes() const { return writtenBytes_; }
  
 private:
  size_t write(const char* logline, size_t len);
  
  FILE* fp_;
  char buffer_[64*1024];
  off_t writtenBytes_;
};

class FileRoller{//立即flush
public:
    FileRoller(const string basename):basename_(basename){}
    virtual bool judgeRoll(AppendFile* file_, time_t now) { file_->flush();return false; }
    virtual void freshRoll() { return; }
    virtual string generateFileName() { return basename_; }
    const string basename_;
};

class TimeRoller : public FileRoller{
public:
  TimeRoller(const string basename,
                off_t rollSize,
                int checkEveryN = 1024);
  bool judgeRoll(AppendFile* file_, time_t now) override;//注意！这里直接传入裸指针了。需要确保file_在judgeRoll时还存活。如果多线程情况应该用shared_ptr
  void freshRoll() override;
  string generateFileName() override;
private:
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t judgeTime_;//用来传递now
  int count_;

  const off_t rollSize_;
  const int checkEveryN_;
  const static int kRollPerSeconds_ = 60*60*24;
};

//用来判断AppendFile的Roll和Flush时机。
class LogFile : noncopyable
{
 public:
  LogFile(std::unique_ptr<FileRoller>& roller, int flushInterval = 3);//在构造时传入，能否使得两者生命周期一样长？
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  void rollFile();
 private:
  std::unique_ptr<AppendFile> file_;
  std::unique_ptr<FileRoller> roller_;

  time_t lastFlush_;
  const int flushInterval_;
};



#endif