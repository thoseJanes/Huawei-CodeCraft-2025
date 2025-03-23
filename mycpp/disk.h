# pragma once
# include "config.h"
#include <cstring>
#include <iterator>
#include <vector>
#include <cassert>

#include "request.h"
#include "object.h"
# include <string>
# include <unordered_set>
# include "global_info.h"

class Disk {
public:

    Disk(int _v,int _G, std::vector<Request>& _request, std::vector<Object>& _object, const GlobalInfo& _info) :pos(1), size(0), unit(std::vector<int> (_v + 1, 0)),obj_pos(std::vector<char> (_v+1,0)),
                        maxG(_G),preAction(-1),reqID(-1),V(_v), request(_request), object(_object),globalInfo(_info),tagBegins(std::vector<int>(_info.tagSize.size(),0)),curG(_G) {
                            float step = 1.0*V/_info.tagSizeSum;
                            for(int i = 1; i < _info.tagSize.size(); i++){
                                tagBegins[i] = tagBegins[i-1] + static_cast<int>(_info.tagSize[i-1]*step);
                            }
                        }

    void do_object_delete(const std::vector<int>& object_unit)
    {
        for (int i = 1; i < object_unit.size(); i++) {//遍历删除
            unit[object_unit[i]] = 0;
            obj_pos[object_unit[i]] = 0;
        }
        size -= object_unit.size() - 1;
    }

    //object_unit 指定的obj存储在那个内存块
    void do_object_write(std::vector<int>& object_unit, int object_id)
    {
        int current_write_point = 0;
        int objsize = object_unit.size() - 1;

        // // 只找位置连续够的位置 -------------------策略似乎没用-------------------
        // bool flag = false;
        // for(int i = 1; i <= V; i++){
        //     int shift_pos = (tagBegins[object[object_id].tag]+i) % V + 1;  //遍历找空位塞入， 从tagpos开始找
        //     if(unit[shift_pos] == 0){
        //         int cnt = 0;
        //         for(int j = 0; j < objsize; j++){
        //             if(unit[(shift_pos+j)%V+1] == 0){
        //                 cnt++;
        //             }
        //         }
        //         if(cnt == objsize){
        //             for(int j = 0; j < objsize; j++){
        //                 unit[(shift_pos+j)%V+1] = object_id;
        //                 object_unit[++current_write_point] = (shift_pos+j)%V+1;
        //                 obj_pos[(shift_pos+j)%V+1] = current_write_point;
        //             }
        //             flag = true;
        //             break;
        //         }
        //     }
        // }
        // // 如果没有找到位置就分散存储
        // if(!flag){
        //     for (int i = 1; i < unit.size(); i++) {
        //         int shift_pos = (pos + i - 1 + maxG/16) % V + 1;  //遍历找空位塞入， 从pos开始找
        //         //int shift_pos = (tagBegins[object[object_id].tag]+i) % V + 1;  //遍历找空位塞入， 从tagpos开始找
                
        //         if (unit[shift_pos] == 0) {
        //             unit[shift_pos] = object_id;
        //             object_unit[++current_write_point] = shift_pos;
        //             obj_pos[shift_pos] = current_write_point; //记录位置
        //             if (current_write_point == objsize) {
        //                 break;
        //             }
        //         }
        //     }
        // }

        for (int i = 1; i < unit.size(); i++) {
            int shift_pos = (pos + i + maxG/16) % V + 1;  //遍历找空位塞入， 从pos开始找， 这里纯调参
            //int shift_pos = (tagBegins[object[object_id].tag]+i) % V + 1;  //遍历找空位塞入， 从tagpos开始找
            
            if (unit[shift_pos] == 0) {
                unit[shift_pos] = object_id;
                object_unit[++current_write_point] = shift_pos;
                obj_pos[shift_pos] = current_write_point; //记录位置
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

    void processRequest(Request& req, Object& obj, std::unordered_set<int>& done_request){
        int dis = (obj.unit[replica_id][req.read_pos] - pos + V) % V+1;
        if (pos!=obj.unit[replica_id][req.read_pos] && dis>maxG-64) {//如果当前磁头位置不是要读取的位置 就跳转 ?
            jump(obj.unit[replica_id][req.read_pos]);
        } else {//是就一直读取
            // 直接读取
            std::string tmp = "";
            // 不考虑pass
            int cost = preAction == 2?  std::max(16, int(ceil(preTocken*0.8))):64;
            // 令牌够直接读取
            while(curG>=cost){
                //读取
                // 遍历该文件所有请求，设置已读该处


                int cur_obj_id = unit[pos];
                if(cur_obj_id != 0){//如果有数据
                    char cur_obj_pos = obj_pos[pos];

                    for(auto it = object[cur_obj_id].request_list.begin(); it !=  object[cur_obj_id].request_list.end(); it++){
                        bool is_done = request[*it].readData(cur_obj_pos, object[cur_obj_id]);
                        if(is_done){
                            done_request.insert(*it);
                            //同时断开连接
                            it = object[cur_obj_id].request_list.erase(it);
                            it = std::prev(it);
                        }
                    }

                }




                pos = pos%V + 1;
                curG -= cost;

                preTocken = cost;
                preAction = 2;
                tmp += "r";

                cost = std::max(16, int(ceil(preTocken*0.8)));//更新cost
            }
            tmp += "#\n";

            //如果读完了 置为闲置状态
            if(request[reqID].is_done){
                reqID = -1;
            }

            printf(tmp.c_str());
        }
    }



    int maxG; //最大G值
    int curG; //当前时间磁盘的G值
    int pos; //磁头位置
    int size; //已经占用的大小
    std::vector<int> unit; //每个单位存储的对象id
    std::vector<char> obj_pos; //每个单位存储的对象的位置id
    int preAction; //上一次的操作, 0 跳 1 pass 2 读
    int preTocken; //上一次的令牌
    int reqID; //正在处理的id 暂时每个磁盘只处理一个请求
    int replica_id; //正在处理的副本id
    const int V; //磁盘大小
    std::vector<Request>& request;
    std::vector<Object>& object;
    const GlobalInfo& globalInfo;
    private:
    std::vector<int> tagBegins;

};