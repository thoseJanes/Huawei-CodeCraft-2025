import matplotlib.pyplot as plt
from typing import Dict,List

class DataObject():
    def __init__(self, tag, objId, size, time, end):
        self.tag = tag
        self.objId = objId
        self.size = size
        self.createTime = time
        self.deleteTime = end
        self.requestTmList = []
        self.requestIdList = []
    def addRequest(self, id, time):
        self.requestTmList.append(time)
        self.requestIdList.append(id)
    def setDelete(self, time):
        self.deleteTime = time
    def getLifeSpan(self):
        return self.deleteTime - self.createTime + 1

class DataSet():
    def __init__(self, dataFile):
        self.dataFile = dataFile
        self.objDict:Dict[int, List[DataObject]] = {}#objid:[obj,],这是为了防止有id重复
        self.tagToObjList:Dict[int, List[DataObject]] = {}
        self.tagToDelNumList:Dict[int, List[int]] = {}
        self.tagToReqNumList:Dict[int, List[int]] = {}
        self.tagToWrtNumList:Dict[int, List[int]] = {}
        
        self.maxSize = 0
        self.getConstantAndStatics()
        self.countData()
        
    def getConstantAndStatics(self):
        '''获取硬件、时间步等信息'''
        with open(self.dataFile, 'r', encoding='utf-8') as file:
            line = file.readline()
            user_input = line.split()
            self.TOLTIME = int(user_input[0])+105
            self.TAGNUM = int(user_input[1])
            self.DISKNUM = int(user_input[2])
            self.UNITNUM = int(user_input[3])
            self.TOKENNUM = int(user_input[4])
            self.CollectionNum = int(user_input[5])
            print("tolTime:",self.TOLTIME)

            operateList = [self.tagToDelNumList, self.tagToWrtNumList, self.tagToReqNumList]
            for loop in range(3):
                for tag in range(1, self.TAGNUM+1):
                    line = file.readline()
                    sline = line.split()
                    operateList[loop][tag] = []
                    for str in sline:
                        operateList[loop][tag].append(int(str))
            
    def countData(self):
        '''读取文件，获取存储对象的信息'''
        with open(self.dataFile, 'r', encoding='utf-8') as file:
            lines = file.readlines()
            lineNum = 0
            while(lines[lineNum].find("TIMESTAMP")==-1):
                lineNum+=1
            time = 0
            while(lineNum<len(lines)):
                if(lines[lineNum].find("TIMESTAMP")!=-1):
                    sline = lines[lineNum].split()
                    assert(len(sline)==2)
                    time = int(sline[1])
                    lineNum += 1
                
                delNum = int(lines[lineNum])
                lineNum += 1
                for i in range(delNum):
                    sline = lines[lineNum].split()
                    assert(len(sline)==1)
                    objList = self.objDict.get(int(sline[0]))
                    assert(objList)
                    objList[-1].setDelete(time)
                    #print('delete on time:'+str(time))
                    lineNum += 1
                    
                wrtNum = int(lines[lineNum])
                lineNum += 1
                for i in range(wrtNum):
                    sline = lines[lineNum].split()
                    assert(len(sline)==3)
                    objId = int(sline[0])
                    objSize = int(sline[1])
                    tag = int(sline[2])
                    self.maxSize = objSize if self.maxSize<objSize else self.maxSize
                    dobj = DataObject(tag, objId, objSize, time, self.TOLTIME)
                    
                    if(self.objDict.get(objId)):
                        self.objDict.get(objId).append(dobj)
                    else:
                        self.objDict[objId] = [dobj]
                        
                    if(self.tagToObjList.get(tag)):
                        self.tagToObjList.get(tag).append(dobj)
                    else:
                        self.tagToObjList[tag] = [dobj]
                    lineNum += 1
                    
                reqNum = int(lines[lineNum])
                lineNum += 1
                for i in range(reqNum):
                    sline = lines[lineNum].split()
                    assert(len(sline)==2)
                    objList = self.objDict.get(int(sline[1]))
                    assert(objList)
                    objList[-1].addRequest(sline[0], time)
                    lineNum += 1
                if(time<self.TOLTIME):
                    if(lines[lineNum].find("GARBAGE")!=-1):
                        lineNum += 1
                    assert(lines[lineNum].find("TIMESTAMP")!=-1)
                    assert(lines[lineNum].find("GARBAGE")==-1)
    def timeBucketNumByTag(self, interval, tag, type, offset = 1):
        """
        获取以interval为桶宽的时间桶内的属于特定tag的操作对象数目。
        
        Args:
            interval (int) :时间桶的宽度
            tag (int) :对象所属标签
            type (str) :指示获取对象的操作类型
                - 'create':获取时间桶内创建的对象数目
                - 'alive':获取时间桶内存活的对象数目，从create所在桶到delete所在桶(含)都算作存活。
                - 'delete':获取时间桶内删除的对象数目
                - 'request':获取时间桶内请求的对象数目
            offset (int) :从第几个时间步开始计时
        Returns:
            list:每个时间桶内的对象数目列表
        """
        bucketNum = [0 for _ in range(int((self.TOLTIME-offset)/interval)+1)]
        objList = self.tagToObjList[tag]
        for obj in objList:
            if(obj.createTime<offset):
                continue
            if(type == 'create'):
                bucketNum[int((obj.createTime-offset)/interval)] += 1
            elif(type == 'alive'):
                start = int((obj.createTime-offset)/interval)
                end = int((obj.deleteTime-offset)/interval)+1
                for p in range(start, end):
                    bucketNum[p] += 1
            elif(type == 'delete'):
                end = int((obj.deleteTime-offset)/interval)
                bucketNum[end] += 1
            elif(type == 'request'):
                for req in obj.requestTmList:
                    if(req<offset):
                        continue
                    bucketNum[int((req-offset)/interval)] += 1
        return bucketNum
    def timeBucketNum(self, interval, type, offset = 1):
        """
        获取以interval为桶宽的时间桶内的操作对象数目。
        
        Args:
            interval (int) :时间桶的宽度
            type (str) :指示获取对象的操作类型
                - 'create':获取时间桶内创建的对象数目
                - 'alive':获取时间桶内存活的对象数目，从create所在桶到delete所在桶(含)都算作存活。
                - 'delete':获取时间桶内删除的对象数目
                - 'request':获取时间桶内请求的对象数目
            offset (int) :从第几个时间步开始计时
        Returns:
            list:每个时间桶内的对象数目列表
        """
        bucketNum = [0 for _ in range(int((self.TOLTIME-offset)/interval)+1)]
        for key, objList in self.objDict.items():
            for obj in objList:
                if(obj.createTime<offset):
                    continue
                if(type == 'create'):
                    bucketNum[int((obj.createTime-offset)/interval)] += 1
                elif(type == 'alive'):
                    start = int((obj.createTime-offset)/interval)
                    end = int((obj.deleteTime-offset)/interval)+1
                    for p in range(start, end):
                        bucketNum[p] += 1
                elif(type == 'delete'):
                    end = int((obj.deleteTime-offset)/interval)
                    bucketNum[end] += 1
                elif(type == 'request'):
                    for req in obj.requestTmList:
                        if(req<offset):
                            continue
                        bucketNum[int((req-offset)/interval)] += 1
        return bucketNum
    def getNumDivideByTag(self):
        """
        获取创建的不同tag存储对象的数量
        Returns:
            list:属于每个tag的存储对象的数目列表
        """
        bucketNum = [0 for _ in range(self.TAGNUM)]
        for key, objList in self.tagToObjList.items():
            bucketNum[key-1] = len(objList)
        return bucketNum
    def getObjListByTag(self, tag:int):
        return self.tagToObjList[tag]
    def getSizeToCreateRequestByTag(self, tag):
        sizeToRequest = [0 for _ in range(self.maxSize)]
        sizeToCreate = [0 for _ in range(self.maxSize)]
        objList = self.tagToObjList[tag]
        for obj in objList:
            sizeToRequest[obj.size-1] += len(obj.requestTmList)
            sizeToCreate[obj.size-1] += 1
        return sizeToCreate, sizeToRequest            

