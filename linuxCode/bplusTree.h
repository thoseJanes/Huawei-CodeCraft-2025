#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <memory>
#include <cstdlib>
#include <assert.h>
#include <stack>
#include <exception>
#include "global.h"


template<int SIZE>
class BplusInnerNode;
template<int SIZE>
class BplusNode {
public:
    int keys[SIZE];
    bool isLeaf = false;
    char keyNum = 0;
    BplusInnerNode<SIZE>* parent;
public:
    BplusNode(bool leaf) {
        assert(SIZE < 255);
        isLeaf = leaf;
        keyNum = 0;
    }
    BplusNode(int key, bool leaf) {
        assert(SIZE < 255);
        keys[0] = key;
        isLeaf = leaf;
        keyNum = 1;
    }

    //找到第一个大于等于key的关键字位置。
    int searchKey(int key) {
        for (int i = 0; i < keyNum; i++) {
            if (keys[i] >= key) {
                return i;
            }
        }
        return keyNum;
    }
    int getPosInParent() {
        if (this->parent != nullptr) {
            BplusInnerNode<SIZE>* parentNode = this->parent;
            int parentKeyNum = parentNode->keyNum;
            for (int i = 0; i < parentKeyNum; i++) {
                if (parentNode->children[i] == this) {
                    return i;
                }
            }
            throw std::logic_error("Can't find this node in its parent!");
        }
        return -1;
    }
    int getMaxKey() { return keys[keyNum - 1]; }
    void setMaxKey(int key) { keys[keyNum - 1] = key; }
    bool isLastChild() {
        if (this->parent) {
            return this->parent->children[this->parent->keyNum - 1] == this;
        }
        return false;
    }
    bool isFull() {
        if (this->keyNum >= SIZE) {
            LOG_BplusTree << "keyNum out of range!";
            return true;
        }
        return false;
    }
};

template<int SIZE>
inline LogStream& operator<<(LogStream& s, const BplusNode<SIZE>& v)
{
    s << "(" << static_cast<int>(v.keyNum) << ":";
    for (int i = 0; i < v.keyNum; i++) {
        s << v.keys[i] << " ";
    }
    s << ")";
    return s;
}

