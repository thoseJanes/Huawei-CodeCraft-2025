#if !defined(GLOBAL_H)
#define GLOBAL_H

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)

extern int T, M, N, V, G;
extern int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
extern int disk_point[MAX_DISK_NUM];

#endif // GLOBAL_H



