#ifndef READBLOCK_H
#define READBLOCK_H
#include <list>
#include "disk.h"
#include "common.h"
#include "bplusTree.h"
using namespace std;

struct ReadBlock{
    int startPos = 0;
    int blockLength = 0;
    int validLength = 0;
    int reqNum = 0;
    int edge = 0;
    #ifdef ENABLE_OBJECTSCORE
    int score = 0;
    #endif
    int tokensEnd = 0;
    //int tokensCost = 0;
    int lastReadLength = 0;
    bool inBlock(int key, int spaceSize){
        if(getDistance(key, startPos, spaceSize) < blockLength){
            return true;
        }
        return false;
    }
    bool inEffectRange(int key, int spaceSize){
        int dist = getDistance(key, startPos, spaceSize);
        if(dist >= blockLength && dist < blockLength+11){
            return true;
        }
        return false;
    }
};

// typedef BplusTree<4, ReadBlock> BlockTree;
// class ReadBlockManager:public BlockTree{
// public:
//     list<ReadBlock*> readBlockList;
//     Disk* disk;
//     ReadBlockManager(Disk* disk){
//         this->disk = disk;
//     }

//     //1/把新到的key按离某个基准点的远近距离排序
//     //2/按这一距离逐个更新？应当先更新前面的，并且在对象请求更新完毕后再更新连读块，
//     //  更新完连读块之后再放入key并判断key是不是在范围之内。但需要判断哪些连读块需要更新。也就是处于连读块中间的key。

//     //不分配磁盘，直接连接。
//     //规划的时候再分配磁盘，每5步规划一次，一次性规划105步？
//     //新进的某个块可能影响之后的时间，因此破坏规划，导致老的请求失效？这该怎么办？
    
//     void freshReadBlockWithNewKeys(std::vector<int> keys){//新放入的key
//         std::vector<BlockTree::Anchor> extendBlocks = {};
//         std::sort(keys.begin(), keys.end());
//         for(int i=0;i<keys.size();i++){//找到影响的block。某个key属于某个block可以从磁盘找到。
//             //新放入的key会影响它前面的block。除非该key在某个block里面。这种情况下也会影响这个block
//             //内部新增了key不会影响当前块的判定，只会影响之前块的判定，且需要之前块在当前块11步范围之内。
            
//             //从首向尾更新。首先对key从小到大排序。然后检查上一个key的关联anchor是不是下一个key的关联anchor。
//             //如果不是，那么就可以将这个anchor作为首，
//             int key = keys[i];
//             auto anchor = this->anchorAt(key); assert(key!=anchor.getKey());
//             anchor.toLast();
//             auto block = anchor.getValue();
//             if(anchor.getValue()->inEffectRange(key, disk->spaceSize)){
//                 extendReadBlock(anchor.getValue());
//                 assert(anchor.getKey()==anchor.getValue()->startPos);
//                 int keyDist = getDistance(anchor.getNext().getKey(), anchor.getKey(), disk->spaceSize);
//                 assert(keyDist != anchor.getValue()->blockLength);
//                 if(keyDist < anchor.getValue()->blockLength){
                    
//                 }
//             }else if(anchor.getValue()->inBlock(key, disk->spaceSize)){
//                 freshReadBlock(anchor.getValue(), key);
//             }else{
//                 insertReadBlock(key);
//             }
//         }
//     }
//     //从ReadBlock的尾部对block进行扩展
//     void extendReadBlock(ReadBlock* block) {
//         ReadBlock tempBlock;
//         tempBlock = *block;//复制
//         int tempPos = (tempBlock.startPos + tempBlock.blockLength)%disk->spaceSize;//当前是第一个未读位置
        
//         int multiReadLen = tempBlock.lastReadLength;
//         int multiReadTokensProfit = 0;
//         int invalidReadLen = 0;
//         DiskUnit unitInfo = this->disk->getUnitInfo(tempPos);
//         Object* obj = sObjectsPtr[unitInfo.objId];
//         while (invalidReadLen < 11) {//从tempPos开始，是否可以读原本不需要读的块来获取收益。
//             if (unitInfo.objId>0 && obj != deletedObject && obj != nullptr &&//该处有对象且未被删除
//                 obj->unitReqNum[unitInfo.untId] > 0 && //判断是否有请求
//                 (!obj->isPlaned(unitInfo.untId))) {
                
