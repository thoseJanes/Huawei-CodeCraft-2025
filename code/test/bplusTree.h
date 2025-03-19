#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <memory>
#include <assert.h>
#include <stack>
//#include "tools/LogT"
#include "../tools/Logger.h"
#define LOG_BplusTree LOG_FILE("BplusTree")
#define LOG_BplusTreeInfo LOG_FILE("BplusTreeInfo")

template<int SIZE>
class BplusInnerNode;
template<int SIZE>
class BplusNode{
public:
    int keys[SIZE];
    bool isLeaf = false;
    char keyNum = 0;
    BplusInnerNode<SIZE>* parent;
public:
    BplusNode(bool leaf){
        assert(SIZE<255);
        isLeaf = leaf;
        keyNum = 0;
    }
    BplusNode(int key, bool leaf){
        assert(SIZE<255);
        keys[0] = key;
        isLeaf = leaf;
        keyNum = 1;
    }
    
    //找到第一个大于等于key的关键字位置。
    int searchKey(int key){
        for(int i=0;i<keyNum;i++){
            if(key<=keys[i]){
                return i;
            }
        }
        return keyNum;
    }

    int getMaxKey(){return keys[keyNum-1];}
    void setMaxKey(int key){keys[keyNum-1] = key;}
};


template<int SIZE>
class BplusInnerNode:public BplusNode<SIZE>{
public:
    BplusInnerNode(int key, BplusNode<SIZE>* child, bool isroot = false):
        isRoot(isroot), BplusNode<SIZE>::BplusNode(key, false)
        {children[0]=child;child->parent = this;}
    BplusInnerNode(bool isroot = false):isRoot(isroot),BplusNode<SIZE>::BplusNode(false){}
    BplusNode<SIZE>* children[SIZE];
    bool isRoot;
    //该函数专用于与节点分裂配合。由于分裂时已经设置了parent，因此这里不需要设置parent。
    bool addKeyAndChild(int key, BplusNode<SIZE>* oldchild, BplusNode<SIZE>* newchild){
        if((!isRoot) && this->keyNum>=SIZE){
            LOG_BplusTree << "keyNum out of range!";
            return false;
        }
        for(int i=0;i<this->keyNum;i++){
            if(key<this->keys[i]){
                memmove(this->keys+i+1, this->keys+i, (this->keyNum-i)*sizeof(int));
                memmove(children+i+2, children+i+1, (this->keyNum-i-1)*sizeof(BplusNode<SIZE>*));
                this->keys[i] = key;
                //children[i] = oldchild;//i处原本就是oldchild
                children[i+1] = newchild;
                this->keyNum += 1;
                return true;
            }else if(key==this->keys[i]){
                LOG_BplusTree << "error: duplicate key!";
                exit(0);
            }
        }
        LOG_BplusTree << "error: key out of range!";
        exit(0);
    }
    bool deleteKeyByPos(int pos, bool delChild=false){//删除时应当会借位。
        if((!isRoot) && this->keyNum <= (SIZE+1)/2){
            LOG_FILE("BplusTree") << "keyNum less than  (SIZE+1)/2+1!";
            return false;
        }
        if(this->keyNum <= pos||pos<0){
            LOG_FILE("BplusTree") << "error: pos out of range!";
            exit(0);
        }
        if(delChild){
            delete this->children[pos];
        }
        memmove(this->keys+pos, this->keys+pos+1, (this->keyNum-pos-1)*sizeof(int));
        memmove(children+pos, children+pos+1, (this->keyNum-pos-1)*sizeof(BplusNode<SIZE>*));
        this->keyNum -= 1;
        return true;
    }
    
    
    BplusInnerNode<SIZE>* splitNode(int key, BplusNode<SIZE>* oldchild, BplusNode<SIZE>* newchild){
        if(this->keyNum != SIZE){
            LOG_BplusTree << "key number less than SIZE";
        }
        BplusInnerNode<SIZE>* newNode = new BplusInnerNode<SIZE>();
        newNode->parent = this->parent;
        //找到key的位置
        int pos = 0;
        while(pos<this->keyNum){
            if(key>this->keys[pos]){
                pos++;
            }else if(key==this->keys[pos]){
                LOG_BplusTree << "duplicate key!";
                exit(0);
            }else{
                break;
            }
        }
        if(pos==this->keyNum){
            LOG_BplusTree << "error: newKey should be less than key of newchild!";
            exit(0);
        }
        children[pos+1] = newchild;
        //分给旧节点SIZE/2+1个，分给新节点(SIZE+1)/2个。
        int oldNum = SIZE/2+1;int newNum = (SIZE+1)/2;
        if(pos<oldNum){
            memcpy(newNode->keys, this->keys+oldNum-1, newNum*sizeof(int));
            memmove(this->keys+pos+1, this->keys+pos, sizeof(int)*(oldNum-1-pos));
            this->keys[pos] = key;

            memcpy(newNode->children, children+oldNum-1, newNum*sizeof(BplusNode<SIZE>*));
            memmove(children+pos+1, children+pos, sizeof(BplusNode<SIZE>*)*(oldNum-1-pos));
            children[pos] = oldchild;
        }else{
            memcpy(newNode->keys, this->keys+oldNum, sizeof(int)*(oldNum-pos));
            newNode->keys[oldNum-pos] = key;
            memcpy(newNode->keys+oldNum-pos+1, this->keys+oldNum*2-pos+1, sizeof(int)*(newNum-oldNum+pos-1));

            memcpy(newNode->children, children+oldNum, sizeof(BplusNode<SIZE>*)*(oldNum-pos));
            newNode->children[oldNum-pos] = oldchild;
            memcpy(newNode->children+oldNum-pos+1, children+oldNum*2-pos+1, sizeof(BplusNode<SIZE>*)*(newNum-oldNum+pos-1));
        }
        this->keyNum = oldNum;
        newNode->keyNum = newNum;
        return newNode;
    }
    //会把一切都搞定。或者什么都不做。
    bool lendKey(int posInParent, int delPos, bool delChild=true){
        BplusInnerNode<SIZE>* bro = nullptr;
        if(this->next->parent == this->parent){
            bro = this->next;
            int key; BplusInnerNode<SIZE>* child;
            key = bro->keys[0];
            child = bro->children[0];
            if(bro->deleteKeyByPos(0, false)){
                if(delChild){
                    delete children[delPos];
                }
                memmove(this->keys+delPos, this->keys+delPos+1, (this->keyNum-delPos-1)*sizeof(int));
                memmove(children+delPos, children+delPos+1, (this->keyNum-delPos-1)*sizeof(int));
                this->keys[this->keyNum-1] = key;
                children[this->keyNum-1] = child;
                this->parent->keys[posInParent] = key;
                return true;
            }
        }
        if(posInParent > 0){
            bro = this->parent->children[posInParent-1];
            int key; BplusInnerNode<SIZE>* child;
            key = bro->keys[bro->keyNum-1];
            child = bro->children[bro->keyNum-1];
            if(bro->deleteKeyByPos(bro->keyNum-1)){
                if(delChild){
                    delete children[delPos];
                }
                memmove(this->keys+1, this->keys, delPos*sizeof(int));
                memmove(children+1, children, delPos*sizeof(int));
                this->keys[0] = key;
                children[0] = child;
                this->parent->keys[posInParent-1] = bro->getMaxKey();
                this->parent->keys[posInParent] = this->getMaxKey();
                return true;
            }
        }
        return false;
    }
    //把所有东西都放入前面那个节点中，更新parent，然后返回后面节点在parent中的位置。
    //需要在外部删除parent的
    int mergeBro(int posInParent, int delPos, bool delChild=true){
        BplusInnerNode<SIZE>* bro = nullptr;
        if(delChild){
            delete children[delPos];
        }
        if(this->next->parent == this->parent){
            bro = this->next;
            if(delChild){
                delete this->children[delPos];
            }
            memmove(this->keys+delPos, this->keys+delPos+1, (this->keyNum-delPos-1)*sizeof(int));
            memcpy(this->keys+this->keyNum-1, bro->keys, (bro->keyNum)*sizeof(int));
            memmove(children+delPos, children+delPos+1, (this->keyNum-delPos-1)*sizeof(int));
            memcpy(children+this->keyNum-1, bro->children, (bro->keyNum)*sizeof(int));
            this->keyNum += bro->keyNum-1;
            this->parent->keys[posInParent] = this->getMaxKey();
            return posInParent+1;
        }else if(posInParent > 0){
            bro = this->parent->children[posInParent-1];
            if(delChild){
                delete this->children[delPos];
            }
            memcpy(bro->keys+bro->keyNum, this->keys, delPos*sizeof(int));
            memcpy(bro->keys+bro->keyNum+delPos, this->keys+delPos+1, (this->keyNum-delPos-1)*sizeof(int));
            memcpy(bro->children+bro->keyNum, children, delPos*sizeof(int));
            memcpy(bro->children+bro->keyNum+delPos, children+delPos+1, (this->keyNum-delPos-1)*sizeof(int));
            bro->keyNum += this->keyNum-1;
            this->parent->keys[posInParent-1] = bro->getMaxKey();
            return posInParent;
        }
    }
};

