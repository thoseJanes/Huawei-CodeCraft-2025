#include "Logger.h"
#include "LogFile.h"

#include <algorithm>
#include <limits>
#include <type_traits>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>

namespace detail
{
    const char digits[] = "9876543210123456789";
    const char* zero = digits + 9;
    static_assert(sizeof(digits) == 20, "wrong number of digits");

    const char digitsHex[] = "0123456789ABCDEF";
    static_assert(sizeof digitsHex == 17, "wrong number of digitsHex");

    // Efficient Integer to String Conversions, by Matthew Wilson.
    template<typename T>
    size_t convert(char buf[], T value)
    {
    T i = value;
    char* p = buf;

    do
    {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);//反转从buf到p的内存（貌似不包含p）

    return p - buf;
    }

    size_t convertHex(char buf[], uintptr_t value)
    {
    uintptr_t i = value;
    char* p = buf;

    do
    {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
    }

}  // namespace detail

/*
 Format a number with 5 characters, including SI units.
 [0,     999]
 [1.00k, 999k]
 [1.00M, 999M]
 [1.00G, 999G]
 [1.00T, 999T]
 [1.00P, 999P]
 [1.00E, inf)
*/
std::string formatSI(int64_t s)
{
  double n = static_cast<double>(s);
  char buf[64];
  if (s < 1000)
    snprintf(buf, sizeof(buf), "%" PRId64, s);
  else if (s < 9995)
    snprintf(buf, sizeof(buf), "%.2fk", n/1e3);
  else if (s < 99950)
    snprintf(buf, sizeof(buf), "%.1fk", n/1e3);
  else if (s < 999500)
    snprintf(buf, sizeof(buf), "%.0fk", n/1e3);
  else if (s < 9995000)
    snprintf(buf, sizeof(buf), "%.2fM", n/1e6);
  else if (s < 99950000)
    snprintf(buf, sizeof(buf), "%.1fM", n/1e6);
  else if (s < 999500000)
    snprintf(buf, sizeof(buf), "%.0fM", n/1e6);
  else if (s < 9995000000)
    snprintf(buf, sizeof(buf), "%.2fG", n/1e9);
  else if (s < 99950000000)
    snprintf(buf, sizeof(buf), "%.1fG", n/1e9);
  else if (s < 999500000000)
    snprintf(buf, sizeof(buf), "%.0fG", n/1e9);
  else if (s < 9995000000000)
    snprintf(buf, sizeof(buf), "%.2fT", n/1e12);
  else if (s < 99950000000000)
    snprintf(buf, sizeof(buf), "%.1fT", n/1e12);
  else if (s < 999500000000000)
    snprintf(buf, sizeof(buf), "%.0fT", n/1e12);
  else if (s < 9995000000000000)
    snprintf(buf, sizeof(buf), "%.2fP", n/1e15);
  else if (s < 99950000000000000)
    snprintf(buf, sizeof(buf), "%.1fP", n/1e15);
  else if (s < 999500000000000000)
    snprintf(buf, sizeof(buf), "%.0fP", n/1e15);
  else
    snprintf(buf, sizeof(buf), "%.2fE", n/1e18);
  return buf;
}

/*
 [0, 1023]
 [1.00Ki, 9.99Ki]
 [10.0Ki, 99.9Ki]
 [ 100Ki, 1023Ki]
 [1.00Mi, 9.99Mi]
*/
std::string formatIEC(int64_t s)
{
  double n = static_cast<double>(s);
  char buf[64];
  const double Ki = 1024.0;
  const double Mi = Ki * 1024.0;
  const double Gi = Mi * 1024.0;
  const double Ti = Gi * 1024.0;
  const double Pi = Ti * 1024.0;
  const double Ei = Pi * 1024.0;

  if (n < Ki)
    snprintf(buf, sizeof buf, "%" PRId64, s);
  else if (n < Ki*9.995)
    snprintf(buf, sizeof buf, "%.2fKi", n / Ki);
  else if (n < Ki*99.95)
    snprintf(buf, sizeof buf, "%.1fKi", n / Ki);
  else if (n < Ki*1023.5)
    snprintf(buf, sizeof buf, "%.0fKi", n / Ki);

  else if (n < Mi*9.995)
    snprintf(buf, sizeof buf, "%.2fMi", n / Mi);
  else if (n < Mi*99.95)
    snprintf(buf, sizeof buf, "%.1fMi", n / Mi);
  else if (n < Mi*1023.5)
    snprintf(buf, sizeof buf, "%.0fMi", n / Mi);

  else if (n < Gi*9.995)
    snprintf(buf, sizeof buf, "%.2fGi", n / Gi);
  else if (n < Gi*99.95)
    snprintf(buf, sizeof buf, "%.1fGi", n / Gi);
  else if (n < Gi*1023.5)
    snprintf(buf, sizeof buf, "%.0fGi", n / Gi);

  else if (n < Ti*9.995)
    snprintf(buf, sizeof buf, "%.2fTi", n / Ti);
  else if (n < Ti*99.95)
    snprintf(buf, sizeof buf, "%.1fTi", n / Ti);
  else if (n < Ti*1023.5)
    snprintf(buf, sizeof buf, "%.0fTi", n / Ti);

  else if (n < Pi*9.995)
    snprintf(buf, sizeof buf, "%.2fPi", n / Pi);
  else if (n < Pi*99.95)
    snprintf(buf, sizeof buf, "%.1fPi", n / Pi);
  else if (n < Pi*1023.5)
    snprintf(buf, sizeof buf, "%.0fPi", n / Pi);

  else if (n < Ei*9.995)
    snprintf(buf, sizeof buf, "%.2fEi", n / Ei );
  else
    snprintf(buf, sizeof buf, "%.1fEi", n / Ei );
  return buf;
}


template<typename T>
Fmt::Fmt(const char* fmt, T val)
{
  static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");

  length_ = snprintf(buf_, sizeof buf_, fmt, val);
  assert(static_cast<size_t>(length_) < sizeof buf_);
}

// Explicit instantiations
template Fmt::Fmt(const char* fmt, char);
template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);
template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);




