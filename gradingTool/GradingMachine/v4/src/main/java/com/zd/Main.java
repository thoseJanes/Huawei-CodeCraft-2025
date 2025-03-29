package com.zd;

import com.zd.GUI.MainFrame;
import com.zd.Impl.DiskPointImpl;
import com.zd.Impl.GeneralInformationImpl;
import com.zd.Impl.StorageObjectImpl;
import com.zd.Init.ParseMatrix;
import com.zd.ServiceImpl.ClearDataBaseRunnable;
import com.zd.ServiceImpl.UpdateDataBaseRunnable;
import com.zd.UDP.Communication;
import com.zd.Util.*;

import javax.swing.*;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

import com.zd.Impl.DiskImpl;

public class Main {
    static int T, M, N, V, G;
    static int FRE_COLUMN;
    static double SCORES;
    static final String TIMESTAMP = "TIMESTAMP";
    static int[][] FRE_DEL;
    static int[][] FRE_WRITE;
    static int[][] FRE_READ;
    static List<Integer> UNDONE_REQUEST = new ArrayList<>();
    static final int FRE_PER_SLICING = 1800;

    static final int MAX_DISK_NUM = (10 + 1);

    static final int MAX_DISK_SIZE = (16384 + 1);

    static final int MAX_REQUEST_NUM = (30000000 + 1);

    static final int MAX_OBJECT_NUM = (100000 + 1);

    static final int REP_NUM = (3);

    static final int EXTRA_TIME = (105);

    private static int[][] disk = new int[MAX_DISK_NUM][MAX_DISK_SIZE];

    private static int[] diskPoint = new int[MAX_DISK_NUM];
    static int[] diskToken = new int[MAX_DISK_NUM];
    static char[] diskLastAction = new char[MAX_DISK_NUM];
    static int[] preToken = new int[MAX_DISK_NUM];

    static int[] objectIds = new int[MAX_OBJECT_NUM];

    //static Request[] requests = new Request[MAX_REQUEST_NUM];
    static List<Request> requests = new ArrayList<>();

    //static StorageObject[] storageObjects = new StorageObject[MAX_OBJECT_NUM];
    static List<StorageObject> storageObjects = new ArrayList<>();

    static int currentRequest = 0;

    static int currentPhase = 0;
    static int abortRequestsNum = 0;
    static int completedRequestsNum = 0;
    static int totalRequestsNum = 0;
    static final boolean VISIBLE = true;
    public static MainFrame myFrame;
    static double maxObjectScore = 0;
    static int maxObject = 0;
    static GeneralInformation generalInfo = new GeneralInformation();
    static DiskPointInfomation diskPointInfo = new DiskPointInfomation();
    static List<DiskOperation> diskOperationsList = new ArrayList<>();
    static List<StorageObjectionDeleteInfo> storageObjectionDeleteInfoList = new ArrayList<>();
    static List<StorageObjectionWriteInfo> storageObjectionWriteInfoList = new ArrayList<>();
    static DiskImpl diskImpl = new DiskImpl();
    static DiskPointImpl diskPointImpl = new DiskPointImpl();
    static StorageObjectImpl storageObjectImpl = new StorageObjectImpl();
    static GeneralInformationImpl generalInfoImpl = new GeneralInformationImpl();

    static long scoreTime = 0;
    static long dataBaseTime = 0;
    static long logicTime = 0;
    static int firstScoreCursor = 0;
    static int secondScoreCursor = 0;

