# pragma once
# include "config.h"
#include <cstring>
#include <vector>
#include <cassert>


class Disk {
public:

    Disk(int _v,int _G) :pos(1), size(0), unit(std::vector<int> (_v + 1, 0)),maxG(_G),preAction(-1) {}

    void do_object_delete(const std::vector<int>& object_unit)
    {
        for (int i = 1; i < object_unit.size(); i++) {//遍历删除
            unit[object_unit[i]] = 0;
        }
        size -= object_unit.size() - 1;
    }

    //object_unit 指定的obj存储在那个内存块
    void do_object_write(std::vector<int>& object_unit, int object_id)
    {
        int current_write_point = 0;
        int objsize = object_unit.size() - 1;
        //遍历找空位塞入
        for (int i = 1; i < unit.size(); i++) {
            if (unit[i] == 0) {
                unit[i] = object_id;
                object_unit[++current_write_point] = i;
                if (current_write_point == objsize) {
                    break;
                }
            }
        }

        assert(current_write_point == objsize);
        size += objsize;
    }

    void jump(int _pos)
    {
        pos = _pos;
        curG = 0;
        preAction = 0;
        preTocken = -1;
        printf("j %d\n", _pos);

    }

    void fresh(){
        curG = maxG;
    }

    int maxG; //最大G值
    int curG; //当前时间磁盘的G值
    int pos; //磁头位置
    int size; //已经占用的大小
    std::vector<int> unit; //每个单位存储的对象id
    int preAction; //上一次的操作, 0 跳 1 pass 2 读
    int preTocken; //上一次的令牌

};