template<int SIZE>
class BplusLeafNode:public BplusNode<SIZE>{
public:
    BplusLeafNode():BplusNode<SIZE>::BplusNode(true){}
    BplusLeafNode(int key):BplusNode<SIZE>::BplusNode(key, true){}
    BplusLeafNode<SIZE>* next = nullptr;
    BplusLeafNode<SIZE>* splitNode(int key){
        if(this->keyNum != SIZE){
            LOG_BplusTree << "key number less than SIZE";
        }
        BplusLeafNode<SIZE>* newNode = new BplusLeafNode<SIZE>();
        //找到key的位置
        int pos = 0;
        while(pos<this->keyNum){
            if(key>this->keys[pos]){
                pos++;
            }else if(key==this->keys[pos]){
                LOG_BplusTree << "duplicate key!";
                exit(0);
            }else{
                break;
            }
        }
        //分给旧节点SIZE/2+1个，分给新节点(SIZE+1)/2个。
        //新节点是给父节点新的键的节点。因此新节点应该是键值更小的那个节点。
        int oldNum = SIZE/2+1;int newNum = (SIZE+1)/2;
        if(pos<newNum){
            memcpy(newNode->keys, this->keys+oldNum-1, newNum*sizeof(int));
            memmove(this->keys+pos+1, this->keys+pos, sizeof(int)*(oldNum-1-pos));
            this->keys[pos] = key;
        }else{
            memcpy(newNode->keys, this->keys+oldNum, sizeof(int)*(oldNum-pos));
            newNode->keys[oldNum-pos] = key;
            memcpy(newNode->keys+oldNum-pos+1, this->keys+oldNum*2-pos+1, sizeof(int)*(newNum-oldNum+pos-1));
        }
        this->keyNum = oldNum;
        newNode->keyNum = newNum;

        newNode->next = this->next;
        this->next = newNode;
        return newNode;
        
    }
    bool deleteKeyByPos(int pos){//删除时应当会借位。
        if(this->keyNum <= (SIZE+1)/2){//如果父节点为根，则没有此限制。
            if(!this->parent->isRoot){
                LOG_FILE("BplusTree") << "keyNum less than  (SIZE+1)/2+1!";
                return false;
            }
        }
        if(this->keyNum <= pos||pos<0){
            LOG_FILE("BplusTree") << "error: pos out of range!";
            return false;
        }
        memmove(this->keys+pos, this->keys+pos+1, (this->keyNum-pos-1)*sizeof(int));
        this->keyNum -= 1;
        return true;
    }
    bool addKey(int key){
        if(this->keyNum>=SIZE){
            LOG_BplusTree << "keyNum out of range!";
            return false;
        }
        for(int i=0;i<this->keyNum;i++){
            if(key<this->keys[i]){
                memmove(this->keys+i+1, this->keys+i, (this->keyNum-i)*sizeof(int));
                this->keys[i] = key;
                this->keyNum += 1;
                return true;
            }
        }
        this->keys[this->keyNum] = key;
        this->keyNum += 1;
        return true;
    }
    //会把一切都搞定。或者什么都不做。
    bool lendKey(int posInParent, int delPos){
        BplusLeafNode<SIZE>* bro = nullptr;
        if(next->parent == this->parent){
            bro = next;
            int key;
            key = bro->keys[0];
            if(bro->deleteKeyByPos(0)){
                memmove(this->keys+delPos, this->keys+delPos+1, (this->keyNum-delPos-1)*sizeof(int));
                this->keys[this->keyNum-1] = key;
                this->parent->keys[posInParent] = key;
                return true;
            }
        }
        if(posInParent > 0){
            bro = this->parent->children[posInParent-1];
            int key;
            key = bro->keys[bro->keyNum-1];
            if(bro->deleteKeyByPos(bro->keyNum-1)){
                memmove(this->keys+1, this->keys, delPos*sizeof(int));
                this->keys[0] = key;
                this->parent->keys[posInParent-1] = bro->getMaxKey();
                this->parent->keys[posInParent] = this->getMaxKey();
                return true;
            }
        }
        return false;
    }
    //把所有东西都放入前面那个节点中，更新parent，然后返回后面节点在parent中的位置。
    //需要在外部删除parent的
    int mergeBro(int posInParent, int delPos){
        BplusLeafNode<SIZE>* bro = nullptr;
        if(next->parent == this->parent){
            bro = next;
            memmove(this->keys+delPos, this->keys+delPos+1, (this->keyNum-delPos-1)*sizeof(int));
            memcpy(this->keys+this->keyNum-1, bro->keys, (bro->keyNum)*sizeof(int));
            this->keyNum += bro->keyNum-1;
            this->parent->keys[posInParent] = this->getMaxKey();
            //LOG_BplusTreeInfo << "merge node (" << 
            //此时，parent的posInParent+1上的key不正确，但是child正确。
            return posInParent+1;
        }else if(posInParent > 0){
            bro = this->parent->children[posInParent-1];
            memcpy(bro->keys+bro->keyNum, this->keys, delPos*sizeof(int));
            memcpy(bro->keys+bro->keyNum+delPos, this->keys+delPos+1, (this->keyNum-delPos-1)*sizeof(int));
            bro->keyNum += this->keyNum-1;
            this->parent->keys[posInParent-1] = bro->getMaxKey();
            return posInParent;
        }
    }
};