//                 multiReadTokensProfit = std::min(0,
//                     multiReadTokensProfit
//                     + getReadConsumeAfterN(multiReadLen)
//                     - getReadConsumeAfterN(tempBlock.blockLength));
//                 multiReadLen++;
//                 invalidReadLen = 0;

//                 tempBlock.tokensCost = calNewTokensCost(tempBlock.tokensCost, getReadConsumeAfterN(tempBlock.blockLength), false);
//                 tempBlock.blockLength++;
//                 tempBlock.reqNum += obj->unitReqNum[unitInfo.untId];
//                 tempBlock.validLength++;
//                 //obj->calUnitScoreAndEdge(unitInfo.untId, &tempBlock.score, &tempBlock.edge);
//             }
//             else {
//                 multiReadTokensProfit =
//                     multiReadTokensProfit
//                     - getReadConsumeAfterN(tempBlock.blockLength) + 1;
//                 invalidReadLen++;
//                 multiReadLen = 0;

//                 tempBlock.tokensCost = calNewTokensCost(tempBlock.tokensCost, getReadConsumeAfterN(tempBlock.blockLength), false);
//                 tempBlock.blockLength++;
//             }
//             if (multiReadTokensProfit >= 0) {//没有连读损耗
//                 *block = tempBlock;
//             }
//             tempPos = (tempPos + 1) % this->disk->spaceSize; //查看下一个位置是否未被规划。
//             unitInfo = disk->getUnitInfo(tempPos);
//             obj = sObjectsPtr[unitInfo.objId];
//         }
//     }
//     //在ReadBlock内部更新block。不需要更新时间，因为全读。只需要更新请求量。分数、边缘如何更新呢？还是临时计算？
//     //对于分数，每回合考虑到 对象删除/请求发起/请求超时/请求完成.然后用分数减去边缘即可。
//     //对于边缘，考虑到 
//         //对象删除（查询size*3个单元所属块，删除对应单元边缘，,可以在split时计算）
//         //请求发起（查询size*3个单元所属块，增加对应请求边缘）
//         //请求繁忙（拒绝式繁忙无须更新，中途繁忙需要查询size*3个单元所属块，减去对应请求边缘）
//         //单元确认（确认单元时删除对应单元边缘,）
//         //第几阶段（查询leftSize*3个单元所属块，根据请求更新）
//         //请求超时（查询leftSize*3个单元所属块，删除对应请求边缘）
//     //相比于直接记录req并查询，确实不清楚哪个花销高。可以先试试。
//     void freshReadBlock(ReadBlock* block, int newkey){
//         auto unitInfo = disk->getUnitInfo(newkey);
//         auto obj = sObjectsPtr[unitInfo.objId];

//         block->reqNum += obj->unitReqNum[unitInfo.untId];
//     };

//     void splitReadBlock(ReadBlock* block){//某个key被删除了，需要尝试将block分离成多个block

//     }
//     //如果是确定了使用某个副本，则需要删除另外两个副本。
//     //或者删除对象时。或者完成read单元时。
//     //注意流程：先确定副本，删除另外两个副本，然后read，删除剩余的副本
//     //如果删除对象，对于已确定副本的对象，只需要删除对应的一份key，对于未确定的副本，则需要删除三个key
//     //如何确定对象是否已确定副本？通过planReqDisk
//     void freshReadBlockWithDeletedKeys(std::vector<int> keys){

//     }

//     void insertReadBlock(int key){
//         //如果该key无法影响已存在的块，且不在块内,则将该key作为新块插入。
//         ReadBlock* newBlock = new ReadBlock;
//         newBlock->startPos = key;
//         newBlock->blockLength = 1;
//         newBlock->validLength = 1;
//         newBlock->lastReadLength = 1;
//         newBlock->tokensCost = 64;
//         insert(key, newBlock);
//     }
//     //根据加入的单元离哪个最大块近以及和哪个最大的块可以合并来决定单元分配到哪里读。
//     //如果某个单元的读时间掉出了105帧，则判断该单元能否在其它磁盘上被读取，若不能，则丢弃。
// };

#endif