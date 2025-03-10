import matplotlib.pyplot as plt
from typing import Dict,List
INTERVAL = 100

#1.确定了每个对象只有一个id

class DataObject():
    def __init__(self, tag, objId, size, time):
        self.tag = tag
        self.objId = objId
        self.size = size
        self.createTime = time
        self.requestTmList = []
        self.requestIdList = []
    def request(self, id, time):
        self.requestTmList.append(time)
        self.requestIdList.append(id)
    def delete(self, time):
        self.deleteTime = time
    def getLifeSpan(self):
        return self.deleteTime - self.createTime + 1

class DataSet():
    def __init__(self, dataFile):
        self.dataFile = dataFile
        self.objDict:Dict[int, List[DataObject]] = {}#objid:[obj,],这是为了防止有
        self.getConstant()
        self.countData()
    def getConstant(self):
        with open(self.dataFile, 'r', encoding='utf-8') as file:
            line = file.readline()
            user_input = line.split()
            self.TOLTIME = int(user_input[0])+105
            self.TAGNUM = int(user_input[1])
            self.DISKNUM = int(user_input[2])
            self.UNITNUM = int(user_input[3])
            self.TOKENNUM = int(user_input[4])
    def countData(self):
        with open(self.dataFile, 'r', encoding='utf-8') as file:
            lines = file.readlines()
            lineNum = 0
            while(not lines[lineNum].startswith("TIMESTAMP")):
                lineNum+=1
            time = 0
            while(lineNum<len(lines)):
                line = lines[lineNum]
                if(line.startswith("TIMESTAMP")):
                    sline = line.split(' ')
                    assert(len(sline)==2)
                    time = int(sline[1])
                    lineNum += 1
                    continue
                delNum = int(lines[lineNum])
                lineNum += 1
                for i in range(delNum):
                    sline = lines[lineNum]
                    objList = self.objDict.get(int(sline))
                    assert(objList)
                    objList[-1].delete(time)
                    lineNum += 1
                wrtNum = int(lines[lineNum])
                lineNum += 1
                for i in range(wrtNum):
                    sline = lines[lineNum].split()
                    dobj = DataObject(int(sline[2]), int(sline[0]), int(sline[1]), time)
                    if(self.objDict.get(int(sline[0]))):
                        self.objDict.get(int(sline[0])).append(dobj)
                    else:
                        self.objDict[int(sline[0])] = [dobj]
                    lineNum += 1
                reqNum = int(lines[lineNum])
                lineNum += 1
                for i in range(reqNum):
                    sline = lines[lineNum].split()
                    objList = self.objDict.get(int(sline[1]))
                    assert(objList)
                    objList[-1].request(sline[0], time)
                    lineNum += 1
    def tagBucketNum(self, interval, tag, type):
        bucketNum = [0 for _ in range(int(self.TOLTIME/interval)+1)]
        for key, objList in ds.objDict.items():
            for obj in objList:
                if(obj.tag != tag):
                    continue
                if(type == 'create'):
                    bucketNum[int((obj.createTime-1)/interval)] += 1
                elif(type == 'request'):
                    for req in obj.requestTmList:
                        bucketNum[int((req-1)/interval)] += 1
        return bucketNum
    def bucketNum(self, interval, type):
        bucketNum = [0 for _ in range(int(self.TOLTIME/interval)+1)]
        for key, objList in ds.objDict.items():
            for obj in objList:
                if(type == 'create'):
                    bucketNum[int((obj.createTime-1)/interval)] += 1
                elif(type == 'request'):
                    for req in obj.requestTmList:
                        bucketNum[int((req-1)/interval)] += 1
        return bucketNum


maxLen = 1
ds = DataSet("sample_practice.in")
for key,objList in ds.objDict.items():
    assert(maxLen == len(objList))

# 示例数据
data = ds.bucketNum(200, 'request')

# 绘制直方图
plt.hist(data, bins=5, edgecolor='black')  # bins参数指定直方图的柱数

# 添加标题和标签
plt.title('直方图示例')
plt.xlabel('数值')
plt.ylabel('频数')

# 显示图形
plt.show()