    public static void main(String[] args) {
        //读取sample_practise.in文件并通过UDP输出
        String file_path = "src/main/resources/sample.in";
        List<String> lines = null;
        try{
            lines = Files.readAllLines(Paths.get(file_path));
        }catch (Exception e){
            e.printStackTrace();
        }
        requests.add(0,new Request());
        storageObjects.add(0,new StorageObject());
        StorageObject.REP_NUM = 3;

        //初始化T、M、N、V、G以及三个操作数量统计矩阵

        if (lines != null) {
            String[] init_args = lines.get(0).split("\\s+");
            T = Integer.parseInt(init_args[0]);
            M = Integer.parseInt(init_args[1]);
            N = Integer.parseInt(init_args[2]);
            V = Integer.parseInt(init_args[3]);
            G = Integer.parseInt(init_args[4]);
            FRE_COLUMN = (int) Math.ceil(((double)T)/FRE_PER_SLICING);
        }else{
            System.out.println(file_path + "文件读取失败！");
            System.exit(1);
        }
        FRE_DEL = new int[M][FRE_COLUMN];
        FRE_WRITE = new int[M][FRE_COLUMN];
        FRE_READ = new int[M][FRE_COLUMN];
        FRE_DEL = ParseMatrix.ParseSingleMatrix(lines,1,1+M,FRE_COLUMN);
        FRE_WRITE = ParseMatrix.ParseSingleMatrix(lines,1+M,1+2*M,FRE_COLUMN);
        FRE_READ = ParseMatrix.ParseSingleMatrix(lines,1+2*M,1+3*M,FRE_COLUMN);
        for (int i = 1; i <= N; i++) {
            diskPoint[i] = 1;
        }


        //将所有DataBase的数据清空
        clearDataBase();

        //发送初始化数据
        String ClientIP = "localhost";
        int ClientPort = 8081;
        String ServerIP = "localhost";
        int ServerPort = 8080;
        Communication comm = new Communication(ServerIP,ServerPort,ClientIP,ClientPort);
        String send_data = lines.get(0);//发送T M N V G
        comm.SendMessage(send_data);
        send_data = ParseMatrix.ParseSingleInputMatrix(lines,1,1+3*M);//三个数组的合并体send_data = [fre_del;fre_write;fre_read]
        comm.SendMessage(send_data);//发送三个数组fre_del、fre_write、fre_read

        //接收数据"OK"
        String receive_data = comm.ReceiveMessage();
        if(!receive_data.equals("OK\n")){
            System.out.println("初始化失败，没有接收到字符串OK");
            System.exit(1);
        }

        //初始化显示页面
        if(VISIBLE){
            myFrame = initInteractor();
        }


        int line_index = 1+3*M;
        long startTime;
        long endTime;
        for(int timestamp=1;timestamp<=T+EXTRA_TIME;timestamp++){

            //对齐时间片
            //发送时间片
            send_data = lines.get(line_index++);
            String receive_timestamp = send_data.split("\\s+")[1];
            comm.SendMessage(receive_timestamp);


            //接收时间片
            receive_data = comm.ReceiveMessage();
            if(!receive_data.equals(receive_timestamp)){
                String message = String.format("时间片对齐失败，Java服务端时间片为:%s,C++客户端时间片为:TIMESTAMP %s",send_data,receive_data);
                System.out.println(message);
                System.exit(1);
            }

            startTime = System.currentTimeMillis();

            CalculateScores();


            //更新对象分数减少速度
            while(firstScoreCursor<requests.size() && timestamp-requests.get(firstScoreCursor).startTimeStamp >= 105){
                int objectId = requests.get(firstScoreCursor).objectId;
                if(!requests.get(firstScoreCursor).isAborted && !requests.get(firstScoreCursor).isDone){
                    storageObjects.get(objectId).desScore -= 0.01F;
                }
                firstScoreCursor++;
            }
            while(secondScoreCursor<requests.size() && timestamp-requests.get(secondScoreCursor).startTimeStamp >= 10){
                int objectId = requests.get(secondScoreCursor).objectId;
                if(!requests.get(secondScoreCursor).isAborted && !requests.get(secondScoreCursor).isDone){
                    storageObjects.get(objectId).desScore += 0.005F;
                }
                secondScoreCursor++;
            }
            System.out.printf("时间帧%d的对象的分数据已更新完成！%n",timestamp);

            endTime = System.currentTimeMillis();

            scoreTime += (endTime-startTime);

            startTime = System.currentTimeMillis();

            //删除事件交互
            //JAVA向C++发送删除事件的输出
            send_data = lines.get(line_index++);//n_delete
            int nDelete = Integer.parseInt(send_data);//n_delete
            StringBuilder del_obj_id = new StringBuilder();
            comm.SendMessage(send_data);
            if(!send_data.equals("0")){
                int del_num = Integer.parseInt(send_data);
                for(int j = line_index;j<line_index+del_num;j++){
                    del_obj_id.append(lines.get(j));
                    del_obj_id.append("\n");
                }
                send_data = String.valueOf(del_obj_id);
                comm.SendMessage(send_data);
                line_index += del_num;
            }
            //处理删除事件
            if(nDelete > 0){
                String[] obj_id =  String.valueOf(del_obj_id).split("\n");
                for(int j=1; j<=nDelete; j++){
                    objectIds[j] = Integer.parseInt(obj_id[j-1]);
                }

            }

            //删除
            for (int j = 1; j <= nDelete; j++) {
                int id = objectIds[j];//删除对象的对象ID
                for (int k = 1; k <= REP_NUM; k++) {
                    doObjectDelete(storageObjects.get(id).unit[k], disk[storageObjects.get(id).replica[k]], storageObjects.get(id).size);
                }
                storageObjects.get(id).isDelete = true;
                storageObjects.get(id).scores = 0;
                storageObjects.get(id).scoresLevel = 0;
                storageObjects.get(id).requestsNum = 0;
                storageObjects.get(id).desScore = 0;

                if(id == maxObject){
                    updateMaxScore();
                }

                DiskOperation _diskOpera = new DiskOperation(id,"delete",timestamp);
                diskOperationsList.add(_diskOpera);

                StorageObjectionDeleteInfo _objDel = new StorageObjectionDeleteInfo(id,timestamp);
                storageObjectionDeleteInfoList.add(_objDel);
            }

            //删除事件接收C++消息，接收选手的消息并处理
            //处理的注意事项如下：
            //***选手输出的请求编号必须为删除对象的当前未完成的读请求，否则判题器会报错。***
            //***选手需要保证n_abort与接下来输出的req_id数量一致，否则判题器可能会报错或一直等待选手输入而卡住***
            int del_abort_num = Integer.parseInt(comm.ReceiveMessage());//接收被取消的读取请求数量
            if(del_abort_num>0){
                receive_data = comm.ReceiveMessage();//接收被取消的读取请求编号
                String[] abort_read_request = receive_data.split("\n");
                if(abort_read_request.length != del_abort_num){
                    System.out.println("选手输入出错：被取消的读取请求的总数量n_abort与输出的req_id数量不一致!");
                    System.exit(1);
                }
                for(int j=1;j<=del_abort_num;j++){
                    int requestId = Integer.parseInt(abort_read_request[j-1]);
                    if( requests.get(requestId).isAborted || !requests.get(requestId).isGenerated  || requests.get(requestId).isDone){
                        System.out.println("选手输入出错：被取消的读取请求不属于当前未完成的读取请求！");
                        System.exit(1);
                    }
                    requests.get(requestId).isAborted = true;
                    requests.get(requestId).readRequestInfo = new int[requests.get(requestId).readRequestInfo.length];
                    abortRequestsNum += 1;

                }

            }

            //对象写入事件交互
            send_data = lines.get(line_index++);//n_write
            comm.SendMessage(send_data);
            if(!send_data.equals("0")){
                StringBuilder sb = new StringBuilder();
                int nWrite = Integer.parseInt(send_data);
                for(int j = line_index; j<line_index+ nWrite; j++){
                    sb.append(lines.get(j));
                    sb.append("\n");
                }
                send_data = String.valueOf(sb);
                comm.SendMessage(send_data);
                line_index += nWrite;

                String[] write_infos = String.valueOf(sb).split("\n");
                for(int j = 1; j<= nWrite; j++){
                    //接收对象写入信息，需要记录磁盘的存储信息和对象的各个对象块的写入位置，但是还没有做
                    //注意事项如下：
                    //* 当前时间片写入的对象数可能为0，此时选手无需有任何输出。
                    //* 选手输出的写入对象顺序无需和输入保持一致，但是需要在该时间片处理所有的写入请求。
                    //* 选手输出的每个副本的存储单元数需要与该对象大小一致，否则判题器可能会报错或一直等待选手的输入而卡住。
                    int id, size, tag, lastRequestPoint,requestsNum,scoresLevel;
                    boolean isDelete;
                    double scores;
                    String[] write_info = write_infos[j-1].split("\\s+");
                    id = Integer.parseInt(write_info[0]);
                    size = Integer.parseInt(write_info[1]);
                    tag = Integer.parseInt(write_info[2]);
                    lastRequestPoint = 0;
                    isDelete = false;
                    requestsNum = 0;
                    scores = 0.0F;
                    scoresLevel = 0;
                    storageObjects.add(id,new StorageObject(size,lastRequestPoint,isDelete,tag,requestsNum,scores,scoresLevel));
                }

                //选手写入事件，当前时间片写入的对象数可能为0，此时选手无需有任何输出。
                receive_data = comm.ReceiveMessage();
                String[] write_disk_infos = receive_data.split("\n");
                if(write_disk_infos.length != 4*nWrite){
                    System.out.println("选手输入出错：选手输入对象写入信息数量与实际硬盘写入数量不一致！");
                    System.exit(1);
                }
                for(int j = 0; j< nWrite; j++){
                    int id = Integer.parseInt(write_disk_infos[j*4]);
                    int size = storageObjects.get(id).size;
                    int[][] disk_pos = new int[3][size+1];
                    for(int k=0; k<3; k++){
                        String[] _disk_pos = write_disk_infos[j*4+k+1].split("\\s+");
                        for(int m=0; m<size+1; m++){
                            disk_pos[k][m] = Integer.parseInt(_disk_pos[m]);//选手输入的磁盘编号和存储块编号均从1开始，disk的索引均从1开始
                        }
                    }
                    for (int k = 1; k <= REP_NUM; k++) {
                        int replica = disk_pos[k-1][0];
                        int[] unit = new int[size+1];
                        System.arraycopy(disk_pos[k-1],1,unit,1,size);
                        storageObjects.get(id).replica[k] = replica;
                        storageObjects.get(id).unit[k] = unit;
                        doObjectWrite(storageObjects.get(id).unit[k], disk[storageObjects.get(id).replica[k]], size, id,timestamp);
                    }


                    DiskOperation _diskOpera = new DiskOperation(id,"write",timestamp);
                    diskOperationsList.add(_diskOpera);

                    StorageObjectionWriteInfo objWriteInfo = new StorageObjectionWriteInfo(id, storageObjects.get(id),timestamp);
                    storageObjectionWriteInfoList.add(objWriteInfo);

                }


            }

            //对象读取事件交互
            int nRead;
            String[] read_info = null;

            send_data = lines.get(line_index++);//n_read
            nRead = Integer.parseInt(send_data);
            comm.SendMessage(send_data);
            StringBuilder read_info_builder = new StringBuilder();
            if(!send_data.equals("0")){
                int read_num = Integer.parseInt(send_data);
                for(int j = line_index;j<line_index+read_num;j++){
                    read_info_builder.append(lines.get(j));
                    read_info_builder.append("\n");
                }
                send_data = String.valueOf(read_info_builder);
                comm.SendMessage(send_data);
                line_index += read_num;
            }

            //选手读取事件，处理读取请求
            totalRequestsNum += nRead;
            if(nRead>0){
                read_info = String.valueOf(read_info_builder).split("\n");//read_info是读取请求
            }
            for (int j = 1; j <= nRead; j++) {
                String[] line = read_info[j-1].split("\\s+");
                int requestId = Integer.parseInt(line[0]);
                int objectId = Integer.parseInt(line[1]);
                int objectSize = storageObjects.get(objectId).size;
                int prevId = storageObjects.get(objectId).lastRequestPoint;
                boolean isDone = false;
                boolean isGenerated = true;
                int[] readRequestInfo = new int[objectSize+1];//将读取请求情况初始化为0
                int startTimeStamp = timestamp;
                boolean isAborted = false;
                requests.add(requestId,new Request(objectId,prevId,isDone,startTimeStamp,readRequestInfo,isAborted,isGenerated));
                storageObjects.get(objectId).lastRequestPoint = requestId;
                storageObjects.get(objectId).requestsNum += 1;
                storageObjects.get(objectId).scores += (double) ((objectSize+1)*0.5*1);
                maxObject = maxObjectScore>storageObjects.get(objectId).scores ? maxObject:objectId;
                maxObjectScore = Math.max(storageObjects.get(objectId).scores,maxObjectScore);
                storageObjects.get(objectId).desScore += 0.005F;
            }

            //读取选手输出三个磁盘动作并处理
            String[] disk_action = comm.ReceiveMessage().split("\n");
            if(disk_action.length != N){
                System.out.println("选手输入出错：磁头指令数量与磁盘数量不一致！");
                System.exit(1);
            }
            for(int j=1;j<=N;j++){
                int token = G, index=0;
                while(index<disk_action[j-1].length() && token>0){
                    char action = disk_action[j-1].charAt(index);
                    if(action == 'p'){
                        if(token>=1){
                            token--;
                            diskToken[j] = token;
                            diskPoint[j] = diskPoint[j]%V+1;
                            diskLastAction[j] = 'p';

                        }else{
                            String message = String.format("磁盘%d的动作字符串%s中第%d个动作失效，原因为令牌不足！",j,disk_action[j-1],index+1);
                            System.out.println(message);
                            break;
                        }
                        index++;
                    }else if(action == 'r'){
                        int consume;
                        if(timestamp==1 || diskLastAction[j] != 'r'){
                            consume = 64;
                        }else{
                            consume = (int) Math.max(16,Math.ceil(preToken[j]*0.8));
                        }
                        if(token>=consume){
                            token -= consume;
                            //判断磁盘中被读取的对象
                            int objectId = disk[j][diskPoint[j]];
                            if(objectId == 0){//判断读取内容是否为空，objectId=0则为空
                                String message = String.format("选手输入出错：磁盘%d读取存储位置%d时，磁盘内容为空！",j,diskPoint[j]);
                                System.out.println(message);
                            }else{
                                int objectBlockIndex = getObjectBlockIndex(objectId,j,diskPoint[j]);
                                int currentId = storageObjects.get(objectId).lastRequestPoint;
                                int prevId = requests.get(currentId).prevId;
                                boolean isRequestValid = (!requests.get(currentId).isAborted && requests.get(currentId).isGenerated && !requests.get(currentId).isDone);
                                while(currentId != 0){
                                    if(isRequestValid){
                                        requests.get(currentId).readRequestInfo[objectBlockIndex] += 1;
                                    }
                                    currentId = prevId;
                                    prevId = requests.get(prevId).prevId;
                                }
                            }

                            //处理token、磁头位置、磁头最新动作
                            diskToken[j] = token;
                            diskPoint[j] = diskPoint[j]%V+1;
                            diskLastAction[j] = 'r';
                        }else{
                            String message = String.format("磁盘%d的动作字符串%s中第%d个动作失效，原因为令牌不足！",j,disk_action[j-1],index+1);
                            System.out.println(message);
                            break;
                        }
                        index++;
                    }else if(action == 'j'){
                        if(token >= G ){
                            token -= G;
                            diskToken[j] = token;
                            StringBuilder jump_pos = new StringBuilder();
                            index += 2;
                            char temp =  disk_action[j-1].charAt(index);
                            while( temp>='0' && temp<='9' && index<disk_action[j-1].length()){
                                temp = disk_action[j-1].charAt(index);
                                jump_pos.append(temp);
                                index++;
                            }
                            diskPoint[j] = Integer.parseInt(String.valueOf(jump_pos));
                            diskLastAction[j] = 'j';
                        }else{
                            String message = String.format("磁盘%d的动作字符串%s中第%d个动作失效，原因为令牌不足！",j,disk_action[j-1],index+1);
                            System.out.println(message);
                            break;
                        }
                    }else if(action == '#'){
                        break;
                    }
                }
            }


            //读取已经完成的读取请求数量
            receive_data = comm.ReceiveMessage();
            if(!receive_data.equals("0")){
                //读取已经完成的读取请求编号
                int n_rsp = Integer.parseInt(receive_data);
                receive_data = comm.ReceiveMessage();//接收选手上报的已经完成的读取请求
                String[] finished_request = receive_data.split("\n");
                for(int j=0;j<n_rsp;j++){
                    //处理选手上报的已经完成的请求
                    //注意事项如下：
                    //* 一个请求只能上报一次读成功或者被取消，多次上报会被判题器报错
                    //* 在𝑇+105个时间分片后，未完成的读请求会被记为0分，不影响完成交互。
                    //* 请严格按照题目规定的格式输出，格式错误可能会被判题器判错。
                    int requestId = Integer.parseInt(finished_request[0]);
                    Request request = requests.get(requestId);
                    boolean isRequestValid = (!request.isDone);
                    boolean isRequestAchieved = IsRequestAchieved(requestId);
                    if(isRequestValid && isRequestAchieved && timestamp<=T-EXTRA_TIME){
                        //记录分数
                        double score = getScore(requestId,timestamp);
                        SCORES += score;
                        requests.get(requestId).isDone = true;
                        requests.get(requestId).readRequestInfo = new int[requests.get(requestId).readRequestInfo.length];
                        int objectId = requests.get(requestId).objectId;

                        //更新对象下降分数
                        if(storageObjects.get(objectId).requestsNum>0){
                            storageObjects.get(objectId).requestsNum -= 1;
                            if(requestId >= secondScoreCursor){
                                storageObjects.get(objectId).desScore -= 0.005F;
                            } else if (requestId >= firstScoreCursor) {
                                storageObjects.get(objectId).desScore -= 0.01F;
                            }

                        }

                        storageObjects.get(objectId).scores -= (double) score;
                        if(objectId == maxObject){
                            updateMaxScore();
                        }

                        delRequestLinkedList(requestId);
                        completedRequestsNum += 1;
                    }else if(!isRequestValid){
                        String message = String.format("选手输入错误：选手上报的读取请求%d已经上报过！",requestId);
                        System.out.println(message);
                        System.exit(1);
                    }else{
                        requests.get(requestId).isDone = true;
                        requests.get(requestId).readRequestInfo = new int[requests.get(requestId).readRequestInfo.length];
                        delRequestLinkedList(requestId);
                        String message = String.format("选手上报的读取请求%d不得分！",requestId);
                        System.out.println(message);
                    }
                }
            }

            generalInfo = new GeneralInformation(timestamp,SCORES,totalRequestsNum,abortRequestsNum,completedRequestsNum);
            diskPointInfo = new DiskPointInfomation(diskPoint,timestamp);

            endTime = System.currentTimeMillis();

            logicTime += (endTime-startTime);


            startTime = System.currentTimeMillis();

            updateDataBase();

            endTime = System.currentTimeMillis();

            dataBaseTime += (endTime-startTime);

            updateObjectScoresLevel();

            if(VISIBLE){
                myFrame.updateParams(disk, storageObjects,diskPoint,SCORES,abortRequestsNum,completedRequestsNum,totalRequestsNum,timestamp);
            }


        }

        System.out.printf("程序运行结束，选手得分为：%f%n",SCORES);
    }

