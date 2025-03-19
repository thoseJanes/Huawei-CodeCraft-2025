#ifndef LOGGER_H
#define LOGGER_H
#include <memory>
#include <string.h>
#include <string>
#include <stdio.h>
#include <functional>  
#include "LogFileManager.h"
using namespace std;



//StringPiece
class StringLike{
public:
    StringLike(const char* cc):cptr_(cc), len_(static_cast<int>(strlen(cptr_))){}
    StringLike(const char* cc, size_t length):cptr_(cc), len_(static_cast<int>(length)){}
    StringLike(const char* cc, int length):cptr_(cc), len_(length){}
    StringLike(const string& str):cptr_(str.c_str()), len_(static_cast<int>(str.size())){}

    int length() const {return len_;}
    const char* data() const {return cptr_;}

private:
    const char* cptr_;
    int len_;
};

//FixedBuffer
const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000*1000;
template<int SIZE>
class LogBuffer{
public:
    LogBuffer() :cur_(data_){}
    void testAppend(const char* cc, size_t size){
        if(space() > static_cast<int>(size)){
            memcpy(cur_, cc, size);
            cur_ += size;
        }
    }
    void testAppend(uintptr_t p, int reserveSpace);
    void testAppend(double v, int reserveSpace);

    int length() const {return static_cast<int>(cur_-data_);}
    int space() const {return static_cast<int>(data_ + sizeof data_ - cur_);}
    const char* debugString();

    void add(int len) { cur_ += len; }
    char* current() { return cur_; }
    const char* data() const { return data_; }

    StringLike toStringLike() const {return StringLike(cur_, length());}
private:
    char data_[SIZE];
    char* cur_;
};


class Fmt // : noncopyable
{
 public:
  template<typename T>
  Fmt(const char* fmt, T val);

  const char* data() const { return buf_; }
  int length() const { return length_; }

 private:
  char buf_[32];
  int length_;
};


class LogStream{
    typedef LogStream self;
    
public:
    typedef LogBuffer<kSmallBuffer> Buffer;
    self& operator<<(bool v)
  {
    buffer_.testAppend(v ? "1" : "0", 1);
    return *this;
  }

    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);

    self& operator<<(const void* p){
        uintptr_t v = reinterpret_cast<uintptr_t>(p);
        buffer_.testAppend(v, kMaxNumericSize);
        return *this;
    }

    self& operator<<(double v){
        buffer_.testAppend(v, kMaxNumericSize);
        return *this;
    }
    self& operator<<(float v)
    {
        *this << static_cast<double>(v);
        return *this;
    }
    
    // self& operator<<(long double);
    self& operator<<(char v)
    {
        buffer_.testAppend(&v, 1);
        return *this;
    }
    // self& operator<<(signed char);
    // self& operator<<(unsigned char);
    self& operator<<(const char* str)
    {
        if (str)
        {
            buffer_.testAppend(str, strlen(str));
        }
        else
        {
            buffer_.testAppend("(null)", 6);
        }
        return *this;
    }
    self& operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }
    self& operator<<(const string& v)
    {
        buffer_.testAppend(v.c_str(), v.size());
        return *this;
    }
    self& operator<<(const StringLike& v)
    {
        buffer_.testAppend(v.data(), v.length());
        return *this;
    }
    self& operator<<(const Buffer& v)
    {
        *this << v.toStringLike();
        return *this;
    }

    void staticCheck();
    template<typename T>
    void formatInteger(T v);
    const Buffer& buffer() const { return buffer_; }
    void testAppend(const char* data, int len) { buffer_.testAppend(data, len); }
private:
    Buffer buffer_;
    static const int kMaxNumericSize = 48;
};


class Logger{
public:
    class SourceFile
    {
    public:
        template<int N>
        SourceFile(const char (&arr)[N])
        : data_(arr),
            size_(N-1)
        {
        const char* slash = strrchr(data_, '/'); // builtin function
        if (slash)
        {
            data_ = slash + 1;
            size_ -= static_cast<int>(data_ - arr);
        }
        }

        explicit SourceFile(const char* filename)
        : data_(filename)
        {
        const char* slash = strrchr(filename, '/');
        if (slash)
        {
            data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
        }

        const char* data_;
        int size_;
    };
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };
    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, const char* func, string logFileName);//输出到文件。
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    Logger(SourceFile file, int line, bool toAbort);
    ~Logger();

    LogStream& stream() { return impl_.stream_; }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    //typedef void (*OutputFunc)(const char* msg, int len);
    //typedef void (*FlushFunc)();
    //OutputFunc l_output;
    //FlushFunc l_flush;
    std::function<void(const char*, int)> l_output;
    std::function<void()> l_flush;
private:
    class Impl
    {
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
        void finish();

        //Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };

    Impl impl_;
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
    return g_logLevel;
}

#define LOG_TRACE if (Logger::logLevel() <= Logger::TRACE) \
  Logger(__FILE__, __LINE__, Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (Logger::logLevel() <= Logger::DEBUG) \
  Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
#define LOG_INFO if (Logger::logLevel() <= Logger::INFO) \
  Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()

#define LOG_FILE(x) if (Logger::logLevel() <= Logger::DEBUG && LogFileManager::existFile(x)) \
  Logger(__FILE__, __LINE__, __func__, x).stream()



#endif