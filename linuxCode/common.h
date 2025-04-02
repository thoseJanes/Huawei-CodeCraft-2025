#ifndef COMMONFUNC_H
#define COMMONFUNC_H
#include "watch.h"
/// @brief 通过旧的Tokens花费计算新的Tokens花费，会考虑到时间步的截断效果。
/// @param lastTokenCost 上一次Tokens花费
/// @param value 行动花费的Tokens
/// @param divisible 行动花费的Tokens是否可分，仅对以pass次数作为参数的pass行动而言可分，可分则不会考虑时间步的截断效果
/// @return 返回新的Tokens花费
inline int calNewTokensCost(int lastTokenCost, int value, bool divisible) {
    if (Watch::toTimeStep(lastTokenCost) < Watch::getTime()) {
        lastTokenCost = (Watch::getTime() - 1) * G;
    }
    if (divisible) {
        return lastTokenCost + value;
    }
    else {
        int tolTokens = lastTokenCost;
        if ((tolTokens + value - 1) / G == tolTokens / G + 1) {//如果操作跨回合了。
            tolTokens = (tolTokens / G + 1) * G;//跳到回合开始处。
        }
        tolTokens += value;
        return tolTokens;
    }
}
/// @brief 通过旧的行动结束位置计算出新的行动结束位置
/// @param lastHeadPos 上一步行动的结束位置
/// @param value 移动的值
/// @param isOffset 移动的是绝对值还是相对值
/// @param spaceSize 
/// @return 新的行动结束位置
inline int calNewHeadPos(int lastHeadPos, int value, bool isOffset, int spaceSize) {
    if (isOffset) {
        return (lastHeadPos + value) % spaceSize;
    }
    else {
        return value;
    }
}
/// @brief 计算磁盘上两个位置在磁头方向上的相对距离
/// @param target 目标位置
/// @param from 磁头起始位置
/// @param spaceSize 空间总大小
/// @return 两个位置之间的距离
inline int getDistance(int target, int from, int spaceSize) {
    return (target - from + spaceSize) % spaceSize;
}




#endif