    private static void doObjectDelete(int[] objectUnit, int[] diskUnit, int size) {
        for (int i = 1; i <= size; i++) {
            diskUnit[objectUnit[i]] = 0;
        }
    }

    private static void doObjectWrite(int[] objectUnit, int[] diskUnit, int size, int objectId, int timestamp) {
        for(int i=1;i<=size;i++){
            if(diskUnit[objectUnit[i]] == 0){
                diskUnit[objectUnit[i]] = objectId;
            }else{
                String message = String.format("选手输入错误：在写入对象%d时，目标写入位置已被占用！",objectId);
                SCORES = 0;
                updateObjectUnfinishedScore(timestamp);
                updateObjectScoresLevel();
                if(VISIBLE){
                    myFrame.updateParams(disk, storageObjects,diskPoint,SCORES,abortRequestsNum,completedRequestsNum,totalRequestsNum,timestamp);
                }
                System.out.println(message);
                System.exit(1);
            }
        }
    }

    private static int getObjectBlockIndex(int objectId,int diskIndex,int diskBlockIndex){
        int objectBlockIndex = 0;
        StorageObject storageObject = storageObjects.get(objectId);
        int[][] unit = storageObject.unit;
        int objectSize = unit[1].length-1;
        for(int i=1;i<=REP_NUM;i++){
            if(diskIndex != storageObject.replica[i]) continue;
            for(int j=1; j<=objectSize;j++){
                if(unit[i][j] == diskBlockIndex){
                    objectBlockIndex = j;
                }
            }
        }
        return objectBlockIndex;
    }

