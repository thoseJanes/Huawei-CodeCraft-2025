#ifndef GLOBAL_H
#define GLOBAL_H
#include <memory>
#include <cmath>
#include <list>
#include <map>
#include <vector>
#include <stdexcept>
#include "LogTool.h"

#define LOG_PLANNER LOG_FILE("planner")
#define LOG_PLANNERN(x) LOG_FILE("planner"+std::to_string(x))
#define LOG_IPCINFO LOG_FILE("ipcInfo")
#define LOG_BplusTreeN(x) LOG_FILE("BplusTree"+std::to_string(x))
#define LOG_BplusTreeInfoN(x) LOG_FILE("BplusTreeInfo"+std::to_string(x))
#define LOG_BplusTree LOG_FILE("BplusTree")
#define LOG_BplusTreeInfo LOG_FILE("BplusTreeInfo")
#define LOG_LINKEDSPACE LOG_FILE("circularLinkedList")
#define LOG_LINKEDSPACEN(x) LOG_FILE("circularLinkedList"+std::to_string(x))
#define LOG_INIT LOG_FILE("main")
#define LOG_REQUEST LOG_FILE("request")
#define LOG_OBJECT LOG_FILE("object")
#define LOG_BplusTreeTest LOG_FILE("bPlusTreeTest")
#define LOG_SCORE LOG_FILE("score")

//#define ENABLE_SCATTEROBJS
#define ENABLE_OBJECTEDGE
#ifdef ENABLE_OBJECTEDGE
    //#define ENABLE_OBJECTSCORE
#endif
#define ENABLE_INDICATOR
//#define ENABLE_ACCELERATION


#define MAX_DISK_NUM (10 + 1)//磁盘id从0开始
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)//id从1开始//4字节（32位）：范围是 [−2147483648,2147483647]
#define MAX_OBJECT_NUM (100000 + 1)//id从1开始
#define REP_NUM (3)
#define HEAD_NUM (2)
#define FRE_PER_SLICING (1800)


#define PHASE_ONE_TIME (10)
#define EXTRA_TIME (105)
#define SCORE_FACTOR(size) (size+1)
//这里的分数共乘了四倍，为了方便除法运算。
#define START_SCORE (4000)
#define PHASE_ONE_EDGE (20)
#define PHASE_TWO_EDGE (40)

#define PLAN_STEP (1)
#define MULTIREAD_SEARCH_NUM (10)
#define PASS_TO_JUMP_SEARCH_NUM (35)
#define MULTIREAD_JUDGE_LENGTH (11)

#define FIRST_READ_CONSUME (64)

static std::map<int, int> toNextReadConsume = {
    {64, 52},{52, 42},{42, 34},{34, 28},{28, 23}, {23, 19},{19, 16}, {16, 16}
};
static std::vector<int> readConsumeAfterN = {
    {64, 52, 42, 34, 28, 23, 19, 16}
};
static std::map<int, int> toAheadReadTimes = {
    {64, 0},{52, 1},{42, 2},{34, 3},{28, 4}, {23, 5},{19, 6}, {16, 7}//8代表大于等于8
};
inline int getReadConsumeAfterN(int n){
    if(n>readConsumeAfterN.size()-1){
        return 16;
    }else{
        return readConsumeAfterN[n];
    }
}
inline int getNextReadConsume(int presentConsume){
    return toNextReadConsume[presentConsume];
}
inline int getAheadReadTimes(int nextReadConsume){
    return toAheadReadTimes[nextReadConsume];
}



extern int T;//时间步
extern int M;//tag数，输入tag从1到M，内部从0到M-1
extern int N;//磁盘数，输入磁盘从1开始到N，内部磁盘从0到N-1
extern int V;//单元数，输入单元从1开始到V，内部单元用0到V-1表示
extern int G;//令牌数
extern int K;//垃圾收集阶段的交换单元数（对于每个硬盘）

template<typename T>
LogStream& operator<<(LogStream& s, const std::vector<T>& vec){
    s<<"(vec:";
    for(int i=0;i<vec.size();i++){
        s << vec[i]<<",";
    }
    s<<")";
    return s;
}

template<typename T>
LogStream& operator<<(LogStream& s, const std::list<T>& lst){
    s<<"(lst:";
    for(auto it = lst.begin();it!=lst.end();it++){
        s<<*it<<", ";
    }
    s<<")";
    return s;
}


//模糊模式：对象数量并不精准契合统计数据
//精准模式：对象数量完美契合统计数据

#endif // GLOBAL_H



