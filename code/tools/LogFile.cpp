#include "LogFile.h"
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
  // fwrite_unlocked on linux
  return ::_fwrite_nolock(logline, 1, len, fp_);
}



LogFile::LogFile(const string& basename,
                 off_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
  : basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
  assert(basename.find('/') == string::npos);
  rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char* logline, int len)
{
    append_unlocked(logline, len);
}

void LogFile::flush()
{
    file_->flush();
}

void LogFile::append_unlocked(const char* logline, int len)
{
  file_->append(logline, len);

  if (file_->writtenBytes() > rollSize_)
  {
    rollFile();
  }
  else
  {
    ++count_;
    if (count_ >= checkEveryN_)
    {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_)
      {
        rollFile();
      }
      else if (now - lastFlush_ > flushInterval_)
      {
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

bool LogFile::rollFile()
{
  time_t now = 0;
  string filename = getLogFileName(basename_, &now);
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  if (now > lastRoll_)
  {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    file_.reset(new AppendFile(filename));
    return true;
  }
  return false;
}

string LogFile::getLogFileName(const string& basename, time_t* now)
{
  string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_s(&tm, now);//linux系统上为gmtime_r(now, &tm)
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;
  filename += ".log";

  return filename;
}