template<int SIZE>
class BplusInnerNode :public BplusNode<SIZE> {
public:
    BplusInnerNode(int key, BplusNode<SIZE>* child) :
         BplusNode<SIZE>::BplusNode(key, false)
    {
        children[0] = child; child->parent = this;
    }
    BplusInnerNode() :BplusNode<SIZE>::BplusNode(false) {}
    BplusNode<SIZE>* children[SIZE];
    //该函数专用于与节点分裂配合。由于分裂时已经设置了parent，因此这里不需要设置parent。
    bool addKeyAndChild(int key, BplusNode<SIZE>* oldchild, BplusNode<SIZE>* newchild) {
        if (this->keyNum >= SIZE) {
            LOG_BplusTree << "keyNum out of range!";
            return false;
        }
        for (int i = 0; i < this->keyNum; i++) {
            if (key < this->keys[i]) {
                memmove(this->keys + i + 1, this->keys + i, (this->keyNum - i) * sizeof(int));
                memmove(children + i + 2, children + i + 1, (this->keyNum - i - 1) * sizeof(BplusNode<SIZE>*));
                this->keys[i] = key;
                //children[i] = oldchild;//i处原本就是oldchild
                children[i + 1] = newchild;
                this->keyNum += 1;
                return true;
            }
            else if (key == this->keys[i]) {
                LOG_BplusTree << "error: duplicate key!";
                throw std::logic_error("Duplicate key when adding key to inner node.");
            }
        }
        LOG_BplusTree << "error: key out of range!";
        throw std::domain_error("Param key is bigger than any other key, \
                but the max key should have been set to no shorter than input key on insertion.");
    }
    bool addKeyAndChildByPos(int pos, int key, BplusNode<SIZE>* child) {
        if (key == this->keys[pos]) {
            LOG_BplusTree << "fail: duplicate key!";
            return false;
        }
        memmove(this->keys + pos + 1, this->keys + pos, (this->keyNum - pos) * sizeof(int));
        memmove(children + pos + 1, children + pos, (this->keyNum - pos) * sizeof(BplusNode<SIZE>*));
        this->keys[pos] = key;
        //children[i] = oldchild;//i处原本就是oldchild
        children[pos] = child;
        this->keyNum += 1;
        return true;
    }
    bool deleteKeyByPos(int pos, bool delChild = false) {//删除时应当会借位。
        if (this->parent&&this->keyNum <= (SIZE + 1) / 2) {
            LOG_FILE("BplusTree") << "keyNum less than  (SIZE+1)/2+1!";
            return false;
        }
        if (this->keyNum <= pos || pos < 0) {
            LOG_FILE("BplusTree") << "error: pos out of range!";
            throw std::out_of_range("Param pos is out of valid range");
        }
        if (delChild) {
            delete this->children[pos];
        }
        memmove(this->keys + pos, this->keys + pos + 1, (this->keyNum - pos - 1) * sizeof(int));
        memmove(children + pos, children + pos + 1, (this->keyNum - pos - 1) * sizeof(BplusNode<SIZE>*));

        this->keyNum -= 1;
        if (this->keyNum == pos && this->parent) {
            freshUpOnDelete(this->keyNum - 1);
        }
        return true;
    }
    void freshUpOnDelete(int freshPos) {
        auto presentNode = this;
        BplusInnerNode<SIZE>* parentNode = nullptr;
        int posInParent;
        while(freshPos == presentNode->keyNum - 1) {
            posInParent = presentNode->getPosInParent();
            parentNode = presentNode->parent;
            if (posInParent >= 0) {
                parentNode->keys[posInParent] = presentNode->getMaxKey();
                presentNode = parentNode;
                freshPos = posInParent;
            }
            else {
                break;
            }
        }
    }
    BplusInnerNode<SIZE>* splitNode(int pos, int key, BplusNode<SIZE>* oldchild, int posInParent) {
        if (this->keyNum != SIZE) {
            LOG_BplusTree << "key number less than SIZE";
        }
        LOG_BplusTreeInfo << "in adding key " << key
            << ", " << *this << " split";
        BplusInnerNode<SIZE>* newNode = new BplusInnerNode<SIZE>();
        //需要快点为其分配parent，防止和root混淆。deleteKeyByPos中会根据有没有parent判断节点数是否能降低到0
        newNode->parent = this->parent;
        //分给旧节点SIZE/2+1个，分给新节点(SIZE+1)/2个。
        int oldNum = SIZE / 2 + 1; int newNum = (SIZE + 1) / 2;
        if (pos < oldNum) {
            memcpy(newNode->keys, this->keys + oldNum - 1, newNum * sizeof(int));
            memmove(this->keys + pos + 1, this->keys + pos, sizeof(int) * (oldNum - 1 - pos));
            this->keys[pos] = key;

            memcpy(newNode->children, children + oldNum - 1, newNum * sizeof(BplusNode<SIZE>*));
            memmove(children + pos + 1, children + pos, sizeof(BplusNode<SIZE>*) * (oldNum - 1 - pos));
            children[pos] = oldchild;
        }
        else {
            memcpy(newNode->keys, this->keys + oldNum, sizeof(int) * (pos - oldNum));
            newNode->keys[pos - oldNum] = key;
            memcpy(newNode->keys + pos - oldNum + 1, this->keys + pos, sizeof(int) * (newNum + pos - oldNum - 1));

            memcpy(newNode->children, children + oldNum, sizeof(BplusNode<SIZE>*) * (pos - oldNum));
            newNode->children[pos - oldNum] = oldchild;
            memcpy(newNode->children + pos - oldNum + 1, children + pos, sizeof(BplusNode<SIZE>*) * (newNum + pos - oldNum - 1));
        }
        this->keyNum = oldNum;
        newNode->keyNum = newNum;
        
        for (int i = 0; i < newNode->keyNum; i++) {
            newNode->children[i]->parent = newNode;
        }
        if (this->parent!=nullptr) {
            this->parent->children[posInParent] = newNode;
        }
        LOG_BplusTreeInfo << "split inner node: " << *this
            << ", " << *newNode;
        return this;
    }
    //会把一切都搞定。或者什么都不做。
    bool lendKey(int posInParent, int delPos, bool delChild = true) {
        BplusInnerNode<SIZE>* bro = nullptr;
        if (posInParent < this->parent->keyNum - 1) {
            bro = static_cast<BplusInnerNode<SIZE>*>(this->parent->children[posInParent + 1]);
            int key; BplusNode<SIZE>* child;
            key = bro->keys[0];
            child = bro->children[0];
            if (bro->deleteKeyByPos(0, false)) {
                if (delChild) {
                    delete children[delPos];
                }
                memmove(this->keys + delPos, this->keys + delPos + 1, (this->keyNum - delPos - 1) * sizeof(int));
                memmove(children + delPos, children + delPos + 1, (this->keyNum - delPos - 1) * sizeof(BplusInnerNode<SIZE>*));
                this->keys[this->keyNum - 1] = key;
                children[this->keyNum - 1] = child;
                child->parent = this;
                this->parent->keys[posInParent] = key;
                return true;
            }
        }
        if (posInParent > 0) {
            bro = static_cast<BplusInnerNode<SIZE>*>(this->parent->children[posInParent - 1]);
            int key; BplusNode<SIZE>* child;
            key = bro->keys[bro->keyNum - 1];
            child = bro->children[bro->keyNum - 1];
            if (bro->deleteKeyByPos(bro->keyNum - 1)) {
                if (delChild) {
                    delete children[delPos];
                }
                memmove(this->keys + 1, this->keys, delPos * sizeof(int));
                memmove(children + 1, children, delPos * sizeof(BplusInnerNode<SIZE>*));
                this->keys[0] = key;
                children[0] = child;
                child->parent = this;
                this->parent->keys[posInParent - 1] = bro->getMaxKey();
                this->parent->keys[posInParent] = this->getMaxKey();
                this->parent->freshUpOnDelete(posInParent);
                return true;
            }
        }
        return false;
    }
    //把所有东西都放入前面那个节点中，更新parent，然后返回后面节点在parent中的位置。
    //需要在外部删除parent的
    int mergeBro(int posInParent, int delPos, bool delChild = true) {
        BplusInnerNode<SIZE>* bro = nullptr;
        if (delChild) {
            delete children[delPos];
        }
        if (posInParent < this->parent->keyNum - 1) {
            bro = static_cast<BplusInnerNode<SIZE>*>(this->parent->children[posInParent + 1]);
            LOG_BplusTreeInfo << "in deleting key " << this->keys[delPos]
                << ", " << "merge inner node " << *bro << " to " << *this;
            memmove(this->keys + delPos, this->keys + delPos + 1, (this->keyNum - delPos - 1) * sizeof(int));
            memcpy(this->keys + this->keyNum - 1, bro->keys, (bro->keyNum) * sizeof(int));
            memmove(children + delPos, children + delPos + 1, (this->keyNum - delPos - 1) * sizeof(BplusInnerNode<SIZE>*));
            memcpy(children + this->keyNum - 1, bro->children, (bro->keyNum) * sizeof(BplusInnerNode<SIZE>*));
            for (int i = 0; i < bro->keyNum; i++) {
                bro->children[i]->parent = this;
            }
            this->keyNum += bro->keyNum - 1;
            this->parent->keys[posInParent] = this->getMaxKey();
            return posInParent + 1;
        }
        else if (posInParent > 0) {
            bro = static_cast<BplusInnerNode<SIZE>*>(this->parent->children[posInParent - 1]);
            LOG_BplusTreeInfo << "in deleting key " << this->keys[delPos]
                << ", " << "merge inner node " << *bro << " to " << *this;
            memcpy(bro->keys + bro->keyNum, this->keys, delPos * sizeof(int));
            memcpy(bro->keys + bro->keyNum + delPos, this->keys + delPos + 1, (this->keyNum - delPos - 1) * sizeof(int));
            memcpy(bro->children + bro->keyNum, children, delPos * sizeof(BplusInnerNode<SIZE>*));
            memcpy(bro->children + bro->keyNum + delPos, children + delPos + 1, (this->keyNum - delPos - 1) * sizeof(BplusInnerNode<SIZE>*));
            for (int i = 0; i < this->keyNum - 1; i++) {
                bro->children[i + bro->keyNum]->parent = bro;
            }
            bro->keyNum += this->keyNum - 1;
            this->parent->keys[posInParent - 1] = bro->getMaxKey();
            return posInParent;
        }
        else {
            LOG_BplusTree << "can't find merge inner node";
            throw std::runtime_error("Failed on merging inner node!");
        }
    }
};

