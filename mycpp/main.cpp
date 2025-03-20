#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <list>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "disk.h"
#include "object.h"
#include "request.h"
#include "util.h"
#include <string>
#include <algorithm>

int T, M, N, V, G; //T时间 M对象标签数 N硬盘数 V硬盘大小 G 每个硬盘令牌数
std::vector<Request> request = std::vector<Request>(MAX_REQUEST_NUM);
std::vector<Object> object = std::vector<Object>(MAX_OBJECT_NUM);
std::vector<Disk> disk;
std::list<int> request_list;
std::unordered_map<int, std::list<int>::iterator> request_map;

int timestamp = -1;

void delete_action()
{
    int n_delete;
    int abort_num = 0;
    // 读取删除文件id
    scanf("%d", &n_delete);
    static int _id[MAX_OBJECT_NUM];
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);
    }
    
    std::vector<std::vector<int>> notDoneRequest;
    int accum_abort = 0;
    // 回溯删除文件的读请求
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        std::vector<int> curNotDoneRequest;
        while (current_id != 0) {
            if (request[current_id].is_done == false) {
                curNotDoneRequest.push_back(current_id);
            }
            current_id = request[current_id].prev_id;
        }
        notDoneRequest.push_back(curNotDoneRequest);
        accum_abort += curNotDoneRequest.size();
    }

    // 打印结果
    printf("%d\n", accum_abort);
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        for(int notDoneID: notDoneRequest[i-1]){
            printf("%d\n", notDoneID);
            if(request_map.find(notDoneID) != request_map.end()){
                request_list.erase(request_map[notDoneID]);
                request_map.erase(notDoneID);
            }
        }

        for (int j = 1; j <= REP_NUM; j++) {
            disk[object[id].replica[j]].do_object_delete(object[id].unit[j]);
        }
        object[id].is_delete = true;
    }

    fflush(stdout);
}



void write_action()
{
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++) {
        int id, size;
        scanf("%d%d%*d", &id, &size);
        object[id].last_request_point = 0;
        object[id].size = size;
        object[id].is_delete = false;

        // 分配存储位置
        for (int j = 1; j <= REP_NUM; j++) {
            // 查询是否有充足的空间
            for(int k = 1; k <= N; k++) {
                int cur_disk = (id + k + j) % N + 1;
                if (disk[cur_disk].size + size <= V) {
                    object[id].replica[j] = cur_disk;
                    break;
                }
            }

            object[id].unit[j] = std::vector<int> (size + 1, 0);
            // 分配具体位置
            disk[object[id].replica[j]].do_object_write(object[id].unit[j], id);
        }

        //输出
        printf("%d\n", id);
        for (int j = 1; j <= REP_NUM; j++) {
            printf("%d", object[id].replica[j]);
            for (int k = 1; k <= size; k++) {
                printf(" %d", object[id].unit[j][k]);
            }
            printf("\n");
        }
    }

    fflush(stdout);
}

void read_action()
{
    int n_read;
    int request_id, object_id;
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id);
        request[request_id].object_id = object_id;
        request_list.push_front(request_id);
        //当前请求对象的上一个请求，用作链表方便删除搜索，头插法
        request[request_id].prev_id = object[object_id].last_request_point;
        object[object_id].last_request_point = request_id;
        request[request_id].is_done = false;
        request[request_id].start_time = timestamp;
    }



    for (int i = 1; i <= N; i++) {
        //遍历硬盘
        if(disk[i].reqID==-1||request[disk[i].reqID].is_done){//如果硬盘上没有请求或者请求已经完成,分配请求
            //--------------------------------
            // 从请求队列中找到一个请求
            disk[i].reqID = -1;//防错

            if (request_list.size() == 0) {//请求队列为空
                printf("#\n");
                continue;
            }
            int cnt_req = 0;//计数遍历的请求数 我们只允许前100个请求
            for(auto it = request_list.begin(); it != request_list.end(); it++){
                cnt_req++;
                if(cnt_req > 100){
                    break;
                }
                // 请求的物体的某个副本在当前磁盘上就分配给磁盘
                int temp_obj_id = request[*it].object_id;
                for(int j = 1; j <= REP_NUM; j++){
                    if(object[temp_obj_id].replica[j] == i){
                        disk[i].reqID = *it;
                        disk[i].replica_id = j;
                        request_list.erase(it);
                        break;
                    }
                }
                if(disk[i].reqID!=-1){
                    break;
                }
            }
        }

        if(disk[i].reqID==-1){
            printf("#\n");
            continue;
        }

        disk[i].processRequest(request[disk[i].reqID], object[request[disk[i].reqID].object_id]);

    }

    // 结算
    std::vector<int> done_request;
    for(int i = 1; i <= N; i++){
        if(disk[i].reqID!=-1 && request[disk[i].reqID].is_done){
            if(object[request[disk[i].reqID].object_id].is_delete){//文件已经被删除了{
                disk[i].reqID = -1;
            }
            else{
                done_request.push_back(disk[i].reqID);
                disk[i].reqID = -1;
            }
        }
    }

    // 输出
    if (done_request.size() > 0) {
        printf("%d\n", done_request.size());
        for (int i = 0; i < done_request.size(); i++) {
            printf("%d\n", done_request[i]);
            //删除已经完成的请求
            if(request_map.find(done_request[i]) != request_map.end()){
                request_list.erase(request_map[done_request[i]]); //其实已经在分配的时候删除了
                request_map.erase(done_request[i]);
            }
        }
    } else {
        printf("0\n");
    }

    fflush(stdout);
}


void freshAll(){
    for (int i = 1; i <= N; i++) {
        disk[i].fresh();
    }
}


int main()
{
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    printf("OK\n");
    fflush(stdout);

    //初始化磁盘
    disk = std::vector<Disk>(N+1, Disk(V, G));

    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        freshAll();
        timestamp = timestamp_action();
        delete_action();
        write_action();
        read_action();
    }

    return 0;
}