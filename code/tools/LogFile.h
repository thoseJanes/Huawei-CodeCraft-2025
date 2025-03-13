#include <string>
#include <memory>
#include "noncopyable.h"

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


class LogFile : noncopyable
{
 public:
  LogFile(const string& basename,
          off_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();

  typedef void (*RollerJudger)();
  typedef void (*NameGenerator)(const string& basename, time_t* now);

 private:
  void append_unlocked(const char* logline, int len);

  static string getLogFileName(const string& basename, time_t* now);

  const string basename_;
  const off_t rollSize_;
  const int flushInterval_;
  const int checkEveryN_;

  int count_;

  //std::unique_ptr<MutexLock> mutex_;
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t lastFlush_;
  std::unique_ptr<AppendFile> file_;

  const static int kRollPerSeconds_ = 60*60*24;
};


class LogFileManager{

};