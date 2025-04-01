#if !defined(CIRCULARLIST_H)
#define CIRCULARLIST_H
#include <assert.h>
#include "global.h"




struct SpaceUnitNode{
    int pos;
    SpaceUnitNode* next;
    SpaceUnitNode(int value):pos(value),next(nullptr){}
};

class SpacePieceNode{
private:
    int pos;
    int len;
    SpacePieceNode* next;
    //int tagStart;//取边缘的10个位置代表tag的主要成分？
    //int tagEnd;
    friend class CircularSpacePiece;
    friend class SpacePieceBlock;
public:
    SpacePieceNode(int value, int length) : pos(value), len(length), next(nullptr) {}
    const SpacePieceNode* getNext() const {return const_cast<const SpacePieceNode*>(next);}
    int getLen() const {return len;}
    int getStart() const {return pos;}
};

LogStream& operator<<(LogStream& s, const SpacePieceNode& node);
// store free space of disk.
// TO BE UPDATED: use red-black tree
class CircularSpacePiece {
private:
    typedef SpacePieceNode Node;
    Node* head;
    int spaceSize;
    int tolSpace;
    
    //如果链表中节点起始位置没有重复，那么返回的aheadNode与head的距离会 刚好 小于等于 start与head的距离。
    Node* getAheadNode(int start){
        int headpos = head->pos;
        int distance = getDistance(start, headpos);
        Node* nodeAhead = head;
        if(nodeAhead->next != head){
            while(distance >= getDistance(nodeAhead->next->pos, headpos)
                    &&  nodeAhead->next!=head){
                nodeAhead = nodeAhead->next;
            }
        }
        // else{
        //     nodeAhead = nodeAhead;//nodeAhead的位置即headpos
        // }
        return nodeAhead;
    }
    
public:
    CircularSpacePiece(int size) :spaceSize(size), tolSpace(size), head(new Node(0, size)) {
        head->next = head;//循环
    }

    const Node* getHead() const { return head; }
    int getDistance(int target, int base){//磁头(只能单向移动)。目标位置from相对于基准位置base的移动距离。
        return (target - base + spaceSize)%spaceSize;
    }
    const Node* getStartAfter(int start, bool rotate){
        Node* node = getAheadNode(start);
        if(rotate){
            this->head = node;
        }
        return node;//转换为const，防止改变内容。但是也可以从外部改变node的next？如何解决？
    }
    int getTolSpace(){return tolSpace;}

    
    //尝试把前面的节点nodeAhead与后面起始点为start，长度为len的节点合并。
    //如果合并成功，将会修改nodeAhead的长度。
bool testMerge(Node* nodeAhead, int start, int len, bool allowOverlap);

//根据start，插入+合并，如果插入的空白区域与已有空白区域重叠，则会报错。
void dealloc(int start, int len);
//尝试分配空间，如果成功，将会修改空间节点。
bool testAlloc(int start, int len);


//获取start后的第一个空节点，rotate控制是否转动链表使得head指向返回节点(这样在别的地方用start调用getAheadNode就很快了)。

// // 打印链表
// void print() {
//     if (head == nullptr) {
//         std::cout << "Circular Linked List is empty." << std::endl;
//         return;
//     }
//     Node* temp = head;
//     do {
//         std::cout << temp->data << " ";
//         temp = temp->next;
//     } while (temp != head);
//     std::cout << std::endl;
// }

// 析构函数
~CircularSpacePiece() {
    if (head == nullptr) return;

    Node* current = head;
    Node* next = nullptr;
    do {
        next = current->next;
        delete current;
        current = next;
    } while (current != head);

    head = nullptr;
}
};

LogStream& operator<<(LogStream& s, CircularSpacePiece& linkedList);


struct SpacePiece {
    int start;
    int len;
};

enum StorageMode {
    StoreFromFront,
    StoreFromEnd,
};
class SpacePieceBlock {//无双向计算。
private:
    friend LogStream& operator<<(LogStream& s, SpacePieceBlock& space);
    typedef SpacePiece Node;
    typedef std::list<Node>::iterator NodeIt;
    std::list<Node> spaceNodes = {};
    const int startPos;
    const int size;
    int res;
    std::pair<int, int> tagPair;

