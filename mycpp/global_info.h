#pragma once

#include <utility>
#include <vector>
class GlobalInfo {
    public:
    GlobalInfo(std::vector<int>&& _tagsize):tagSize(std::move(_tagsize)){
        tagSizeSum = 0;
        for(auto i:tagSize){
            tagSizeSum += i;
        }
    }
    std::vector<int> tagSize;
    int tagSizeSum;
};