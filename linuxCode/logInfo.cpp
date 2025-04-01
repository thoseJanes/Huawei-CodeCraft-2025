#include "headPlanner.h"
#include "circularLinkedList.h"


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