class dataListInfo:
    dataList:List = []
    axisLable:str = ''
    curveLegend:str = ''
    color:str = ''
    def __init__(self, data, lineWidth=1, color='', axisLabel='', curveLegend=''):
        self.dataList = data
        self.axisLable = axisLabel
        self.curveLegend = curveLegend
        self.lineWidth = lineWidth
        self.color = color

def doubleYPlot(xlist:List, yListInfo1:dataListInfo, yListInfo2:dataListInfo, axis = None, xlabel = None):
    yListInfo = [yListInfo1, yListInfo2]
    colors = [yListInfo[0].color, yListInfo[1].color]
    if not axis:
        axis = plt.subplot()
        if xlabel:
            axis.set_xlabel(xlabel)
    # axis.set_xlabel(xlabel)#不清楚原因，无法在这个函数内设置xlabel
    axises = [None, None]
    axises[0] = axis
    axises[1] = axises[0].twinx()
    axis.set_ybound(lower=0)
    #axis.set_ylim(bottom=0, top=max(yListInfo1.dataList))
    #axises[1].set_ylim(bottom=0, top=max(yListInfo2.dataList))
    
    for sq in range(2):
        if(yListInfo[sq].curveLegend != ''):
            axises[sq].plot(xlist, yListInfo[sq].dataList, colors[sq][0]+'-', linewidth=yListInfo[sq].lineWidth, label = yListInfo[sq].curveLegend)
        else:
            axises[sq].plot(xlist, yListInfo[sq].dataList, colors[sq][0]+'-', linewidth=yListInfo[sq].lineWidth)
        axises[sq].set_ylabel('{} num'.format(yListInfo[sq].axisLable), color=colors[sq])
    
        if(colors[sq] != ''):
            axises[sq].spines['right'].set_color(colors[sq])
            axises[sq].yaxis.label.set_color(colors[sq])
            axises[sq].tick_params(axis='y', colors=colors[sq])
        if(yListInfo[sq].curveLegend != ''):
            axises[sq].legend(loc = 'upper right')
    plt.plot()