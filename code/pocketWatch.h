#ifndef POCKETWATCH_H
#define POCKETWATCH_H

class PocketWatch{
public:
    static int correct(int timeStamp){
        offset_ = timeStamp - timeStamp_;
        if(offset_){
            timeStamp_ += offset_; 
        }
        return offset_;
    }
    static int getTime(){return timeStamp_;}
    static int getBucket(int width){return (timeStamp_-1)/width;}
    static void clock(){timeStamp_++;}
private:
    static int offset_;
    static int timeStamp_;
};
int PocketWatch::timeStamp_ = 0;

#endif