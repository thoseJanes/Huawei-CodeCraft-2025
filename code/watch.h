#ifndef WATCH_H
#define WATCH_H
#include "global.h"

class Watch{
public:
    static int correct(int timeStamp){
        offset_ = timeStamp - timeStamp_;
        if(offset_){
            timeStamp_ += offset_; 
            calBucket();
        }
        return offset_;
    }
    static int getTime(){return timeStamp_;}
    static void calBucket(){bucket_ = (timeStamp_-1)/FRE_PER_SLICING;}
    static int getBucket(){return bucket_;}
    static void clock(){timeStamp_++;calBucket();}
private:
    static int offset_;
    static int timeStamp_;
    static int bucket_;
};


#endif