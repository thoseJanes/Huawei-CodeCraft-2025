# pragma once
# include "config.h"
#include <cstring>
#include <vector>

class Object {
public:
    Object() :size(0), last_request_point(-1), is_delete(false) {
    }

    std::vector<int> replica = std::vector<int>(REP_NUM + 1,0); //副本磁盘位置
    std::vector<std::vector<int>> unit = std::vector<std::vector<int>> (REP_NUM + 1); //副本每个单位内存位置 每个int*指向一个size+1内存数组指向内存位置
    int size; //对象大小
    int last_request_point; //最近请求的id
    bool is_delete; //是否删除
} ;

