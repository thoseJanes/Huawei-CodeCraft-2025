#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "config.h"
#include "disk.h"
#include "object.h"
#include "request.h"
#include "util.h"
#include <string>
#include <algorithm>
#include "global_info.h"

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
        int id = _id[i];//删除的文件id

        std::vector<int> curNotDoneRequest;
        for(auto it = object[id].request_list.begin(); it != object[id].request_list.end(); it++){
            if(request[*it].is_done == false){
                curNotDoneRequest.push_back(*it);
            }
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
        object[id].request_list.clear();
    }

    fflush(stdout);
}



void write_action()
{
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++) {
        int id, size,tag;
        scanf("%d%d%d", &id, &size,&tag);
        object[id].tag = tag;
        object[id].size = size;
        object[id].is_delete = false;

        // 分配存储位置
        std::unordered_set<int> st;//记录已经分配磁盘的位置
        for (int j = 1; j <= REP_NUM; j++) {
            // 查询是否有充足的空间
            for(int k = 1; k <= N; k++) {
                //int cur_disk = (id + k + j) % N + 1; // id顺序分配
                int cur_disk = (tag + k + j) % N + 1; // tag顺序分配 -----有用-------
                if (st.find(cur_disk) != st.end()) {
                    continue;
                }
                if (disk[cur_disk].size + 10*size <= V) {
                    object[id].replica[j] = cur_disk;
                    break;
                }
            }
            // 没有充分的空间，那就找个占用最小的空间
            if(object[id].replica[j] == 0){
                int min_size = 1e9;
                for(int k = 1; k <= N; k++) {
                    if(st.find(k) != st.end()){
                        continue;
                    }
                    if (disk[k].size < min_size) {
                        min_size = disk[k].size;
                        object[id].replica[j] = k;
                    }
                }
            }


            st.insert(object[id].replica[j]);
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
    // 读取请求
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id);
        request[request_id].object_id = object_id;
        request_list.push_front(request_id); //记录全局请求队列
        //记录当前文件的最新请求
        object[object_id].request_list.push_front(request_id);
        request[request_id].is_done = false;
        request[request_id].start_time = timestamp;
    }

    std::unordered_set<int> done_request;

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

        disk[i].processRequest(request[disk[i].reqID], object[request[disk[i].reqID].object_id],done_request);

    }

    // 结算


    

    // 输出
    if (done_request.size() > 0) {
        printf("%d\n", done_request.size());
        for (auto it = done_request.begin(); it != done_request.end(); it++) {
            printf("%d\n", *it);
            //删除已经完成的请求
            if(request_map.find( *it) != request_map.end()){
                request_list.erase(request_map[ *it]); //其实已经在分配的时候删除了
                request_map.erase( *it);
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

    //前𝑚行中，第𝑖行第𝑗个数𝑓𝑟𝑒_𝑑𝑒𝑙[𝑖][𝑗]表示时间片编号𝑖𝑑满足 (𝑗 − 1) ∗ 1800 + 1 ≤ 𝑖𝑑 ≤ 𝑗 ∗1800的情况下，所有删除操作中对象标签为𝑖的对象大小之和

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }
    std::vector<int> tagSize(M+1,0);
    //接下来𝑚行，第𝑖行第𝑗个数𝑓𝑟𝑒_𝑤𝑟𝑖𝑡𝑒[𝑖][𝑗]表示时间片编号𝑖𝑑满足 (𝑗 − 1) ∗ 1800 + 1 ≤ 𝑖𝑑 ≤ 𝑗 ∗ 1800的情况下，所有写入操作中对象标签为𝑖的对象大小之和
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            int tmp;
            scanf("%d", &tmp);
            tagSize[i] += tmp;
        }
    }
    GlobalInfo globalInfo(std::move(tagSize));
    //接下来𝑚行，第𝑖行第𝑗个数𝑓𝑟𝑒_𝑟𝑒𝑎𝑑[𝑖][𝑗]表示时间片编号𝑖𝑑满足 (𝑗 − 1) ∗ 1800 + 1 ≤ 𝑖𝑑 ≤ 𝑗 ∗1800的情况下，所有读取操作中对象标签为𝑖的对象大小之和
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    printf("OK\n");
    fflush(stdout);

    //初始化磁盘
    disk = std::vector<Disk>(N+1, Disk(V, G, request, object, globalInfo));

    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        freshAll();
        timestamp = timestamp_action();
        delete_action();
        write_action();
        read_action();
    }

    return 0;
}