template<int SIZE>
class BplusLeafNode :public BplusNode<SIZE> {
public:
    BplusLeafNode() :BplusNode<SIZE>::BplusNode(true) {}
    BplusLeafNode(int key) :BplusNode<SIZE>::BplusNode(key, true) {}
    BplusLeafNode<SIZE>* next = nullptr;
    BplusLeafNode<SIZE>* splitNode(int key, int posInParent) {
        if (this->keyNum != SIZE) {
            LOG_BplusTree << "key number less than SIZE";
        }
        BplusLeafNode<SIZE>* newNode = new BplusLeafNode<SIZE>();
        //找到key的位置
        LOG_BplusTreeInfo << "in adding key " << key
            << ", " << *this << " split";
        int pos = 0;
        while (pos < this->keyNum) {
            if (key > this->keys[pos]) {
                pos++;
            }
            else if (key == this->keys[pos]) {
                LOG_BplusTree << "duplicate key!";
                throw std::runtime_error("Found duplicate key when split leaf node!");
            }
            else {
                break;
            }
        }
        //分给旧节点SIZE/2+1个，分给新节点(SIZE+1)/2个。
        //新节点是给父节点新的键的节点。因此新节点应该是键值更小的那个节点。
        int oldNum = SIZE / 2 + 1; int newNum = (SIZE + 1) / 2;
        if (pos < oldNum) {
            memcpy(newNode->keys, this->keys + oldNum - 1, newNum * sizeof(int));
            memmove(this->keys + pos + 1, this->keys + pos, sizeof(int) * (oldNum - 1 - pos));
            this->keys[pos] = key;
        }
        else {
            memcpy(newNode->keys, this->keys + oldNum, sizeof(int) * (pos - oldNum));
            newNode->keys[pos - oldNum] = key;
            memcpy(newNode->keys + pos - oldNum + 1, this->keys + pos, sizeof(int) * (newNum + pos - oldNum - 1));
        }
        this->keyNum = oldNum;
        newNode->keyNum = newNum;

        newNode->next = this->next;
        this->next = newNode;

        newNode->parent = this->parent;
        this->parent->children[posInParent] = newNode;//占据原先该节点在parent中的位置。
        
        LOG_BplusTreeInfo << "split node: " << *this
            << ", " << *newNode;
        return this;

    }
    bool deleteKeyByPos(int pos) {//删除时应当会借位。
        if (this->parent->keyNum>1&&this->keyNum <= (SIZE + 1) / 2) {
            LOG_FILE("BplusTree") << "keyNum less than  (SIZE+1)/2+1!";
            return false;
        }
        if (this->keyNum <= pos || pos < 0) {
            LOG_FILE("BplusTree") << "error: pos out of range!";
            return false;
        }
        
        memmove(this->keys + pos, this->keys + pos + 1, (this->keyNum - pos - 1) * sizeof(int));
        this->keyNum -= 1;
        if (this->keyNum == pos && this->parent) {
            int posInParent = this->getPosInParent();
            this->parent->keys[posInParent] = this->getMaxKey();
            this->parent->freshUpOnDelete(posInParent);
        }

        return true;
    }
    //void freshUpOnDelete(int freshPos) {
    //    auto presentNode = this->parent;
    //    BplusInnerNode<SIZE>* parentNode = nullptr;
    //    int posInParent;
    //    while (freshPos == presentNode->keyNum - 1) {
    //        posInParent = presentNode->getPosInParent();
    //        parentNode = presentNode->parent;
    //        if (posInParent > 0) {
    //            parentNode->keys[posInParent] = presentNode->getMaxKey();
    //            presentNode = parentNode;
    //            freshPos = posInParent;
    //        }
    //        else {
    //            break;
    //        }
    //    }
    //}
    //如果有重复的键，则失败。
    bool addKey(int key) {
        for (int i = 0; i < this->keyNum; i++) {
            if (key < this->keys[i]) {
                memmove(this->keys + i + 1, this->keys + i, (this->keyNum - i) * sizeof(int));
                this->keys[i] = key;
                this->keyNum += 1;
                return true;
            }
            else if (key == this->keys[i]) {
                LOG_BplusTree << "fail: key " << key << " is already in the tree";
                return false;
            }
        }
        this->keys[this->keyNum] = key;
        this->keyNum += 1;
        return true;
    }
    //会把一切都搞定。或者什么都不做。
    bool lendKey(int posInParent, int delPos) {
        BplusLeafNode<SIZE>* bro = nullptr;
        if (posInParent < this->parent->keyNum-1) {
            bro = static_cast<BplusLeafNode<SIZE>*>(this->parent->children[posInParent + 1]);
            int key;
            key = bro->keys[0];
            if (bro->deleteKeyByPos(0)) {
                LOG_BplusTreeInfo << "in deleting key " << this->keys[delPos]
                    << ", " << *this << " lend key from " << *bro;
                memmove(this->keys + delPos, this->keys + delPos + 1, (this->keyNum - delPos - 1) * sizeof(int));
                this->keys[this->keyNum - 1] = key;
                this->parent->keys[posInParent] = key;
                return true;
            }
        }
        if (posInParent > 0) {
            bro = static_cast<BplusLeafNode<SIZE>*>(this->parent->children[posInParent - 1]);
            int key;
            key = bro->keys[bro->keyNum - 1];
            if (bro->deleteKeyByPos(bro->keyNum - 1)) {
                LOG_BplusTreeInfo << "in deleting key " << this->keys[delPos]
                    << ", " << *this << " lend key from " << *bro;
                memmove(this->keys + 1, this->keys, delPos * sizeof(int));
                this->keys[0] = key;
                this->parent->keys[posInParent - 1] = bro->getMaxKey();
                this->parent->keys[posInParent] = this->getMaxKey();
                this->parent->freshUpOnDelete(posInParent);
                return true;
            }
        }
        return false;
    }
    //把所有东西都放入前面那个节点中，更新parent，然后返回后面节点在parent中的位置。
    //需要在外部删除parent的
    int mergeBro(int posInParent, int delPos) {
        BplusLeafNode<SIZE>* bro = nullptr;
        if (posInParent < this->parent->keyNum-1) {
            bro = static_cast<BplusLeafNode<SIZE>*>(this->parent->children[posInParent + 1]);
            LOG_BplusTreeInfo << "in deleting key " << this->keys[delPos]
                << ", " << "merge leaf node " << *bro << " to " << *this;
            memmove(this->keys + delPos, this->keys + delPos + 1, (this->keyNum - delPos - 1) * sizeof(int));
            memcpy(this->keys + this->keyNum - 1, bro->keys, (bro->keyNum) * sizeof(int));
            this->keyNum += bro->keyNum - 1;
            this->parent->keys[posInParent] = this->getMaxKey();
            this->next = bro->next;
            LOG_BplusTreeInfo << "merged node: " << *this;
            //此时，parent的posInParent+1上的key不正确，但是child正确。
            return posInParent + 1;
        }
        else if (posInParent > 0) {//这种情况下可能会破坏head节点。需要在外面操作。
            bro = static_cast<BplusLeafNode<SIZE>*>(this->parent->children[posInParent - 1]);
            LOG_BplusTreeInfo << "in deleting key " << this->keys[delPos]
                << ", " << "merge leaf node " << *this << " to " << *bro;
            memcpy(bro->keys + bro->keyNum, this->keys, delPos * sizeof(int));
            memcpy(bro->keys + bro->keyNum + delPos, this->keys + delPos + 1, (this->keyNum - delPos - 1) * sizeof(int));
            bro->keyNum += this->keyNum - 1;
            this->parent->keys[posInParent - 1] = bro->getMaxKey();
            bro->next = this->next;
            LOG_BplusTreeInfo << "merged node: " << *bro;
            return posInParent;
        }
        else {
            LOG_BplusTree << "can't find merge node";
            return -1;
        }
    }
};

