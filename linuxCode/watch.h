#ifndef WATCH_H
#define WATCH_H
#include "global.h"


class Watch{
public:
    static void correct(int timeStamp){
        if (timeStamp - timeStamp_ != 0) {
            throw std::runtime_error("time lose!");
        }
        calBucket();
    }
    static int getTime(){return timeStamp_;}
    static int toTimeStep(int tokens){return (tokens-1)/G+1;}
    static void calBucket(){bucket_ = (timeStamp_-1)/FRE_PER_SLICING;}
    static int getBucket(){return bucket_;}
    static void clock(){timeStamp_++;calBucket();}
    static void start(){}
    static void end(){}
private:
    static int offset_;
    static int timeStamp_;
    static int bucket_;
    static std::vector<int> starts_;
    static std::vector<int> tols_;
};


#endif