    private static boolean IsRequestAchieved(int requestId){
        boolean isAchieved = true;
        Request request = requests.get(requestId);
        int[] requestReadInfo = request.readRequestInfo;
        if(request.isAborted) isAchieved=false;
        if(!request.isGenerated) isAchieved=false;
        for(int i=1;i<requestReadInfo.length;i++){
            if(requestReadInfo[i] <= 0){
                isAchieved = false;
                break;
            }
        }
        return isAchieved;
    }

    private static double getScore(int requestId, int timeStamp){
        double f;
        double g = (storageObjects.get(requests.get(requestId).objectId).size+1)*0.5;
        double score = 0;
        int interval = timeStamp- requests.get(requestId).startTimeStamp;
        if(interval > 105){
            f = 0;
        } else if (interval > 10) {
            f = -0.01*interval+1.05;
        }else {
            f = -0.005*interval+1;
        }
        boolean isAchieved = IsRequestAchieved(requestId);
        if(isAchieved){
            score = f*g;
        }
        return score;
    }

    private static double getUnfinishedScore(int requestId, int timeStamp){
        double f;
        double g = (storageObjects.get(requests.get(requestId).objectId).size+1)*0.5;
        double score = 0;
        int interval = timeStamp- requests.get(requestId).startTimeStamp;
        if(interval > 105){
            f = 0;
        } else if (interval > 10) {
            f = -0.01*interval+1.05;
        }else {
            f = -0.005*interval+1;
        }
        boolean isAchieved = IsRequestAchieved(requestId);
        if(!isAchieved){
            score = f*g;
        }
        return score;
    }

