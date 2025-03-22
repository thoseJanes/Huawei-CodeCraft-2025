#include <cstdio>
#include <cassert>
#include <cstdlib>
#include "global.h"
#include "worker.h"

extern int T, M, N, V, G;

void start(){
    printf("OK\n");
    fflush(stdout);
}
int main()
{
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);

    Worker worker;
    worker.swallowStatistics();
    worker.initDisk();
    start();

    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        Watch::clock();
        worker.correctWatch();
        worker.freshDiskTokens();
        worker.clearOvertimeReq();

        worker.processDelete();
        worker.processWrite();
        worker.processRead();
    }
    //clean();

    return 0;
}