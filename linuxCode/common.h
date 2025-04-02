#ifndef COMMONFUNC_H
#define COMMONFUNC_H
#include "watch.h"
/// @brief ͨ���ɵ�Tokens���Ѽ����µ�Tokens���ѣ��ῼ�ǵ�ʱ�䲽�Ľض�Ч����
/// @param lastTokenCost ��һ��Tokens����
/// @param value �ж����ѵ�Tokens
/// @param divisible �ж����ѵ�Tokens�Ƿ�ɷ֣�������pass������Ϊ������pass�ж����Կɷ֣��ɷ��򲻻ῼ��ʱ�䲽�Ľض�Ч��
/// @return �����µ�Tokens����
inline int calNewTokensCost(int lastTokenCost, int value, bool divisible) {
    if (Watch::toTimeStep(lastTokenCost) < Watch::getTime()) {
        lastTokenCost = (Watch::getTime() - 1) * G;
    }
    if (divisible) {
        return lastTokenCost + value;
    }
    else {
        int tolTokens = lastTokenCost;
        if ((tolTokens + value - 1) / G == tolTokens / G + 1) {//���������غ��ˡ�
            tolTokens = (tolTokens / G + 1) * G;//�����غϿ�ʼ����
        }
        tolTokens += value;
        return tolTokens;
    }
}
/// @brief ͨ���ɵ��ж�����λ�ü�����µ��ж�����λ��
/// @param lastHeadPos ��һ���ж��Ľ���λ��
/// @param value �ƶ���ֵ
/// @param isOffset �ƶ����Ǿ���ֵ�������ֵ
/// @param spaceSize 
/// @return �µ��ж�����λ��
inline int calNewHeadPos(int lastHeadPos, int value, bool isOffset, int spaceSize) {
    if (isOffset) {
        return (lastHeadPos + value) % spaceSize;
    }
    else {
        return value;
    }
}
/// @brief �������������λ���ڴ�ͷ�����ϵ���Ծ���
/// @param target Ŀ��λ��
/// @param from ��ͷ��ʼλ��
/// @param spaceSize �ռ��ܴ�С
/// @return ����λ��֮��ľ���
inline int getDistance(int target, int from, int spaceSize) {
    return (target - from + spaceSize) % spaceSize;
}




#endif
