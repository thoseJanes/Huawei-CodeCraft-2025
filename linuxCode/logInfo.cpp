#include "headPlanner.h"
#include "circularLinkedList.h"
#include "bplusTree.h"

template<int SIZE, typename VALUE>
inline LogStream& operator<<(LogStream& s, const BplusTree<SIZE, VALUE>& v)
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
        BplusLeafNode<SIZE, VALUE>* temp = v.head->next;
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

template LogStream& operator<<(LogStream& s, const BplusTree<4, bool>& v);

LogStream& operator<<(LogStream& s, const ActionNode& actionNode) {
    s << "\n[" << actionNode.action << "--" <<
        ",  finishPos:" << actionNode.endPos <<
        ",  endTokens:" << actionNode.endTokens << "]";
    return s;
}
LogStream& operator<<(LogStream& s, HeadPlanner& headPlanner) {
    s << "\n";
    s << "diskId:" << headPlanner.diskId << " ";
    s << "spaceSize:" << headPlanner.spaceSize << "\n";
    s << "(actions,topos,endTokens):\n{";
    for (auto it = headPlanner.actionNodes.begin(); it != headPlanner.actionNodes.end(); it++) {
        s << "" << *it << ", ";
    }
    s << "}\n";
    return s;
}
LogStream& operator<<(LogStream& s, const HeadOperator& headOperator) {
    if (headOperator.action == JUMP) {
        s << "[JUMP jumpTo:" << headOperator.jumpTo << "]";
    }
    else if (headOperator.action == PASS) {
        s << "[PASS times:" << headOperator.passTimes << "]";
    }
    else if (headOperator.action == READ) {
        s << "[READ aheadRead:" << headOperator.aheadRead << "]";
    }
    else if (headOperator.action == VREAD) {
        s << "[VREAD aheadRead:" << headOperator.aheadRead << "]";
    }
    else if (headOperator.action == START) {
        s << "[START aheadRead:" << headOperator.aheadRead << "]";
    }
    return s;
}
LogStream& operator<<(LogStream& s, HeadOperator& headOperator) {
    if (headOperator.action == JUMP) {
        s << "[JUMP jumpTo:" << headOperator.jumpTo << "]";
    }
    else if (headOperator.action == PASS) {
        s << "[PASS times:" << headOperator.passTimes << "]";
    }
    else if (headOperator.action == READ) {
        s << "[READ aheadRead:" << headOperator.aheadRead << "]";
    }
    else if (headOperator.action == VREAD) {
        s << "[VREAD aheadRead:" << headOperator.aheadRead << "]";
    }
    else if (headOperator.action == START) {
        s << "[START aheadRead:" << headOperator.aheadRead << "]";
    }
    return s;
}

LogStream& operator<<(LogStream& s, CircularSpacePiece& linkedList) {
    const SpacePieceNode* node = linkedList.getHead();
    const SpacePieceNode* head = linkedList.getHead();
    s << "{" << "totalSpace:" << linkedList.getTolSpace() << ": " << *node << ",";
    while (node->getNext() != head) {
        node = node->getNext();
        s << *node << ",";
    }
    s << "}\n";
    return s;
}
LogStream& operator<<(LogStream& s, const SpacePieceNode& node) {
    s << "(" << node.getStart() << "," << node.getLen() + node.getStart() - 1 << ")";
    return s;
}


LogStream& operator<<(LogStream& s, const SpacePiece& node) {
    s << "(" << node.start << ", "<<node.len << ")";
    return s;
}
LogStream& operator<<(LogStream& s, SpacePieceBlock& space) {
    s << "space:\n" << "start:" << space.startPos << ",size:" << space.size << ",res:" << space.res;
    s << space.spaceNodes;
    return s;
}

