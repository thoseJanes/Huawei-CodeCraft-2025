#include "circularLinkedList.h"



bool CircularSpacePiece::testMerge(Node* nodeAhead, int start, int len, bool allowOverlap){
    int aheadToStart = getDistance(start, nodeAhead->pos);
    //aheadToStart也有可能为0，说明start和nodeAhead->pos重合。
    //这里如果aheadToStart等于nodeAhead->len，说明start正好在nodeAhead范围之外
    //如果aheadToStart大于nodeAhead->len，则说明start已经超出了nodeAhead的范围。
    if(aheadToStart <= nodeAhead->len){
        if(aheadToStart < nodeAhead->len){
            if(allowOverlap){
                nodeAhead->len = std::max(aheadToStart + len, nodeAhead->len);
            }else{
                throw std::logic_error("overlap insert interval!");//插入空白区域与已有空白区域发生重叠。错误。
            }
        }else if(aheadToStart == nodeAhead->len){
            nodeAhead->len = aheadToStart+len;
        }
        
        if(nodeAhead->len >= spaceSize){
            //assert(false);//磁盘全都空出来了。待处理。
            //删除除了nodeAhead外的所有节点
            auto tempNode = nodeAhead->next;
            while (tempNode != nodeAhead) {
                auto pNode = tempNode;
                tempNode = tempNode->next;
                delete pNode;
            }
            tempNode->next = tempNode;
            tempNode->pos = 0;
            tempNode->len = spaceSize;
        }
        return true;
    }
    return false;
}

void CircularSpacePiece::dealloc(int start, int len) {
    if (start < 0 || start >= spaceSize || len <= 0) {
        throw std::out_of_range("param out of range");
    }
    LOG_LINKEDSPACE << "dealloc:" << "(" << start << ", " << len << ")\n";
    if (head == nullptr) {
        Node* newNode = new Node(start, len);
        head = newNode;
        head->next = head;
    } else {
        Node* nodeAhead = getAheadNode(start);
        
        //这里可以确认start与headpos的距离>=nodeAhead与headpos的距离。放在nodeAhead之后。
        //首先确认start能否与nodeAhead合并
        Node* newNode;
        Node* nodeAfter = nodeAhead->next;
        if(testMerge(nodeAhead, start, len, false)){
            newNode = nodeAhead;
        }
        else{
            newNode = new Node(start, len);
            newNode->next = nodeAhead->next;
            nodeAhead->next = newNode;
        }
        //然后判断上一个节点（newNode或者合并了nodeAhead的newNode）能否与nodeAfter合并。
        if(newNode != nodeAfter && testMerge(newNode, nodeAfter->pos, nodeAfter->len, false)){
            nodeAhead->next = nodeAfter->next;//若newNode=nodeAhead，也没毛病。
            if (nodeAfter == head) {
                head = nodeAfter->next;
            }
            delete nodeAfter;
        }
    }
    tolSpace += len;
}

bool CircularSpacePiece::testAlloc(int start, int len){
        if (start < 0 || start >= spaceSize || len <= 0) {
            throw std::out_of_range("param out of range");
        }
        LOG_LINKEDSPACE << "testAlloc:" << "(" << start << ", " << len <<")" << "\n";
        if (head == nullptr) {//head是一个tag数长度的向量，如果某个tag没有被分配到该磁盘上，则为nullptr
            return false;
        } else{
            Node* nodeAhead = getAheadNode(start);
            int startToAhead = getDistance(start, nodeAhead->pos);
            if(startToAhead==0){//如果start与某个节点的起始点重合。
                if(nodeAhead->len > len){
                    nodeAhead->pos += len;
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
                        Node* nodeBefore = nodeAhead->next;
                        while(nodeBefore->next != nodeAhead){
                            nodeBefore = nodeBefore->next;
                        }
                        nodeBefore->next = nodeAhead->next;
                        if (nodeAhead == head) {
                            head = nodeAhead->next;
                        }
                        delete nodeAhead;
                        tolSpace -= len;
                        return true;
                    }
                }else if(start + len > spaceSize){//没有足够的空间来分配！
                    return false;
                }
            }else{
                //需要长度大于start终点与nodeAhead的距离。 如果等于，则刚好在nodeAhead范围之外。
                int endToAhead = getDistance((start+len)%spaceSize, nodeAhead->pos);
                if(nodeAhead->len == endToAhead || (nodeAhead->len==spaceSize&&endToAhead==0)){//刚好在尾部。
                    nodeAhead->len -= len;
                    tolSpace -= len;
                    return true;
                }else if(nodeAhead->len > endToAhead){//在中间，需要分裂节点。并且判断分裂之后的两边节点能否合并。
                    if (getDistance(nodeAhead->pos, (start + len + nodeAhead->len - endToAhead) % spaceSize) == 0) {
                        nodeAhead->pos = (start + len) % spaceSize;
                        nodeAhead->len = nodeAhead->len - endToAhead + startToAhead;
                        tolSpace -= len;
                    }else {
                        Node* splitNode = new Node((start + len) % spaceSize, nodeAhead->len - endToAhead);//后面的节点
                        nodeAhead->len = startToAhead;
                        splitNode->next = nodeAhead->next;
                        nodeAhead->next = splitNode;
                        tolSpace -= len;
                    }
                    return true;
                }else{//start+len在nodeAhead空间之外。
                    return false;
                }
            }
        }
        return false;
    }