template<int SIZE>
class BplusTree;

template<int SIZE>
LogStream& operator<<(LogStream& s, const BplusTree<SIZE>& v);



template<int SIZE>
class BplusTree {
    typedef BplusNode<SIZE> Node;
    typedef BplusLeafNode<SIZE> LeafNode;
    typedef BplusInnerNode<SIZE> InnerNode;
private:
    template<int T>
    friend LogStream& operator<<(LogStream& s, const BplusTree<T>& v);
    InnerNode* root;
    LeafNode* head = nullptr;
    int keyNum;
    //head是节点粒度。anchor是key粒度。

public:
    struct Iterator {
    private:
        int startKey = 0;
        int pos = 0;
        LeafNode* node;
        bool end;
        BplusTree<SIZE>* tree;
    public:
        Iterator():end(true){}
        Iterator(BplusTree<SIZE>* tree, LeafNode* node, int pos) :end(false) {
            this->pos = pos;
            this->node = node;
            this->startKey = node->keys[pos];
            this->tree = tree;
        }
        Iterator(BplusTree<SIZE>* tree, LeafNode* node, int pos, int startKey, bool end) {
            this->pos = pos;
            this->node = node;
            this->startKey = startKey;
            this->end = end;
            this->tree = tree;
        }
        int getKey() {
            if (this->end) {
                throw std::out_of_range("is end!");
            };
            return this->node->keys[this->pos];
        }
        Iterator& toNext() {
            if (this->end) {
                return *this;
            }
            if (this->node->keyNum - 1 > this->pos) {
                this->pos += 1;
            }
            else {
                this->node = this->node->next;
                this->pos = 0;
            }
            if (getKey() == this->startKey) {
                this->end = true;
            }
            return *this;
        }
        Iterator getNext() {
            if (this->end) {
                return Iterator();
            }
            int pos; LeafNode* node;
            if (this->node->keyNum - 1 > this->pos) {
                pos = this->pos + 1;
                node = this->node;
            }
            else {
                node = this->node->next;
                pos = 0;
            }
            if (getKey() == this->startKey) {
                return Iterator();
            }
            else {
                return Iterator(tree, node, pos, this->startKey, end);
            }
        }
        bool isEnd() {
            return end;
        }
        int getStartKey() { return this->startKey; }
        Iterator& toNoLessThan(int key) {
            Iterator tempIter = tree->iteratorAt(key);
            int reqKey = tempIter.getKey();
            while ((!this->isEnd()) && this->getKey() != reqKey) {
                this->toNext();
            }
            return *this;
        }
        //public:
        //    //有风险，它无法保证插入或者删除节点之后内部保存的节点还有效
        //    //可以把Anchor存在该对象中来保证，如果插入或者删除了节点就把Anchor内部参数置为无效?
        //    Anchor(LeafNode* n, int p) :pos(p) { node = *n; }
    };