template<int SIZE>
class BplusTree{
public:
    typedef BplusNode<SIZE> Node;
    typedef BplusLeafNode<SIZE> LeafNode;
    typedef BplusInnerNode<SIZE> InnerNode;
    InnerNode* root;
    LeafNode* head;
    BplusTree(){
        root = nullptr;
    }
    void insert(int key){
        if(root == nullptr){
            head = new LeafNode(key);
            head->next = head;
            root = new InnerNode(key, head, true);//这里面会同时设置child和parent
        }else{
            //找到叶子节点
            Node* node = root;
            int pos;
            while(!node->isLeaf){
                pos = node->searchKey(key);
                if(pos == node->keyNum){
                    node->setMaxKey(key);
                    pos -= 1;
                }
                node = static_cast<InnerNode*>(node)->children[pos];
            }
            //给叶子节点插入key
            auto leafNode = static_cast<LeafNode*>(node);
            if(!leafNode->addKey(key)){//键已满
                auto newLeafNode = leafNode->splitNode(key);
                int newKey = leafNode->getMaxKey();
                InnerNode* parentNode = leafNode->parent;
                if(!parentNode->addKeyAndChild(newKey, leafNode, newLeafNode)){
                    splitInnerNode(key, node->parent, leafNode, newLeafNode);
                }
            }
        }
    }
    void splitInnerNode(int key, InnerNode* node, Node* oldchildNode, Node* newchildNode){
        auto newNode = node->splitNode(key, oldchildNode, newchildNode);
        int newKey = node->getMaxKey();
        if(node->isRoot){
            root = new InnerNode(true);
            node->isRoot = false;
            node->parent = root;
            newNode->parent = root;
            root->keyNum = 2;
            root->keys[0] = newKey;
            root->keys[1] = newNode->getMaxKey();
            root->children[0] = node;
            root->children[1] = newNode;
        }
        if(!node->parent->addKeyAndChild(newKey, node, newNode)){
            splitInnerNode(newKey, node->parent, node, newNode);
        }
    }
    
