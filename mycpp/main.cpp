#include <cmath>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
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
int timestamp = -1;

void delete_action()
{
    int n_delete;
    int abort_num = 0;
    static int _id[MAX_OBJECT_NUM];

    // 读取删除文件id
    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);
    }

    // 回溯删除文件的读请求
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {
            if (request[current_id].is_done == false) {
                abort_num++;
            }
            current_id = request[current_id].prev_id;
        }
    }

    // 打印结果
    printf("%d\n", abort_num);
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {
            if (request[current_id].is_done == false) {
                printf("%d\n", current_id);
            }
            current_id = request[current_id].prev_id;
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
        //当前请求对象的上一个请求，用作链表方便删除搜索
        request[request_id].prev_id = object[object_id].last_request_point;
        object[object_id].last_request_point = request_id;
        request[request_id].is_done = false;
        request[request_id].start_time = timestamp;
    }

    static int current_request = 0; //一直存在
    static int current_phase = 0;
    if (!current_request && n_read > 0) {
        //当前请求为最近的一个请求
        current_request = request_id;
    }
    if (!current_request) {//没有请求
        for (int i = 1; i <= N; i++) {
            printf("#\n");
        }
        printf("0\n");
    } else {
        current_phase++;
        object_id = request[current_request].object_id;
        for (int i = 1; i <= N; i++) {
            //遍历硬盘
            if (i == object[object_id].replica[1]) {//只读取第一个副本
                // if (current_phase % 2 == 1) {//奇数
                //     disk[i].jump(object[object_id].unit[1][current_phase / 2 + 1]);
                // } else {//偶数 读取
                //     disk[i].pos = disk[i].pos%V + 1;
                //     printf("r#\n");
                // }
                //printf("当前磁盘位置 %d, cur G %d \n", disk[i].pos, disk[i].curG);
                if (disk[i].pos!=object[object_id].unit[1][request[current_request].read_pos]) {//如果当前磁头位置不是要读取的位置 就跳转
                    disk[i].jump(object[object_id].unit[1][request[current_request].read_pos]);
                    //printf("跳转到 %d\n", object[object_id].unit[1][request[current_request].read_pos]);
                } else {//是就一直读取
                    // 直接读取
                    std::string tmp = "";
                    // 不考虑pass
                    int cost = disk[i].preAction == 2?  std::max(16, int(ceil(disk[i].preTocken*0.8))):64;
                    // 令牌够并且位置对应就读取
                    while(disk[i].curG>=cost && disk[i].pos == object[object_id].unit[1][request[current_request].read_pos]){
                        disk[i].pos = disk[i].pos%V + 1;
                        disk[i].curG -= cost;
                        request[current_request].read_pos++;
                        disk[i].preTocken = cost;
                        disk[i].preAction = 2;
                        tmp += "r";
                        //读取完毕
                        if(request[current_request].read_pos > object[object_id].size){
                            break;
                        }
                        cost = std::max(16, int(ceil(disk[i].preTocken*0.8)));//更新cost
                    }
                    tmp += "#\n";
                    printf(tmp.c_str());
                }


            } else {
                printf("#\n");
            }
        }

        if (request[current_request].read_pos > object[object_id].size) {
            if (object[object_id].is_delete) {//失败
                printf("0\n");
            } else {//成功
                printf("1\n%d\n", current_request);
                request[current_request].is_done = true;
            }
            current_request = 0;
            current_phase = 0;
        } else {
            printf("0\n");
        }
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