# pragma once
# include "config.h"
#include <array>
#include <cstring>
#include <list>
#include <vector>


class Object {
public:
    Object() :size(0), is_delete(false),tag(0) {
    }

    std::vector<int> replica = std::vector<int>(REP_NUM + 1,0); //副本磁盘位置
    std::vector<std::vector<int>> unit = std::vector<std::vector<int>> (REP_NUM + 1); //副本每个单位内存位置 每个int*指向一个size+1内存数组指向内存位置,第二个是请求数量
    std::list<int> request_list; //最近请求的id
    std::array<int, 6> reqNum_; //每个单元请求数量 ，在来请求的时候增加，读取的时候清0
    bool is_delete; //是否删除
    char size; //对象大小
    char tag; //标记
    // 每次更新请求数量
    void updateReqNum() {
        for(int i = 1;i<=size;++i) {
            reqNum_[i]++;
        }
    }
} ;