    void remove(int key){
        if(root == nullptr){
            LOG_BplusTree << "error: empty tree!";
            exit(0);
        }
        //找到叶子节点
        Node* node = root;
        std::stack<int> parentPos = {};
        //叶子节点在父节点中的位置
        int keyPos;//叶子节点的key位置
        while(!node->isLeaf){
            int pos = node->searchKey(key);
            parentPos.push(pos);
            if(pos == node->keyNum){
                LOG_BplusTree << "error: no key:" << key;
                exit(0);
            }
            node = static_cast<InnerNode*>(node)->children[pos];
        }
        keyPos = node->searchKey(key);
        if(node->keys[keyPos]!=key){
            LOG_BplusTree << "error: can't find key!";
            exit(0);
        }
        //删除叶子节点的key
        LeafNode* leafNode = static_cast<LeafNode*>(node);
        if(!leafNode->deleteKeyByPos(keyPos)){
            if(!leafNode->lendKey(parentPos.top(), keyPos)){
                int delPos = leafNode->mergeBro(parentPos.top(), keyPos);
                parentPos.pop();
                mergeInnerNode(leafNode->parent, delPos, parentPos);
            }
        }
        if(leafNode->keyNum==0){//父节点为根的情况
            int delPos = parentPos.top();
            parentPos.pop();
            mergeInnerNode(leafNode->parent, delPos, parentPos);
        }else{
            leafNode->parent->children[parentPos.top()] = leafNode->getMaxKey();
        }
    }
    void mergeInnerNode(InnerNode* node, int delPos, std::stack<int>& parentPos){
        if(node->isRoot){
            node->deleteKeyByPos(delPos, true);
        }
        if(!node->deleteKeyByPos(delPos, true)){
            if(!node->lendKey(parentPos.top(), delPos, true)){
                int parentDelPos = node->mergeBro(parentPos.top(), delPos, true);
                mergeInnerNode(node->parent, parentDelPos, parentPos);
            }
        }
    }
};

#endif