    struct Anchor {
    private:
        int leftKey = 0;
        int pos = 0;
        LeafNode* node;
        BplusTree<SIZE>* tree;
    public:
        Anchor(BplusTree<SIZE>* tree, LeafNode* node, int pos, int keyNum) :leftKey(keyNum),tree(tree) {
            this->pos = pos;
            this->node = node;
        }
        int getKey() {
            if (this->leftKey<=0) {
                throw std::out_of_range("is end!");
            };
            return this->node->keys[this->pos];
        }
        Anchor& toNext() {
            if (this->leftKey==0) {
                return *this;
            }
            if (this->node->keyNum - 1 > this->pos) {
                this->pos += 1;
            }
            else {
                this->node = this->node->next;
                this->pos = 0;
            }
            this->leftKey--;
            return *this;
        }
        Anchor getNext() {
            if (this->leftKey==0) {
                throw std::logic_error("has to end");
            }
            Anchor newAnchor = *this;//应该是复制操作，这里是否复制了呢？
            newAnchor.toNext();
            return newAnchor;
        }
        bool isEnd() {
            return this->leftKey==0;
        }
        Anchor& toNoLessThan(int key) {
            Anchor tempIter = tree->anchorAt(key);
            int reqKey = tempIter.getKey();
            while ((!this->isEnd()) && this->getKey() != reqKey) {
                this->toNext();
            }
            return *this;
        }
    };
    int id;//用于log
    BplusTree() :root(new InnerNode()), keyNum(0) {
        LOG_BplusTree << "init bplus tree.";
        root->parent = nullptr;//必须赋nullptr。InnerNode如果没有parent则不受最小节点限制。
        LOG_BplusTree << "init bplus tree over.";
    }
    void insert(int key) {
        LOG_BplusTreeN(id) << "insert key" << key;
        if (root->keyNum == 0) {
            head = new LeafNode(key);
            head->next = head;
            head->parent = root;

            root->keys[root->keyNum] = key;
            root->children[root->keyNum] = head;
            root->keyNum += 1;
            keyNum += 1;
        }
        else {
            //找到叶子节点
            Node* node = root;
            std::stack<int> parentPos = {};
            while (!node->isLeaf) {
                parentPos.push(node->searchKey(key));
                if (parentPos.top() == node->keyNum) {//如果pos比当前最大的还要大。
                    node->setMaxKey(key);
                    parentPos.top() -= 1;
                }else if (node->keys[parentPos.top()] == key) {
                    LOG_BplusTree << "fail: key " << key << " already in tree!";
                    return;
                }
                
                node = static_cast<InnerNode*>(node)->children[parentPos.top()];
            }
            //给叶子节点插入key
            LeafNode* leafNode = static_cast<LeafNode*>(node);
            if (leafNode->isFull()) {//键已满
                
                auto leftLeafNode = leafNode->splitNode(key, parentPos.top());
                LOG_BplusTreeN(id) << "split node to " << *leftLeafNode << " and " << *leftLeafNode->next;
                keyNum += 1;
                int newKey = leafNode->getMaxKey();
                splitInnerNode(newKey, leafNode->parent, leftLeafNode, parentPos);
            }else{
                if (!leafNode->addKey(key)) {
                    return;
                }else{
                    keyNum += 1;
                }
            }
        }
    }
    void splitInnerNode(int key, InnerNode* node, Node* childNode, std::stack<int> parentPos) {
        if (node->isFull()) {
            int keyPos = parentPos.top(); parentPos.pop();
            if (node == root) {//增高，如果根节点的节点数量超出最大节点限制，则把根节点分裂为三个节点，
                root = new InnerNode();
                root->parent = nullptr;
                node->parent = root;
                root->keyNum = 1;
                root->keys[0] = node->getMaxKey();
                root->children[0] = node;
                parentPos.push(0);
            }
            auto leftNode = node->splitNode(keyPos, key, childNode, parentPos.top());
            int newKey = node->getMaxKey();
            LOG_BplusTreeN(id) << "split inner node to " << *leftNode;
            splitInnerNode(newKey, node->parent, leftNode, parentPos);
        }
        else {
            node->addKeyAndChildByPos(parentPos.top(), key, childNode);
        }
    }