    public static void updateObjectScoresLevel(){

        int levelNum = 4;
        double[] scoreLevelMax = new double[4];
        for(int i=0;i<levelNum;i++){
            scoreLevelMax[i] = maxObjectScore*(i+1)/levelNum;
        }
        for(int i=1;i<storageObjects.size();i++){
            if(storageObjects.get(i).size == 0) break;
            if(!storageObjects.get(i).isDelete){
                for(int j=1;j<=levelNum;j++){
                    if(storageObjects.get(i).scores <= scoreLevelMax[j-1]){
                        storageObjects.get(i).scoresLevel = j;
                        break;
                    }
                }
            }
        }
    }

    private static void updateObjectUnfinishedScore(int timeStamp){
        maxObjectScore = 0;
        for(int i=1;i<storageObjects.size();i++){
            storageObjects.get(i).scores = 0;
            if(!storageObjects.get(i).isDelete){
                int currentId = storageObjects.get(i).lastRequestPoint;
                int prevId = requests.get(currentId).prevId;
                //prevId链表的处理
                while(currentId !=0){
                    storageObjects.get(i).scores += (double) getUnfinishedScore(currentId,timeStamp);
                    currentId = prevId;
                    if(prevId != 0){
                        prevId = requests.get(prevId).prevId;
                    }

                }
            }
            maxObjectScore = Math.max(maxObjectScore, storageObjects.get(i).scores);

        }
    }

