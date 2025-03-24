#ifndef GLOBAL_H
#define GLOBAL_H

#include "tools/Logger.h"
#include "time.h"

#define LOG_IPCINFO LOG_FILE("ipcInfo")

#define MAX_DISK_NUM (10 + 1)//磁盘id从0开始
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)//id从1开始//4字节（32位）：范围是 [−2147483648,2147483647]
#define MAX_OBJECT_NUM (100000 + 1)//id从1开始
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)


#define FIRST_READ_CONSUME (64)

extern int T;//时间步
extern int M;//tag数，输入tag从1到M，内部从0到M-1
extern int N;//磁盘数，输入磁盘从1开始到N，内部磁盘从0到N-1
extern int V;//单元数，输入单元从1开始到V，内部单元用0到V-1表示
extern int G;//令牌数，

template<typename T>
LogStream& operator<<(LogStream& s, std::vector<T>& vec){
    s<<"(vec:";
    for(int i=0;i<vec.size();i++){
        s << vec[i]<<",";
    }
    s<<")";
    return s;
}



//模糊模式：对象数量并不精准契合统计数据
//精准模式：对象数量完美契合统计数据

#endif // GLOBAL_H