    bool exist(int key){
        if (root == nullptr) {
            return false;
        }
        //找到叶子节点
        Node* node = root;
        std::stack<int> parentPos = {};
        //叶子节点在父节点中的位置
        int keyPos;//叶子节点的key位置
        while (!node->isLeaf) {
            int pos = node->searchKey(key);
            parentPos.push(pos);
            if (pos == node->keyNum) {//超过node的最大值
                return false;
            }
            node = static_cast<InnerNode*>(node)->children[pos];
        }
        keyPos = node->searchKey(key);
        if (node->keys[keyPos] != key) {
            return false;
        }
        return true;
    }

    void remove(int key) {
        if (key == 1051 && id == 5) {
            int k = 0;
        }
        LOG_BplusTreeN(id) << "remove key" << key;
        if (root == nullptr) {
            LOG_BplusTree << "error: empty tree!";
            throw std::runtime_error("The root of bplusTree is unexpectedly set nullptr.");
        }
        LOG_BplusTree << "start remove key "<<key;
        //找到叶子节点
        Node* node = root;
        std::stack<int> parentPos = {};
        //叶子节点在父节点中的位置
        int keyPos;//叶子节点的key位置
        while (!node->isLeaf) {
            int pos = node->searchKey(key);
            parentPos.push(pos);
            if (pos == node->keyNum) {//超过node的最大值
                LOG_BplusTreeN(this->id) << "fail: key " << key<< " is not in tree! 1";
                return;
            }
            node = static_cast<InnerNode*>(node)->children[pos];
        }
        keyPos = node->searchKey(key);
        bool isMaxKeyOfNode = (keyPos == node->keyNum - 1);
        if (node->keys[keyPos] != key) {
            LOG_BplusTreeN(this->id) <<"keypos:"<< keyPos<<" ,node" << *node <<" ,remove key"<< key;
            LOG_BplusTreeN(this->id) << "fail: key " << key << " is not in tree! 2";
            return;
        }
        keyNum -= 1;
        //删除叶子节点的key
        LeafNode* leafNode = static_cast<LeafNode*>(node);
        if (!leafNode->deleteKeyByPos(keyPos)) {
            if (!leafNode->lendKey(parentPos.top(), keyPos)) {
                int delPos = leafNode->mergeBro(parentPos.top(), keyPos);
                parentPos.pop();
                node = leafNode->parent->children[delPos - 1];//找到合并的节点。
                mergeInnerNode(leafNode->parent, delPos, parentPos);
            }
        }
        else if (leafNode->keyNum == 0) {//父节点为根且父节点只剩一个子节点的情况
                root->keyNum = 0;
                delete leafNode;//这种情况下会删除node。
                head = nullptr;
        }
        else {
            if (keyPos == leafNode->keyNum) {
                InnerNode* parentNode = leafNode->parent;
                parentNode->keys[parentPos.top()] = leafNode->getMaxKey();
                while (parentNode->parent!=nullptr && parentPos.top() == parentNode->keyNum-1)
                {
                    parentPos.pop();
                    parentNode->parent->keys[parentPos.top()] = parentNode->getMaxKey();
                    parentNode = parentNode->parent;
                }
            }
            
        }

        if (head != nullptr) {
            //回溯设置最大键。在删除失败时也应该设置！！！
            while (node->isLastChild()) {//如果不是最后一个，则只会影响其parent的键
                if (node->parent->getMaxKey() > node->getMaxKey()) {
                    node->parent->setMaxKey(node->getMaxKey());
                }
                node = node->parent;
            }
        }
    }
    void mergeInnerNode(InnerNode* node, int delPos, std::stack<int>& parentPos) {
        if (node == root) {//此时parentPos应当也为0
            node->deleteKeyByPos(delPos, true);
            if (node->keyNum == 1&&(!node->children[0]->isLeaf)) {
                root = static_cast<InnerNode*>(node->children[0]);
                root->parent = nullptr;
                delete node;
            }
        }
        else {
            if (!node->deleteKeyByPos(delPos, true)) {
                if (!node->lendKey(parentPos.top(), delPos, true)) {
                    int parentDelPos = node->mergeBro(parentPos.top(), delPos, true);
                    parentPos.pop();
                    mergeInnerNode(node->parent, parentDelPos, parentPos);
                }
            }
        }
    }