    public static void delRequestLinkedList(int requestId){
        int objectId = requests.get(requestId).objectId;
        int currentId = storageObjects.get(objectId).lastRequestPoint;
        int prevId = requests.get(currentId).prevId;
        //prevId链表的处理
        while(prevId !=0){
            if(prevId == requestId){
                requests.get(currentId).prevId = requests.get(requestId).prevId;//将被取消的请求的prevId链表结点删除
                requests.get(requestId).prevId = 0;//将被取消的请求的prevId设置为0
                break;
            }
            currentId = prevId;
            prevId = requests.get(prevId).prevId;
        }
    }


    public static MainFrame initInteractor(){
        MainFrame myFrame = new MainFrame(T,M,N,V,diskPoint,requests, storageObjects,disk,SCORES,abortRequestsNum,completedRequestsNum,totalRequestsNum,0);
        myFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        myFrame.setVisible(true);
        return myFrame;
    }

    public static void clearDataBase(){
        diskImpl.clearAll();
        diskPointImpl.clearAll();
        storageObjectImpl.clearAll();
        generalInfoImpl.clearAll();
//        ClearDataBaseRunnable clearDataBaserunnable = new ClearDataBaseRunnable();
//        Thread clearDataBaseThread = new Thread(clearDataBaserunnable);
//        clearDataBaseThread.start();
    }