    //返回的节点的start小于等于start
    NodeIt getNodeFromEnd(int start) {
        auto it = spaceNodes.end(); it--;
        while (it != spaceNodes.begin() && (*it).start > start) {
            it--;
        }
        if (it == spaceNodes.begin() && (*it).start > start) {
            throw std::logic_error("can't find free space");
        }
        return it;
    }
    //返回的节点的结束位置(末尾+1)大于start
    NodeIt getNodeFromFront(int start) {
        auto it = spaceNodes.begin();
        while (it != spaceNodes.end() && (*it).len + (*it).start <= start) {
            it++;
        }
        if (it == spaceNodes.end()) {
            throw std::logic_error("can't find free space");
        }
        return it;
    }
public:
    SpacePieceBlock(int start, int size, std::pair<int,int> tagPair) :tagPair(tagPair), startPos(start), size(size), res(size) {
        spaceNodes.push_back({ start, size });
    }
    int getResidualSize() { return res; }
    int getTolSpaceSize() { return size; }
    std::pair<int, int> getTagPair() { return tagPair; }
    bool inSpace(int start) {
        if (start >= startPos && start < startPos + size) {
            return true;
        }
        return false;
    }
    std::list<Node>* getNodes() {
        return &spaceNodes;
    }
    //根据start，插入+合并，如果插入的空白区域与已有空白区域重叠，则会报错。
    void deAlloc(int start, int len, StorageMode mode) {
        res += len;
        NodeIt it;
        if (spaceNodes.size() == 0) {
            spaceNodes.insert(spaceNodes.begin(), { start, len });
            return;
        }
        if (mode == StoreFromFront) {
            if (spaceNodes.back().start + spaceNodes.back().len <= start) {
                it = spaceNodes.insert(spaceNodes.end(), { start, len });
            }
            else {
                it = getNodeFromFront(start);
                assert((*it).start > start);
                it = spaceNodes.insert(it, { start, len });
            }
        }
        else {
            if (spaceNodes.front().start > start) {
                assert(spaceNodes.front().start >= start + len);
                it = spaceNodes.insert(spaceNodes.begin(), { start, len });
            }
            else {
                it = getNodeFromEnd(start);
                assert((*it).start + (*it).len <= start);
                it = spaceNodes.insert(std::next(it), { start, len });
            }
        }
        if (it != spaceNodes.begin()) {//有前置节点。
            it--;
            int nodeEnd = (*it).start + (*it).len;
            if (nodeEnd == start) {//前置节点尾部等于当前节点开头
                (*it).len += len;
                nodeEnd = (*it).start + (*it).len;
                spaceNodes.erase(std::next(it));
            }
        }

        int nodeEnd = (*it).start + (*it).len;
        auto nextNode = std::next(it);
        if (nextNode != spaceNodes.end() && nodeEnd == (*nextNode).start) {
            (*it).len += (*nextNode).len;
            spaceNodes.erase(nextNode);
        }

    }
    //尝试分配空间，如果成功，将会修改空间节点。
    void alloc(int start, int len, StorageMode mode) {
        res -= len;
        NodeIt it;
        if (mode == StoreFromFront) {
            it = getNodeFromFront(start);
        }
        else {
            it = getNodeFromEnd(start);
        }

        if ((*it).len + (*it).start < start + len){
            throw std::logic_error("no space available");
        }
        int frontGap = start - (*it).start;
        int backGap = (*it).len + (*it).start - (start + len);
        int backStart = start + len;
        assert(frontGap >= 0); assert(backGap >= 0);
        if (frontGap != 0) {
            spaceNodes.insert(it, { (*it).start, frontGap });
        }
        if (backGap != 0) {
            spaceNodes.insert(it, { backStart, backGap });
        }
        spaceNodes.erase(it);
    }

    ~SpacePieceBlock() {
    }
};

LogStream& operator<<(LogStream& s, SpacePieceBlock& linkedList);


#endif // CIRCULARLIST_H
