#if !defined(CIRCULARLIST_H)
#define CIRCULARLIST_H
#include <assert.h>
struct SpaceUnitNode{
    int pos;
    SpaceUnitNode* next;
    SpaceUnitNode(int value):pos(value){}
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
    const SpacePieceNode* getNext(){return const_cast<const SpacePieceNode*>(next);}
    int getLen(){return len;}
};

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
    CircularSpacePiece(int size) :spaceSize(size), tolSpace(size), head(new Node(0, spaceSize)) {}

    int getDistance(int target, int base){//磁头(只能单向移动)。目标位置from相对于基准位置base的移动距离。
        return (target - base + spaceSize)%spaceSize;
    }

    //尝试把前面的节点nodeAhead与后面起始点为start，长度为len的节点合并。
    //如果合并成功，将会修改nodeAhead的长度。
    bool testMerge(Node* nodeAhead, int start, int len, bool allowOverlap){
        int aheadToStart = getDistance(start, nodeAhead->pos);
        //aheadToStart也有可能为0，说明start和nodeAhead->pos重合。
        //这里如果aheadToStart等于nodeAhead->len，说明start正好在nodeAhead范围之外
        //如果aheadToStart大于nodeAhead->len，则说明start已经超出了nodeAhead的范围。
        if(aheadToStart <= nodeAhead->len){
            if(aheadToStart < nodeAhead->len){
                if(allowOverlap){

                }else{
                    assert(false);//插入空白区域与已有空白区域发生重叠。错误。
                }
            }else if(aheadToStart == nodeAhead->len){
                nodeAhead->len = aheadToStart+len;
            }
            
            if(nodeAhead->len >= spaceSize){
                assert(false);//磁盘全都空出来了。待处理。
            }
            return true;
        }
        return false;
    }
    
    //根据start，插入+合并，如果插入的空白区域与已有空白区域重叠，则会报错。
    void dealloc(int start, int len) {
        if (head == nullptr) {
            Node* newNode = new Node(start, len);
            head = newNode;
            head->next = head;
        } else {
            Node* nodeAhead = getAheadNode(start);
            
            //这里可以确认start与headpos的距离>=nodeAhead与headpos的距离。放在nodeAhead之后。
            //首先确认start能否与nodeAhead合并
            Node* newNode;
            if(testMerge(nodeAhead, start, len, false)){
                newNode = nodeAhead;
            }
            else{
                Node* newNode = new Node(start, len);
            }
            //然后判断上一个节点（newNode或者合并了nodeAhead的newNode）能否与nodeAfter合并。
            Node* nodeAfter = nodeAhead->next;
            if(testMerge(newNode, nodeAfter->pos, nodeAfter->len, false)){
                nodeAhead->next = nodeAfter->next;//若newNode=nodeAhead，也没毛病。
                delete nodeAfter;
                nodeAfter = nodeAfter->next;
            }
            //最后插入节点。
            nodeAhead->next = newNode;//若newNode=nodeAhead，也没毛病。
            newNode->next = nodeAfter;
        }
        tolSpace += len;
    }
    //尝试分配空间，如果成功，将会修改空间节点。
    bool testAlloc(int start, int len){
        if (head == nullptr) {
            return false;
        } else if(head==head->next) {
            Node* nodeAhead = getAheadNode(start);
            int startToAhead = getDistance(start, nodeAhead->pos);
            if(startToAhead==0){//如果start与某个节点的起始点重合。
                if(nodeAhead->len > len){
                    nodeAhead->pos += start;
                    nodeAhead->pos %= spaceSize;
                    nodeAhead->len -= len;
                    tolSpace -= len;
                    return true;
                }else if(nodeAhead->len == len){
                    if(nodeAhead->next == nodeAhead){//说明nodeAhead就是唯一的头节点。
                        head = nullptr;
                        delete nodeAhead;
                        tolSpace -= len;
                        return true;//返回唯一的头节点。
                    }else{//nodeAhead还有后继。那么需要找到其前驱重新链表，然后返回nodeAhead。
                        Node* nodeBefore;
                        while(nodeBefore->next != nodeAhead){
                            nodeBefore = nodeBefore->next;
                        }
                        nodeBefore->next = nodeAhead->next;
                        delete nodeAhead;
                        tolSpace -= len;
                        return true;
                    }
                }else{//没有足够的空间来分配！
                    return false;
                }
            }else{
                //需要长度大于start终点与nodeAhead的距离。 如果等于，则刚好在nodeAhead范围之外。
                int endToAhead = getDistance((start+len)%spaceSize, nodeAhead->pos);
                if(nodeAhead->len == endToAhead-1){//刚好在尾部。
                    nodeAhead->len -= len;
                    tolSpace -= len;
                    return true;
                }else if(nodeAhead->len < endToAhead-1){//在中间，需要分裂节点。
                    Node* splitNode = new Node((start+len+1)%spaceSize, nodeAhead->len-endToAhead-1);
                    nodeAhead->len = startToAhead;
                    splitNode->next = nodeAhead->next;
                    nodeAhead->next = splitNode;
                    tolSpace -= len;
                    return true;
                }else{//start+len在nodeAhead空间之外。
                    return false;
                }
            }
        }
    }

    //获取start后的第一个空节点，rotate控制是否转动链表使得head指向返回节点(这样在别的地方用start调用getAheadNode就很快了)。
    const Node* getStartAfter(int start, bool rotate){
        Node* node = getAheadNode(start);
        if(rotate){
            this->head = node;
        }
        return node;//转换为const，防止改变内容。但是也可以从外部改变node的next？如何解决？
    }

    int getTolSpace(){return tolSpace;}

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

    // 销毁链表
    void destroy() {
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

    // 析构函数
    ~CircularSpacePiece() {
        destroy();
    }
};

// store position of requested obj unit on disk
// TO BE UPDATED: use red-black tree
// 问题在于，需要轮转。可以存储头结点的位置，然后通过
class CircularSpaceUnit{
public:
    typedef SpaceUnitNode Node;
    //需要包含磁头节点？如何把头节点和需要获取的节点区分开来？或者不包含，直接从外部获取,并且使用外部传参来轮转。
    Node* head;
    CircularSpaceUnit():head(nullptr){};
    
    void addReqUnit(int unitPos){

    }
    void rmReqUnit(int unitPos){

    }

};

#endif // CIRCULARLIST_H