    public static void clearWriteInfo(){
        diskOperationsList.clear();
        storageObjectionDeleteInfoList.clear();
        storageObjectionWriteInfoList.clear();
        generalInfo = new GeneralInformation();
        diskPointInfo = new DiskPointInfomation();
    }

    public static void CalculateScores(){
        maxObjectScore = 0;
        maxObject = 0;
        for(int i=1;i<storageObjects.size();i++){
            if(!storageObjects.get(i).isDelete && Math.abs(storageObjects.get(i).desScore)>=0.004){
                storageObjects.get(i).scores -= storageObjects.get(i).desScore*(storageObjects.get(i).size+1)*0.5F;
            }
            maxObject = maxObjectScore > storageObjects.get(i).scores ? maxObject:i;
            maxObjectScore = Math.max(maxObjectScore, storageObjects.get(i).scores);
        }
    }

    public static void updateMaxScore(){
        maxObjectScore = 0;
        maxObject = 0;
        for(int i=1;i<storageObjects.size();i++){
            maxObject = maxObjectScore > storageObjects.get(i).scores ? maxObject:i;
            maxObjectScore = Math.max(maxObjectScore, storageObjects.get(i).scores);
        }
    }

    public static void updateDataBase(){
//        UpdateDataBaseRunnable updateDataBaseRunnable = new UpdateDataBaseRunnable(diskOperationsList,storageObjectionWriteInfoList,storageObjectionDeleteInfoList,generalInfo,diskPointInfo);
//        Thread updateDataBaseThread = new Thread(updateDataBaseRunnable);
//        updateDataBaseThread.start();
        //磁盘操作写入数据库
        for(DiskOperation diskOperation : diskOperationsList){
            if(diskOperation.operationType.equals("delete")){
                diskImpl.addDiskDeleteInfo(diskOperation.timeStamp,diskOperation.objectId);
            }else if(diskOperation.operationType.equals("write")){
                diskImpl.addDiskWriteInfo(diskOperation.timeStamp,diskOperation.objectId);
            }
        }

        //数据库对象写入数据库
        for(StorageObjectionWriteInfo objWriteInfo : storageObjectionWriteInfoList){
            storageObjectImpl.addStorageObject(objWriteInfo.objectId,objWriteInfo.storageObjectl,objWriteInfo.writeTimeStamp);
        }

        for(StorageObjectionDeleteInfo objDeleteInfo : storageObjectionDeleteInfoList){
            storageObjectImpl.updateStorageObjectDeleteInfo(objDeleteInfo.objectId,objDeleteInfo.deleteTimeStamp);
        }

        //总信息写入数据库
        generalInfoImpl.addGeneralInformation(generalInfo);

        //磁头信息写入数据库
        diskPointImpl.addDiskPointInfo(diskPointInfo);

        clearWriteInfo();
    }


    public static int getT() {
        return T;
    }

    public static int getM() {
        return M;
    }

    public static int getN() {
        return N;
    }

    public static int getV() {
        return V;
    }

    public static int[][] getDisk() {
        return disk;
    }

    public static int[] getDiskPoint() {
        return diskPoint;
    }

    public static List<Request> getRequests() {
        return requests;
    }

    public static List<StorageObject> getObjects() {
        return storageObjects;
    }

    public static double getSCORES() {
        return SCORES;
    }

    public static int getTotalRequestsNum() {
        return totalRequestsNum;
    }

    public static int getCompletedRequestsNum() {
        return completedRequestsNum;
    }

    public static int getAbortRequestsNum() {
        return abortRequestsNum;
    }


}