template<int SIZE>
const char* LogBuffer<SIZE>::debugString()
{
  *cur_ = '\0';
  return data_;
}

template<int SIZE>
inline void LogBuffer<SIZE>::testAppend(uintptr_t p, int reserveSpace){
    
    if (space() >= reserveSpace)
    {
        char* buf = cur_;
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = detail::convertHex(buf+2, p);
        cur_ += 2;
    }
}

template<int SIZE>
inline void LogBuffer<SIZE>::testAppend(double v, int reserveSpace){
    if (space() >= reserveSpace)
    {
        int len = snprintf(cur_, reserveSpace, "%.12g", v);
        /*
        g 格式：
        如果数值的绝对值大于或等于 10 −4且小于 10 6，则使用固定小数点格式（f 格式）。
        否则，使用科学计数法（e 格式）。
        精度 .12 表示小数部分最多显示 12 位数字。
        */
        cur_ += len;
    }
}



void LogStream::staticCheck()
{
  //std::numeric_limits<T>::digits10用于获取T类型能够精确表示的最大十进制数字位数
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
                "kMaxNumericSize is large enough");
}

template<typename T>
void LogStream::formatInteger(T v)
{
  if (buffer_.space() >= kMaxNumericSize)//如果剩余的buffer长度比能表示的最大数字长度大
  {
    size_t len = detail::convert(buffer_.current(), v);
    buffer_.add(len);
  }
}

LogStream& LogStream::operator<<(short v)
{
  *this << static_cast<int>(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
  *this << static_cast<unsigned int>(v);
  return *this;
}

LogStream& LogStream::operator<<(int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
  formatInteger(v);
  return *this;
}


class T
{
 public:
  T(const char* str, unsigned len)
    :str_(str),
     len_(len)
  {
    assert(strlen(str) == len_);//只在debug模式下
  }

  const char* str_;
  const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v)
{
  s.testAppend(v.str_, v.len_);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
  s.testAppend(v.data_, v.size_);
  return s;
}


const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};



Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
  : stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
  //formatTime();
  stream_ << T(LogLevelName[level], 6);
  if (savedErrno != 0)
  {
    stream_ << " (errno=" << savedErrno << ") ";
  }
}

void Logger::Impl::finish()
{
  stream_ << " - " << basename_ << ':' << line_ << '\n';
}


void defaultOutput(const char* msg, int len)
{
    size_t n = fwrite(msg, 1, len, stdout);
    //FIXME check n
    (void)n;
}

void defaultFlush()
{
    fflush(stdout);
}


Logger::Logger(SourceFile file, int line)
  : impl_(INFO, 0, file, line), l_output(defaultOutput), l_flush(defaultFlush)
{
}

Logger::Logger(SourceFile file, int line, const char* func, string logFileName)
  : impl_(INFO, 0, file, line)
{
  assert(LogFileManager::existFile(logFileName));
  l_output = std::bind(&LogFile::append, LogFileManager::getLogFile(logFileName),
                std::placeholders::_1, std::placeholders::_2);
  l_flush = std::bind(&LogFile::flush, LogFileManager::getLogFile(logFileName));
  //g_output = 在LogFileManager中注册的函数作为匿名函数。
  //g_flush = 在...
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : impl_(level, 0, file, line), l_output(defaultOutput), l_flush(defaultFlush)
{
  impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
  : impl_(level, 0, file, line), l_output(defaultOutput), l_flush(defaultFlush)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort)
  : impl_(toAbort?FATAL:ERROR, errno, file, line), l_output(defaultOutput), l_flush(defaultFlush)
{
}

Logger::~Logger()
{
  //impl_在logger创建时初始化，会给logger的logstream添加时间、线程id、日志级别等信息，
  //而logger销毁时，impl_的finish会添加文件名和行号
  impl_.finish();
  const LogStream::Buffer& buf(stream().buffer());
  l_output(buf.data(), buf.length());
  if (impl_.level_ == FATAL)
  {
    l_flush();
    abort();
  }
}



Logger::LogLevel initLogLevel()
{
    if (::getenv("LOG_TRACE"))
        return Logger::TRACE;
    else if (::getenv("LOG_DEBUG"))
        return Logger::DEBUG;
    else
        return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

void Logger::setLogLevel(Logger::LogLevel level)
{
  g_logLevel = level;
}

