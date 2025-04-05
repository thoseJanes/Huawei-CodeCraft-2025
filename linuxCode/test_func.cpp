#include "headPlanner.h"

void HeadPlanner::test_syncWithHeadTest() const {
    if (this->actionNodes.front().endPos != head->headPos) {
        std::logic_error("the planner status not sync with head!!!");
    }
}

void HeadPlanner::test_nodeContinuousTest() const {
    HeadPlanner* testPlanner = new HeadPlanner(diskPcs, head, this->actionNodes.front());
    LOG_PLANNERN(diskId) << "testContinuous test begin";
    for (auto it = std::next(actionNodes.begin(), 1); it != actionNodes.end(); it++) {
        testPlanner->appendAction((*it).action);
        auto testNode = testPlanner->getLastActionNode();
        assert(testNode.endPos == (*it).endPos);
        //assert(testNode.endTokens == (*it).endTokens);tokens不一定连着
        assert(testNode.action.param == (*it).action.param);
    }
    LOG_PLANNERN(diskId) << "testContinuous test over";
    delete testPlanner;
}