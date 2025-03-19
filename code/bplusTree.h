#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <memory>
#include <assert.h>
#include "tools/Logger.h"
#define LOG_BplusTree LOG_FILE("BplusTree")


template<int SIZE>
class BplusInnerNode;
template<int SIZE>
class BplusNode{
public:
    int keys[SIZE];
    bool isLeaf = false;
    char keyNum = 0;
    BplusInnerNode<SIZE>* parent;
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
    bool addKey(int key){
        if(keyNum>=SIZE){
            LOG_BplusTree << "keyNum out of range!";
            return false;
        }
        for(int i=0;i<keyNum;i++){
            if(key<keys[i]){
                memmove(keys+i+1, keys+i, (keyNum-i)*sizeof(int));
                keys[i] = key;
                keyNum += 1;
                return true;
            }
        }
        keys[keyNum] = key;
        keyNum += 1;
        return true;
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
    bool deleteKey(int key){
        if(keyNum <= (SIZE+1)/2){
            LOG_FILE("BplusTree") << "keyNum less than ceil(m/2)!";
            return false;
        }
        for(int i=0;i<keyNum;i++){
            if(key==keys[i]){
                memmove(keys+i, keys+i+1, (keyNum-i-1)*sizeof(int));
                keyNum -= 1;
                return true;
            }
        }
        LOG_FILE("BplusTree") << "error: can't find key!";
        return false;
    }
    bool deleteKeyByPos(int pos){
        if(keyNum <= (SIZE+1)/2){
            LOG_FILE("BplusTree") << "keyNum less than ceil(m/2)!";
            return false;
        }
        if(keyNum <= pos||pos<0){
            LOG_FILE("BplusTree") << "error: pos out of range!";
            return false;
        }
        memmove(keys+pos, keys+pos+1, (keyNum-pos-1)*sizeof(int));
        keyNum -= 1;
        return true;
    }
    int getMaxKey(){return keys[keyNum-1];}
    void setMaxKey(int key){keys[keyNum-1] = key;}
};

template<int SIZE>
class BplusInnerNode:public BplusNode<SIZE>{
public:
    BplusInnerNode(int key, BplusNode<SIZE>* child):BplusNode(key, false){children[0]=child;child.parent = this;}
    BplusInnerNode():BplusNode(false){}
    BplusNode<SIZE>* children[SIZE];
    //该函数专用于与节点分裂配合。由于分裂时已经设置了parent，因此这里不需要设置parent。
    bool addKeyAndChild(int key, BplusNode<SIZE>* oldchild, BplusNode<SIZE>* newchild){
        if(keyNum>=SIZE){
            LOG_BplusTree << "keyNum out of range!";
            return false;
        }
        for(int i=0;i<keyNum;i++){
            if(key<keys[i]){
                memmove(keys+i+1, keys+i, (keyNum-i)*sizeof(int));
                memmove(children+i+2, children+i+1, (keyNum-i-1)*sizeof(BplusNode*));
                keys[i] = key;
                //children[i] = oldchild;//i处原本就是oldchild
                children[i+1] = newchild;
                keyNum += 1;
                return true;
            }else if(key==keys[i]){
                LOG_BplusTree << "error: duplicate key!";
                exit(0);
            }
        }
        LOG_BplusTree << "error: key out of range!";
        exit(0);
    }
    
