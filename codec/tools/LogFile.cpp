#include "LogFile.h"
#include "LogFileManager.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>





AppendFile::AppendFile(StringArg filename)
  : fp_(::fopen(filename.c_str(), "w")),  // 'e' for O_CLOEXEC
    writtenBytes_(0)
{
  assert(fp_);
  //::setbuffer(fp_, buffer_, sizeof buffer_); on linux
  setvbuf(fp_, buffer_, _IOFBF, sizeof buffer_);
  // posix_fadvise POSIX_FADV_DONTNEED ?
}

AppendFile::~AppendFile()
{
  ::fclose(fp_);
}

//循环写入，并记录写入的字节数

void AppendFile::append(const char* logline, const size_t len)
{
  size_t written = 0;

  while (written != len)
  {
    size_t remain = len - written;
    size_t n = write(logline + written, remain);
    //如果刚刚写入的部分不等于剩余需要写入的部分？
    //如果一次写不完，又没有报错，那就继续写，通过while。可能会有一些原因导致一次没有写完全部数据。
    if (n != remain)
    {
      int err = ferror(fp_);
      if (err)
      {
        //fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
        break;
      }
    }
    written += n;
  }

  writtenBytes_ += written;
}

void AppendFile::flush()
{
  ::fflush(fp_);//刷新文件流 fp_ 的缓冲区。它确保所有缓冲区中的数据都被写入到文件中
}

size_t AppendFile::write(const char* logline, size_t len)
{
  // fwrite_unlocked on linux， _fwrite_nolock on windows
  return ::fwrite_unlocked(logline, 1, len, fp_);
}


TimeRoller::TimeRoller(const string basename,
                off_t rollSize,
                int checkEveryN)
                  : rollSize_(rollSize),
                checkEveryN_(checkEveryN),
                count_(0),
                startOfPeriod_(0),
                lastRoll_(0), FileRoller(basename)
{
  //assert(basename.find('/') == string::npos);可以设置输出目录的话，应该不用判断这个。
}


LogFile::LogFile(std::unique_ptr<FileRoller>& roller, int flushInterval)
  : flushInterval_(flushInterval),
    lastFlush_(0)
{
  if(roller != nullptr){
    roller_ = std::move(roller);
  }else{
    assert(false);
  }
  
  rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char* logline, int len)
{
    file_->append(logline, len);

  //roll
  time_t now = ::time(NULL);
  if(roller_ && roller_->judgeRoll(file_.get(), now)){
    rollFile();
    lastFlush_ = now;
  }

  //flush
  if (now - lastFlush_ > flushInterval_)
  {
    lastFlush_ = now;
    file_->flush();
  }
}

void LogFile::flush()
{
    file_->flush();
}

void LogFile::rollFile()
{
  time_t now = ::time(NULL);
  lastFlush_ = now;
  string filename = roller_->generateFileName();//basename_, &now
  file_.reset(new AppendFile(filename));
}

void TimeRoller::freshRoll(){
  lastRoll_ = judgeTime_;
  startOfPeriod_ = judgeTime_ / kRollPerSeconds_ * kRollPerSeconds_;
}

bool TimeRoller::judgeRoll(AppendFile* file_, time_t now){
  judgeTime_ = now;
  if (file_->writtenBytes() > rollSize_)
  {
    return true;
  }
  else
  {
    ++count_;
    if (count_ >= checkEveryN_)
    {
      count_ = 0;
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_)
      {
        return true;
      }
    }
  }
  return false;
}

string TimeRoller::generateFileName()
{
  string filename;
  filename.reserve(basename_.size() + 64);
  filename = basename_;

  char timebuf[32];
  struct tm tm;

  gmtime_r(&judgeTime_, &tm);//linux系统上为gmtime_r(&judgeTime_, &tm),win为gmtime_s(&tm, &judgeTime_);
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;
  filename += ".log";

  return filename;
}

map<string, LogFile*> LogFileManager::fileNameToLogFile = {};
string LogFileManager::logFilePath = ".";


