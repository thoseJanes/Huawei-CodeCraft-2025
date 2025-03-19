#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <memory>
#include <assert.h>
#include "tools/Logger.h"
#define LOG_BplusTree LOG_FILE("BplusTree")


template<int SIZE>
class BplusNode{
public:
    int keys[SIZE];
    bool isLeaf = false;
    char keyNum = 0;
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
    
};

template<int SIZE>
class BplusInnerNode:public BplusNode<SIZE>{
public:
    BplusInnerNode(int key, BplusNode<SIZE>* child):BplusNode(key, false){children[0]=child;}
    BplusNode<SIZE>* children[SIZE];
    bool deleteKey(int key) override{
        if(keyNum <= (SIZE+1)/2){
            LOG_FILE("BplusTree") << "keyNum less than ceil(m/2)!";
            return false;
        }
        for(int i=0;i<keyNum;i++){
            if(key==keys[i]){
                memmove(keys+i, keys+i+1, (keyNum-i-1)*sizeof(int));
                memmove(children+i, children+i+1, (keyNum-i-1)*sizeof(BplusNode*));
                keyNum -= 1;
                return true;
            }
        }
        LOG_FILE("BplusTree") << "error: can't find key!";
        return false;
    }
    bool deleteKeyByPos(int pos) override{
        if(keyNum <= (SIZE+1)/2){
            LOG_FILE("BplusTree") << "keyNum less than ceil(m/2)!";
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
    int getMaxKey(){return keys[keyNum-1];}
    void setMaxKey(int key){keys[keyNum-1] = key;}
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
        while(true){
            if(key>keys[pos]){
                pos++;
            }else if(key==key[pos]){
                LOG_BplusTree << "duplicate key!";
                exit(0);
            }else{
                break;
            }
        }
        for(int i=0;i<keyNum+1;i++){
            if(key<keys[i]&&i<(SIZE+1)/2){
                for(int j=0;j<SIZE/2;j++){
                    newNode->keys[j] = this->keys[j-1]
                }
            }
        }
        keys[keyNum] = key;
        keyNum += 1;
        return true;
        
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
            root = new InnerNode(key, head);
        }else{
            int pos = root->searchKey(key);
            if(pos == root->keyNum){
                root->setMaxKey(key);
                pos -= 1;
            }
            Node* node = root->children[pos];
            if(node->isLeaf()){
                if(!node->addKey(key)){//键已满
                    int r = node->searchKey(key);
                    Node* splitNode = new LeafNode(key);
                }
            }
        }
    }
    

};






#endif