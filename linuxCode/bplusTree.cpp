#include "bplusTree.h"

template<int SIZE>
inline LogStream& operator<<(LogStream& s, const BplusTree<SIZE>& v)
{
    std::vector<BplusNode<SIZE>*> nodeLayer = {};
    std::vector<BplusNode<SIZE>*> nodeNextLayer = {};
    nodeLayer.push_back(v.root);
    if (v.root) {
        s << "\n(num:"<<v.keyNum<<")";
        s << *v.root;
        while (nodeLayer.size()) {
            s << "----";
            for (int i = 0; i < nodeLayer.size(); i++) {
                auto node = nodeLayer[i];
                if (!node->isLeaf) {
                    BplusInnerNode<SIZE>* inode = static_cast<BplusInnerNode<SIZE>*>(node);
                    s << "{";
                    for (int j = 0; j < inode->keyNum; j++) {
                        nodeNextLayer.push_back(inode->children[j]);
                        //if (inode->children[j]->isLeaf) {
                        //    s << *inode->children[j] << "->" << *(static_cast<BplusLeafNode<SIZE>*>(inode->children[j])->next)<<", ";
                        //}
                        //else {
                            s << *inode->children[j] << ",";
                        //}
                        //s << *inode->children[j]->parent << ",";
                    }
                    s << "}";
                }
            }
            //break;
            nodeLayer = nodeNextLayer;
            nodeNextLayer = {};
        }
    }
    if (v.head) {
        s << "\nlinked list:{" << *v.head <<":";
        BplusLeafNode<SIZE>* temp = v.head->next;
        if (temp != v.head) {
            while (temp != v.head) {
                s << *temp;
                temp = temp->next;
            }
        }
        s << "}";
    }
    return s;
}

template LogStream& operator<<(LogStream& s, const BplusTree<4>& v);