    bool deleteKeyByPos(int pos) override{//删除时应当会借位。
        if(keyNum <= (SIZE+1)/2){
            LOG_FILE("BplusTree") << "keyNum less than  (SIZE+1)/2+1!";
            return false;
        }
        if(keyNum <= pos||pos<0){
            LOG_FILE("BplusTree") << "error: pos out of range!";
            return false;
        }
        memmove(keys+pos, keys+pos+1, (keyNum-pos-1)*sizeof(int));
        memmove(children+pos, children+pos+1, (keyNum-pos-1)*sizeof(BplusNode*));
        keyNum -= 1;
        return true;
    }
    
    
    BplusInnerNode<SIZE>* splitNode(int key, BplusNode<SIZE>* oldchild, BplusNode<SIZE>* newchild){
        if(keyNum != SIZE){
            LOG_BplusTree << "key number less than SIZE";
        }
        BplusInnerNode<SIZE>* newNode = new BplusInnerNode<SIZE>();
        newNode->parent = parent;
        //找到key的位置
        int pos = 0;
        while(pos<keyNum){
            if(key>keys[pos]){
                pos++;
            }else if(key==key[pos]){
                LOG_BplusTree << "duplicate key!";
                exit(0);
            }else{
                break;
            }
        }
        if(pos==keyNum){
            LOG_BplusTree << "error: newKey should be less than key of newchild!";
            exit(0);
        }
        children[pos+1] = newchild;
        //分给旧节点SIZE/2+1个，分给新节点(SIZE+1)/2个。
        int oldNum = SIZE/2+1;int newNum = (SIZE+1)/2;
        if(pos<oldNum){
            memcpy(newNode->keys, keys+oldNum-1, newNum*sizeof(int));
            memmove(keys+pos+1, keys+pos, sizeof(int)*(oldNum-1-pos));
            keys[pos] = key;

            memcpy(newNode->children, children+oldNum-1, newNum*sizeof(BplusNode*));
            memmove(children+pos+1, children+pos, sizeof(BplusNode*)*(oldNum-1-pos));
            children[pos] = oldchild;
        }else{
            memcpy(newNode->keys, keys+oldNum, sizeof(int)*(oldNum-pos));
            newNode->keys[oldNum-pos] = key;
            memcpy(newNode->keys+oldNum-pos+1, keys+oldNum*2-pos+1, sizeof(int)*(newNum-oldNum+pos-1));

            memcpy(newNode->children, children+oldNum, sizeof(BplusNode*)*(oldNum-pos));
            newNode->children[oldNum-pos] = oldchild;
            memcpy(newNode->children+oldNum-pos+1, children+oldNum*2-pos+1, sizeof(BplusNode*)*(newNum-oldNum+pos-1));
        }
        keyNum = oldNum;
        newNode->keyNum = newNum;
        return newNode;
    }
};

template<int SIZE>
class BplusLeafNode:public BplusNode<SIZE>{
public:
    BplusLeafNode():BplusNode(true){}
    BplusLeafNode(int key):BplusNode(key, true){}
    BplusLeafNode<SIZE>* next = nullptr;
    BplusLeafNode<SIZE>* splitNode(int key){
        if(keyNum != SIZE){
            LOG_BplusTree << "key number less than SIZE";
        }
        BplusLeafNode<SIZE>* newNode = new BplusLeafNode<SIZE>();
        //找到key的位置
        int pos = 0;
        while(pos<keyNum){
            if(key>keys[pos]){
                pos++;
            }else if(key==key[pos]){
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
            memcpy(newNode->keys, keys+oldNum-1, newNum*sizeof(int));
            memmove(keys+pos+1, keys+pos, sizeof(int)*(oldNum-1-pos));
            keys[pos] = key;
        }else{
            memcpy(newNode->keys, keys+oldNum, sizeof(int)*(oldNum-pos));
            newNode->keys[oldNum-pos] = key;
            memcpy(newNode->keys+oldNum-pos+1, keys+oldNum*2-pos+1, sizeof(int)*(newNum-oldNum+pos-1));
        }
        keyNum = oldNum;
        newNode->keyNum = newNum;

        newNode->next = this->next;
        this->next = newNode;
        return newNode;
        
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
            root = new InnerNode(key, head);//这里面会同时设置child和parent
        }else{
            //找到叶子节点
            Node* node = root;
            while(!node->isLeaf()){
                int pos = node->searchKey(key);
                if(pos == node->keyNum){
                    node->setMaxKey(key);
                    pos -= 1;
                }
                node = node->children[pos];
            }
            //给叶子节点插入key
            auto leafNode = dynamic_cast<LeafNode*>(node);
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
        if(node->parent == nullptr){
            root = new InnerNode();
            node->parent = root;
            newNode->parent = root;
            root->keyNum = 2;
            root->keys[0] = newKey;
            root->keys[1] = newNode->getMaxKey();
            root->children[0] = node;
            root->children[1] = newNode;
        }
        if(!node->parent->addKeyAndChild(newKey, node, newNode)){
            splitInnerNode(newKey, node.parent, node, newNode);
        }
    }
    
    void remove(int key){
        if(root == nullptr){
            LOG_BplusTree << "error: empty tree!";
            exit(0);
        }
        //找到叶子节点
        Node* node = root;
        while(!node->isLeaf()){
            int pos = node->searchKey(key);
            if(pos == node->keyNum){
                LOG_BplusTree << "error: no key:" << key;
                exit(0);
            }
            node = node->children[pos];
        }
        if(node->keys[pos]!=key){
            LOG_BplusTree << "error: can't find key!";
            exit(0);
        }
        //删除叶子节点的key
        auto leafNode = dynamic_cast<LeafNode*>(node);
        if(!leafNode->deleteKey(pos)){
        }

        if(leafNode->keyNum == 0){
            if(leafNode->next == leafNode){
                root = nullptr;
                delete leafNode;
                return;
            }
            InnerNode* parentNode = leafNode->parent;
            int pos = parentNode->searchKey(key);
            if(pos == parentNode->keyNum){
                parentNode->setMaxKey(leafNode->getMaxKey());
                pos -= 1;
            }
            if(pos == 0){
                parentNode->children[1]->next = leafNode->next;
            }else{
                parentNode->children[pos-1]->next = leafNode->next;
            }
            parentNode->deleteKeyByPos(pos);
            delete leafNode;
        }
    }
    void lendNode(){}

};






#endif