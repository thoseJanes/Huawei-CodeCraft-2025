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



#endif // CIRCULARLIST_H
