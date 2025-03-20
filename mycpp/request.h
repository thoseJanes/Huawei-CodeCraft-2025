# pragma once
#include "object.h"
#include <bitset>

class Request {
    
public:    
    
    Request():object_id(-1),prev_id(-1),is_done(false),data() {}
    
    int object_id; //请求的对象id
    int prev_id; //该对象的上一个请求id
    bool is_done; //是否完成
    int start_time; //请求开始时间
    std::bitset<8> data; //记录已经读取的数据
    int read_pos = 1; //读取位置
    // 读取数据,返回是否读完整个对象
    bool readData(int pos,Object& obj){
        data.set(pos); //设置pos位置为1
        if(data.count()>=obj.size){
            is_done = true;
            return true;
        }

        // 找到下一个未读的位置，也就是第二个0的位置
        for(int i  = 1; i<data.size(); i++){
            if(data[i] == 0){
                read_pos = i;
                break;
            }
        }


        return false;
    }


} ;