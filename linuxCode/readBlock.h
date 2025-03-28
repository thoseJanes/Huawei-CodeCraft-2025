//#ifndef READBLOCK_H
//#define READBLOCK_H
//#include "global.h"
//class BlockSpiece {
//
//};
//
//class ReadBlock {
//	int start;
//	int len;
//	int prefixLen;
//	ReadBlock* nextBlock = nullptr;
//	ReadBlock(int start, int len):start(start),len(len){}
//	int growth() {
//		int acc = 0;
//		int loss;
//		int profit;
//		for (int i = 0; i < readConsumeAfterN.size(); i++) {
//			int loss = readConsumeAfterN[acc];
//			int profit = 0;
//			if (i < len - 1) {
//				profit += getReadConsumeAfterN(acc + i) - getReadConsumeAfterN(acc - 1 + i);
//			}
//		}
//	}
//	std::pair<int,int> getPrefix() {
//		int prefixStart = start - prefixLen;
//		return { prefixStart , prefixLen};
//	}
//	std::pair<int, int> getMainLength() {
//		int mainLen = len;
//		auto cBlock = nextBlock;
//		while (cBlock != nullptr) {
//			mainLen += cBlock->getPartLength();
//			cBlock = cBlock->nextBlock;
//		}
//		return { start, mainLen };
//	}
//	int getPartLength() {
//		return len + prefixLen;
//	}
//};
//
//#endif