    int getKeyNum(){return keyNum;}
    //设置Anchor指向的key（如果不存在则指向前一个）
    Iterator iteratorAt(int key, bool setHead = false) {
        if (head == nullptr) {
            LOG_BplusTreeN(this->id) << "fail: tree has no key!";
            throw std::runtime_error("Tree has no key. Add key before setAnchor!");
        }
        //找到叶子节点
        Node* node = root;
        int pos;
        while (!node->isLeaf) {
            pos = node->searchKey(key);
            if (pos == node->keyNum) {//如果pos比当前最大的还要大。
                while (!node->isLeaf) {
                    //直接搜索当前节点的最大值节点。
                    node = static_cast<InnerNode*>(node)->children[node->keyNum - 1];
                }
                if (setHead) {
                    head = static_cast<LeafNode*>(node)->next;
                }
                return Iterator(this, static_cast<LeafNode*>(node)->next, 0);
            }
            else {
                node = static_cast<InnerNode*>(node)->children[pos];
            }
        }
        pos = node->searchKey(key);
        if (setHead) {
            head = static_cast<LeafNode*>(node);
        }
        return Iterator(this,static_cast<LeafNode*>(node), pos);
    }
    Anchor anchorAt(int key) {
        if (head == nullptr) {
            assert(this->keyNum == 0);
            return Anchor(this, nullptr, 0, 0);
        }
        //找到叶子节点
        Node* node = root;
        int pos;
        while (!node->isLeaf) {
            pos = node->searchKey(key);
            if (pos == node->keyNum) {//如果pos比当前最大的还要大。
                while (!node->isLeaf) {
                    //直接搜索当前节点的最大值节点。
                    node = static_cast<InnerNode*>(node)->children[node->keyNum - 1];
                }
                return Anchor(this, static_cast<LeafNode*>(node)->next, 0, this->keyNum);
            }
            else {
                node = static_cast<InnerNode*>(node)->children[pos];
            }
        }
        pos = node->searchKey(key);
        return Anchor(this, static_cast<LeafNode*>(node), pos, this->keyNum);
    }
    const InnerNode* getRoot() { return this->root; }

};


#endif
