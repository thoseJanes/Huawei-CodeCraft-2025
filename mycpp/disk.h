# pragma once
# include "config.h"
#include <cstring>
#include <vector>
#include <cassert>

#include "request.h"
#include "object.h"
# include <string>

class Disk {
public:

    Disk(int _v,int _G) :pos(1), size(0), unit(std::vector<int> (_v + 1, 0)),maxG(_G),preAction(-1),reqID(-1),V(_v) {}

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
        //遍历找空位塞入， 从pos开始找
        for (int i = 1; i < unit.size(); i++) {
            int shift_pos = (pos + i - 1) % V + 1;
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

    void processRequest(Request& req, Object& obj){
        int dis = (obj.unit[replica_id][req.read_pos] - pos + V) % V;
        if (pos!=obj.unit[replica_id][req.read_pos] && dis>maxG-64) {//如果当前磁头位置不是要读取的位置 就跳转
            jump(obj.unit[replica_id][req.read_pos]);
            //printf("跳转到 %d\n", object[object_id].unit[1][request[current_request].read_pos]);
        } else {//是就一直读取
            // 直接读取
            std::string tmp = "";
            // 不考虑pass
            int cost = preAction == 2?  std::max(16, int(ceil(preTocken*0.8))):64;
            // 令牌够直接读取
            while(curG>=cost){
                if(pos == obj.unit[replica_id][req.read_pos]){
                    req.read_pos++;
                }
                pos = pos%V + 1;
                curG -= cost;

                preTocken = cost;
                preAction = 2;
                tmp += "r";
                //读取完毕
                if(req.read_pos > obj.size){
                    req.is_done = true;
                    break;
                }
                cost = std::max(16, int(ceil(preTocken*0.8)));//更新cost
            }
            tmp += "#\n";
            printf(tmp.c_str());
        }
    }



    int maxG; //最大G值
    int curG; //当前时间磁盘的G值
    int pos; //磁头位置
    int size; //已经占用的大小
    std::vector<int> unit; //每个单位存储的对象id
    int preAction; //上一次的操作, 0 跳 1 pass 2 读
    int preTocken; //上一次的令牌
    int reqID; //正在处理的id 暂时每个磁盘只处理一个请求
    int replica_id; //正在处理的副本id
    const int V; //磁盘大小

};