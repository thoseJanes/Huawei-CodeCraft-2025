package com.zd;
import com.zd.UDP.Communication;
import com.zd.Util.Disk;
import com.zd.Util.Request;
import com.zd.Util.StorageObject;
import com.zd.Util.TagStorage;

import java.util.*;

public class Main {
    static boolean debug_flag = true;
    static String ToIP = "localhost";
    static int ToPort = 8080;
    static String IP = "localhost";
    static int Port = 8081;
    static Communication comm = new Communication(IP, Port, ToIP, ToPort);
    static  int FRE_COLUMN;
    static int T, M, N, V, G, K;
    static int[][] FRE_DEL;
    static int[][] FRE_WRITE;
    static int[][] FRE_STORAGE;
    static int[][] FRE_READ;
    static int[] FRE_TOTAL_WRITE;
    static int[] FRE_MAX_STORAGE;

    static final int FRE_PER_SLICING = 1800;

    static final int REP_NUM = (3);
    static final int DISK_HEAD_NUM = (2);

    static final int EXTRA_TIME = (105);

    static List<Disk> disks = new ArrayList<>();
    static Scanner scanner = new Scanner(System.in);

    static List<Request> requests = new ArrayList<>();

    static List<StorageObject> storageObjects = new ArrayList<>();
    static List<TagStorage> tagStorages = new ArrayList<>();

    static int currentRequest = 0;
    static int firstScoreCursor = 0;

    static int currentPhase = 0;
    static Integer[] tagPriority;
    static int[] initTagSize;
    static Boolean[][] isPass;
    static Integer timeStamp = 0;


    public static void main(String[] args) {
        if(debug_flag){
            String receive_data = null, send_data = null;
            receive_data = comm.ReceiveMessage();
            String[] init_args = receive_data.split("\\s+");
            T = Integer.parseInt(init_args[0]);
            M = Integer.parseInt(init_args[1]);
            N = Integer.parseInt(init_args[2]);
            V = Integer.parseInt(init_args[3]);
            G = Integer.parseInt(init_args[4]);
            K = Integer.parseInt(init_args[5]);

            send_data = "OK\n";
            comm.SendMessage(send_data);

        }else{
            T = scanner.nextInt();
            M = scanner.nextInt();
            N = scanner.nextInt();
            V = scanner.nextInt();
            G = scanner.nextInt();
            K = scanner.nextInt();
            System.out.println("OK");
            System.out.flush();
        }

        FRE_COLUMN = (int) Math.ceil(((double)T)/FRE_PER_SLICING);
        FRE_DEL = new int[M][FRE_COLUMN];
        FRE_WRITE = new int[M][FRE_COLUMN];
        FRE_READ = new int[M][FRE_COLUMN];
        FRE_STORAGE = new int[M][FRE_COLUMN];
        FRE_TOTAL_WRITE = new int[M];
        FRE_MAX_STORAGE = new int[M];
        initTagSize = new int[M];
        isPass = new Boolean[N][DISK_HEAD_NUM];
        for(int i=0;i<N;i++){
            for(int j=0;j<DISK_HEAD_NUM;j++){
                isPass[i][j] = false;
            }
        }

        for(int i=0;i<N;i++){
            disks.add(i,new Disk(V, G));
        }
        for(int i=0;i<M;i++){
            tagStorages.add(i,new TagStorage(N));
        }
        skipPreprocessing();
        //确定TAG优先级
        int[] read = new int[M];
        tagPriority = new Integer[M];
        for(int i=0;i<M;i++){
            for(int j=0;j<FRE_COLUMN;j++){
                read[i] += FRE_READ[i][j];
            }
            tagPriority[i] = (Integer) i;
        }
        Arrays.sort(tagPriority, Comparator.comparingInt(a -> read[a]));

        for (int t = 1; t <= T + EXTRA_TIME; t++) {
            timestampAction();
            deleteAction();
            writeAction();
            readAction();
            garbageCollection();
        }
        scanner.close();
    }

    private static void skipPreprocessing() {
        if(!debug_flag){
            for (int i = 0; i < M; i++) {
                for (int j = 0; j < FRE_COLUMN; j++) {
                    FRE_DEL[i][j] = scanner.nextInt();
                }
            }
            for (int i = 0; i < M; i++) {
                int sum = 0,max = 0, min=Integer.MAX_VALUE;

                for (int j = 0; j < FRE_COLUMN; j++) {
                    FRE_WRITE[i][j] = scanner.nextInt();
                    if(j>0){
                        FRE_STORAGE[i][j] = FRE_STORAGE[i][j-1]+FRE_WRITE[i][j]-FRE_DEL[i][j];
                    }else{
                        FRE_STORAGE[i][j] = FRE_WRITE[i][j]-FRE_DEL[i][j];
                    }
                    sum += FRE_STORAGE[i][j];
                    max = Math.max(max,FRE_STORAGE[i][j]);
                    min = Math.min(max,FRE_STORAGE[i][j]);
                    int mean = sum/FRE_COLUMN;

                }

                initTagSize[i] = (int) Math.ceil((max-min)*0.75);
                //initTagSize[i] = mean;

                for(int j=0;j<N;j++){
                    if(i>0){
                        tagStorages.get(i).left[j] = tagStorages.get(i-1).right[j]+1;
                    }else{
                        tagStorages.get(i).left[j] = 0;
                    }

                    if(j<M-1){
                        tagStorages.get(i).right[j] = tagStorages.get(i).left[j]+initTagSize[i]/N-1;
                    }else{
                        tagStorages.get(i).right[j] = tagStorages.get(i).left[j]+(initTagSize[i]%N+initTagSize[i]/N)-1;
                    }

                    tagStorages.get(i).resUnitNum[j] = tagStorages.get(i).right[j]-tagStorages.get(i).left[j]+1;

                }


            }
            for (int i = 0; i < M; i++) {
                for (int j = 0; j < FRE_COLUMN; j++) {
                    FRE_READ[i][j] = scanner.nextInt();
                }
            }


        }else{
            String receive_data = comm.ReceiveMessage();
            System.out.println("初始化的三个矩阵为：");
            System.out.println(receive_data);
            String[] init_matrix = receive_data.split("\n");
            for(int i=0; i<M; i++){
                String[] line = init_matrix[i].split("\\s+");
                for(int j=0; j<FRE_COLUMN;j++){
                    FRE_DEL[i][j] = Integer.parseInt(line[j]);
                }
            }

            int sum = 0;
            for(int i=0; i<M; i++){
                int max = 0, min=Integer.MAX_VALUE;
                String[] line = init_matrix[i+M].split("\\s+");
                for(int j=0; j<FRE_COLUMN;j++){
                    FRE_WRITE[i][j] = Integer.parseInt(line[j]);
                    FRE_TOTAL_WRITE[i] = FRE_TOTAL_WRITE[i]+FRE_WRITE[i][j];
                    if(j>0){
                        FRE_STORAGE[i][j] = FRE_STORAGE[i][j-1]+FRE_WRITE[i][j]-FRE_DEL[i][j];
                    }else{
                        FRE_STORAGE[i][j] = FRE_WRITE[i][j]-FRE_DEL[i][j];
                    }

                    max = Math.max(max,FRE_STORAGE[i][j]);
                    min = Math.min(min,FRE_STORAGE[i][j]);

                }
                sum += max;
                FRE_MAX_STORAGE[i] = max;
            }

            for(int i=0;i<M;i++){
                initTagSize[i] = (int) ((0.9*V*N)*FRE_MAX_STORAGE[i])/sum;
                for(int j=0;j<N;j++){
                    if(i>0){
                        tagStorages.get(i).left[j] = tagStorages.get(i-1).right[j]+1;
                    }else{
                        tagStorages.get(i).left[j] = 0;
                    }

                    if(j<N-1){
                        tagStorages.get(i).right[j] = tagStorages.get(i).left[j]+initTagSize[i]/N-1;
                    }else{
                        tagStorages.get(i).right[j] = tagStorages.get(i).left[j]+(initTagSize[i]%N+initTagSize[i]/N)-1;
                    }
                    tagStorages.get(i).resUnitNum[j] = tagStorages.get(i).right[j]-tagStorages.get(i).left[j]+1;
                }
            }


            for(int i=0; i<M; i++){
                String[] line = init_matrix[i+2*M].split("\\s+");
                for(int j=0; j<FRE_COLUMN;j++){
                    FRE_READ[i][j] = Integer.parseInt(line[j]);
                }
            }

        }

    }

    private static void timestampAction() {

        if(!debug_flag){
            scanner.next();
            timeStamp = scanner.nextInt();
            System.out.printf("TIMESTAMP %d\n", timeStamp);
            System.out.flush();
        }else{
            timeStamp = Integer.parseInt(comm.ReceiveMessage());
            comm.SendMessage(String.valueOf(timeStamp));
            String message = String.format("判题器的时间片为：TIMESTAMP %d，程序的时间片为：TIMESTAMP %d",timeStamp,timeStamp);
            System.out.println(message);
        }

        currentPhase = (timeStamp-1)/FRE_PER_SLICING;
        if ((timeStamp-1) % FRE_PER_SLICING == 0) {
            currentPhase = (timeStamp - 1) / FRE_PER_SLICING;
            Integer[] read = new Integer[M];
            Arrays.fill(read,0);

            //确定TAG优先级,tagPriority[n]代表第n个标签的优先级，数字越大，优先级越高

            for (int i = 0; i < M; i++) {
                for (int j = currentPhase; j < FRE_COLUMN; j++) {
                    read[i] += FRE_READ[i][j];
                }
                tagPriority[i] = (Integer) i;
            }
            Arrays.sort(tagPriority, Comparator.comparingInt(a -> read[(int) a]).reversed());
        }
        //调整center和size
//        if ((timestamp-1) % FRE_PER_SLICING == 0) {
//            currentPhase = (timestamp-1)/FRE_PER_SLICING;
//            //调整center和size
//            //确定TAG优先级
//            int[] read = new int[M];
//            Integer[] tagPriority = new Integer[M];
//            for(int i=0;i<M;i++){
//                for(int j=currentPhase;j<FRE_COLUMN;j++){
//                    read[i] += FRE_READ[i][j];
//                }
//                tagPriority[i] = (Integer) i;
//            }
//            Arrays.sort(tagPriority, Comparator.comparingInt(a -> read[a]));
//
//
//
//            for(int i=0;i<M;i++){
//                int tag = tagPriority[i];
//                TagStorage tagStorage = tagStorages.get(tag-1);
//                for(int j=0;j<N;j++){
//                    int center = tagStorage.tagCenters[j];
//                    int size = tagStorage.tagSize[j];
//                    int resStorageSize = (tagStorage.right[j]-tagStorage.left[j])-(tagStorage.realRight[j]-tagStorage.realLeft[j]);
//                    if(FRE_STORAGE[tag][currentPhase] > resStorageSize) {
//                        if(tag>0 && tag<M-1){
//                            (tagStorage.right[j]-tagStorage.left[j])-(tagStorage.realRight[j]-tagStorage.realLeft[j]);
//                        }
//
//                        int expandStorageSize = getExpandStorageSize(tag,diskId);
//                        if(expandStorageSize>=)
//
//                    }else{
//
//                    }
//                }
//            }
//        }



    }

    private static void deleteAction() {
        int nDelete;
        int abortNum = 0;
        int[] deleteObjectId;
        StringBuilder abortRequest = new StringBuilder();
        if(!debug_flag){
            nDelete = scanner.nextInt();
            deleteObjectId = new int[nDelete];
            for (int i = 0; i < nDelete; i++) {
                int id = scanner.nextInt();
                int tag = storageObjects.get(id-1).tag;
                int size = storageObjects.get(id-1).size;
                deleteObjectId[i] = id;
                int[] replica = storageObjects.get(id-1).replica;
                for(int j=0;j<REP_NUM;j++){
                    int[] objectUnit = storageObjects.get(id-1).unit[j];
                    int diskIndex = replica[j];
                    Integer[] diskUnit = disks.get(diskIndex).disks;
                    tagStorages.get(tag-1).resUnitNum[diskIndex] ++;
                    doObjectDelete(objectUnit, diskUnit, size, diskIndex);
                }
                storageObjects.get(id-1).isDelete = true;
                storageObjects.get(id-1).requestsNum = 0;

            }
        }else{
            nDelete = Integer.parseInt(comm.ReceiveMessage());
            deleteObjectId = new int[nDelete];
            if(nDelete > 0){
                String receive_data = comm.ReceiveMessage();
                String[] obj_id = receive_data.split("\n");
                for(int i=0; i<nDelete; i++){
                    int id = Integer.parseInt(obj_id[i]);
                    int size = storageObjects.get(id-1).size;
                    deleteObjectId[i] = id;
                    int[] replica = storageObjects.get(id-1).replica;

                    for(int j=0;j<REP_NUM;j++){
                        int[] objectUnit = storageObjects.get(id-1).unit[j];
                        int diskIndex = replica[j];
                        Integer[] diskUnit = disks.get(diskIndex).disks;
                        doObjectDelete(objectUnit, diskUnit, size, diskIndex);
                    }
                    storageObjects.get(id-1).isDelete = true;
                    storageObjects.get(id-1).requestsNum = 0;
                }
            }

        }

        for (int i = 0; i < nDelete; i++) {
            int id = deleteObjectId[i];
            int currentId = storageObjects.get(id-1).lastRequestPoint+1;
            while (currentId != 0) {
                if (!requests.get(currentId-1).isDone) {
                    abortNum++;
                    abortRequest.append(String.format("%d\n",currentId));
                    requests.get(currentId-1).isAborted = true;
                    requests.get(currentId-1).readRequestInfo = new int[requests.get(currentId-1).readRequestInfo.length];
                }
                currentId = requests.get(currentId-1).prevIndex+1;
            }
        }


        //删除阶段选手的输出
        if(!debug_flag){
            System.out.printf("%d\n", abortNum);
            if(abortRequest.length()>0){
                System.out.print(abortRequest);
            }
            System.out.flush();
        }else{
            comm.SendMessage(String.valueOf(abortNum));
            if(abortRequest.length() > 0){
                comm.SendMessage(String.valueOf(abortRequest));
            }
        }


    }

    private static void doObjectDelete(int[] objectUnit, Integer[] diskUnit, int size, int diskIndex) {
        for (int i = 0; i < size; i++) {
            int tag = 0;
            for(int k=0;k<M;k++){
                if(objectUnit[i]<=tagStorages.get(k).right[diskIndex] && objectUnit[i]>=tagStorages.get(k).left[diskIndex]){
                    tag = k+1;
                    break;
                }
            }
            diskUnit[objectUnit[i]] = -1;
            disks.get(diskIndex).diskResNum ++;

            if(tag>0){
                tagStorages.get(tag-1).resUnitNum[diskIndex] ++;
            }
        }
    }

    private static void writeAction() {
        int nWrite;
        String receive_data;
        String[] lines = null;
        if(!debug_flag){
            nWrite = scanner.nextInt();
        }else{
            nWrite = Integer.parseInt(comm.ReceiveMessage());
            if(nWrite>0){
                receive_data = comm.ReceiveMessage();
                lines = receive_data.split("\n");
            }

        }

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < nWrite; i++) {
            int id, size, tag;
            if(!debug_flag){
                id = scanner.nextInt();
                size = scanner.nextInt();
                tag = scanner.nextInt();
            }else{
                String[] line = lines[i].split("\\s+");
                id = Integer.parseInt(line[0]);
                size = Integer.parseInt(line[1]);
                tag = Integer.parseInt(line[2]);
            }

            storageObjects.add(id-1,new StorageObject(size,-1,false,tag,0,0.0F,0));

            //写入位置策略
            doWriteReplica(tag,id);

            if(!debug_flag){
                System.out.printf("%d\n", id);
                for (int j = 0; j < REP_NUM; j++) {
                    System.out.printf("%d", storageObjects.get(id-1).replica[j]+1);
                    for (int k = 0; k < size; k++) {
                        System.out.printf(" %d", storageObjects.get(id-1).unit[j][k]+1);
                    }
                    System.out.println();
                    System.out.flush();
                }
            }else{
                sb.append(String.format("%d\n",id));
                for (int j = 0; j < REP_NUM; j++) {
                    sb.append(String.format("%d",storageObjects.get(id-1).replica[j]+1));
                    for (int k = 0; k < size; k++) {
                        sb.append(String.format(" %d", storageObjects.get(id-1).unit[j][k]+1));
                    }
                    sb.append("\n");
                }
            }

        }
        if(sb.length()>0){
            comm.SendMessage(String.valueOf(sb));
        }
    }

    private static void doObjectWrite(int[] objectUnit, Integer[] diskUnit, int size, int objectId, int diskIndex) {
        int currentWritePoint = 0;
        int startIndex = tagStorages.get(M-1).right[diskIndex]+1;
        for (int i = startIndex; i < V; i++) {
            if (diskUnit[i] < 0) {
                diskUnit[i] = objectId-1;
                objectUnit[currentWritePoint++] = i;
                int tag = 0;
                for(int j=0;j<M;j++){
                    if(i<=tagStorages.get(j).right[diskIndex] && i>=tagStorages.get(j).left[diskIndex]){
                        tag = j+1;
                        break;
                    }
                }

                if(tag>0){
                    tagStorages.get(tag-1).resUnitNum[diskIndex] --;
                }

                disks.get(diskIndex).diskResNum --;

                if (currentWritePoint >= size) {
                    break;
                }
            }
        }

        if(currentWritePoint<size){
            for (int i = 0; i < startIndex; i++) {
                if (diskUnit[i] < 0) {
                    diskUnit[i] = objectId-1;
                    objectUnit[currentWritePoint++] = i;
                    int tag = 0;
                    for(int j=0;j<M;j++){
                        if(i<=tagStorages.get(j).right[diskIndex] && i>=tagStorages.get(j).left[diskIndex]){
                            tag = j+1;
                            break;
                        }
                    }

                    if(tag>0){
                        tagStorages.get(tag-1).resUnitNum[diskIndex] --;
                    }

                    disks.get(diskIndex).diskResNum --;

                    if (currentWritePoint >= size) {
                        break;
                    }
                }
            }
        }
    }

    private static void readAction() {
        int nRead;
        int requestId = 0, objectId;
        String[] lines = null;
        if(!debug_flag){
            nRead = scanner.nextInt();
        }else{
            nRead = Integer.parseInt(comm.ReceiveMessage());
        }
        if(nRead > 0){
            String receive_data = comm.ReceiveMessage();
            lines = receive_data.split("\n");
        }
        for (int i = 1; i <= nRead; i++) {
            if(!debug_flag){
                requestId = scanner.nextInt();
                objectId = scanner.nextInt();
            }else{
                String[] line = lines[i-1].split("\\s+");
                requestId = Integer.parseInt(line[0]);
                objectId = Integer.parseInt(line[1]);
            }

            int objectSize = storageObjects.get(objectId-1).size;
            int prevIndex = storageObjects.get(objectId-1).lastRequestPoint;
            requests.add(requestId-1,new Request(objectId-1,objectSize,prevIndex,timeStamp));
            storageObjects.get(objectId-1).lastRequestPoint = requestId-1;
        }
        if (currentRequest == 0 && nRead > 0) {
            currentRequest = requestId-1;
        }
        processCurrentReadRequest();
        processBusyReadRequest();
    }

    private static void processCurrentReadRequest() {
        StringBuilder diskAction = new StringBuilder();
        int completedRequestNum = 0;
        StringBuilder completedRequest = new StringBuilder();

        for(int i=0;i<N;i++) {
            for(int j=0; j<DISK_HEAD_NUM;j++){
                disks.get(i).token[j] = G;
                while (disks.get(i).token[j] > 0) {
                    int objectId = disks.get(i).disks[disks.get(i).head[j]]+1;
                    if (objectId != 0) {
                        int consume = disks.get(i).prevAction[j] == 'r' ? (int) Math.max(16, Math.ceil(0.8 * disks.get(i).prevToken[j])) : 64;
                        if (disks.get(i).token[j] >= consume) {
                            diskAction.append("r");
                            int objectBlockIndex = getObjectBlockIndex(objectId, i, disks.get(i).head[j]);
                            disks.get(i).token[j] -= consume;
                            disks.get(i).prevAction[j] = 'r';
                            disks.get(i).prevToken[j] = consume;
                            disks.get(i).head[j] = (disks.get(i).head[j] + 1) % V;
                            int currentIndex = storageObjects.get(objectId-1).lastRequestPoint;
                            System.out.println("!");

                            while (currentIndex >= 0) {
                                requests.get(currentIndex).readRequestInfo[objectBlockIndex] += 1;
                                Request request = requests.get(currentIndex);
                                boolean isRequestValid = (!requests.get(currentIndex).isAborted && !requests.get(currentIndex).isDone && !requests.get(currentIndex).isBusy);
                                boolean isRequestAchieved = IsRequestAchieved(currentIndex+1);
                                if(isRequestValid && isRequestAchieved && timeStamp<=T-EXTRA_TIME) {
                                    completedRequestNum++;
                                    completedRequest.append(currentIndex+1);
                                    completedRequest.append("\n");
                                    requests.get(currentIndex).isDone = true;
                                    requests.get(currentIndex).readRequestInfo = new int[requests.get(currentIndex).readRequestInfo.length];
                                }
                                currentIndex = requests.get(currentIndex).prevIndex;
                            }

                        } else {
                            break;
                        }
                    } else {
                        if (disks.get(i).token[j] >= 1) {
                            diskAction.append("p");
                            disks.get(i).token[j] -= 1;
                            disks.get(i).prevAction[j] = 'p';
                            disks.get(i).head[j] = (disks.get(i).head[j] + 1) % V;
                        }
                    }
                }
                diskAction.append("#\n");//结束尾字符#
            }


        }

        if(!debug_flag){
            System.out.print(diskAction);
            System.out.println(completedRequestNum);
            if(completedRequestNum>0){
                System.out.print(completedRequest);
            }

            System.out.flush();
        }else{
            comm.SendMessage(String.valueOf(diskAction));
            comm.SendMessage(String.valueOf(completedRequestNum));
            if(completedRequestNum>0){
                comm.SendMessage(String.valueOf(completedRequest));
            }
        }

    }

    private static void processBusyReadRequest(){
        int busyReqNum = 0;
        StringBuilder busyInfo = new StringBuilder();

        //处理超过105的请求,将这些请求存到busyInfo
        while(firstScoreCursor<requests.size() && timeStamp-requests.get(firstScoreCursor).startTimeStamp >= 105){
            if(!requests.get(firstScoreCursor).isAborted && !requests.get(firstScoreCursor).isDone && !requests.get(firstScoreCursor).isBusy){
                requests.get(firstScoreCursor).isBusy = true;
                busyInfo.append(firstScoreCursor+1);
                busyInfo.append("\n");
                busyReqNum++;
            }
            firstScoreCursor++;
        }



        if(!debug_flag){
            System.out.println(busyReqNum);
            if(busyReqNum>0){
                System.out.print(busyInfo);
            }
            System.out.flush();
        }else{
            comm.SendMessage(String.valueOf(busyReqNum));
            if(busyReqNum>0){
                comm.SendMessage(String.valueOf(busyInfo));
            }
        }
    }


    private static void doWriteReplica(int tag,int objectId){
        List<Integer> replicas = new ArrayList<>();
        int size = storageObjects.get(objectId-1).size;
        int replica_num = 0;

        Integer[] index = new Integer[N];
        for(int j=0; j<N;j++){
            index[j] = j;
        }

        Arrays.sort(index, (o1, o2) -> {
            int res1 = tagStorages.get(tag-1).resUnitNum[o1];
            int res2 = tagStorages.get(tag-1).resUnitNum[o2];
            return res2-res1;
        });


        for(int i=0;i<N;i++){
            int res = tagStorages.get(tag-1).resUnitNum[index[i]];
            if(res>=size){
                replicas.add(index[i]);

                //磁盘写入，对象replica记录，对象unit记录
                int write_num = 0;
                storageObjects.get(objectId-1).replica[replica_num] = index[i];
                for(int j=tagStorages.get(tag-1).left[index[i]];j<=tagStorages.get(tag-1).right[index[i]];j++){
                    if(disks.get(index[i]).disks[j] == -1){
                        disks.get(index[i]).disks[j] = objectId-1;
                        storageObjects.get(objectId-1).unit[replica_num][write_num] = j;
                        tagStorages.get(tag-1).resUnitNum[index[i]] --;
                        disks.get(index[i]).diskResNum --;
                        write_num++;
                    }
                    if(write_num>=size) break;
                }
                replica_num++;
            }

            if(replica_num>=REP_NUM){
                break;
            }
        }

        if(replica_num<REP_NUM){
            Integer[] diskRes = new Integer[N];
            Arrays.fill(diskRes,0);
            Integer[] diskIndex = new Integer[N];
            for(int i=0;i<N;i++){
                diskIndex[i] = i;
            }

            Arrays.sort(diskIndex, (o1, o2) -> {
                int res1 = disks.get(o1).diskResNum;
                int res2 = disks.get(o2).diskResNum;
                return res2-res1;
            });

            for(int i=0;i<N;i++){
                if(!replicas.contains(diskIndex[i]) && disks.get(index[i]).diskResNum>=size){
                    storageObjects.get(objectId-1).replica[replica_num] = diskIndex[i];
                    replicas.add(diskIndex[i]);
                    doObjectWrite(storageObjects.get(objectId-1).unit[replica_num],disks.get(diskIndex[i]).disks,size,objectId,diskIndex[i]);
                    replica_num++;
                }
                if(replica_num>=REP_NUM){
                    break;
                }
            }
        }


    }

    private static int getObjectBlockIndex(int objectId,int diskIndex,int diskBlockIndex){
        int objectBlockIndex = 0;
        StorageObject storageObject = storageObjects.get(objectId-1);
        int[][] unit = storageObject.unit;
        int objectSize = unit[0].length;
        for(int i=0;i<REP_NUM;i++){
            if(diskIndex != storageObject.replica[i]) continue;
            for(int j=0; j<objectSize;j++){
                if(unit[i][j] == diskBlockIndex){
                    objectBlockIndex = j;
                    break;
                }
            }
        }
        return objectBlockIndex;
    }

    private static boolean IsRequestAchieved(int requestId){
        boolean isAchieved = true;
        Request request = requests.get(requestId-1);
        int[] requestReadInfo = request.readRequestInfo;
        if(request.isAborted) isAchieved=false;
        if(request.isBusy) isAchieved=false;
        if(request.isDone) isAchieved=false;
        for(int i=0;i<requestReadInfo.length;i++){
            if(requestReadInfo[i] <= 0){
                isAchieved = false;
                break;
            }
        }
        return isAchieved;
    }

    private static void garbageCollection(){
        if(timeStamp % FRE_PER_SLICING == 0){
            String garbageCollection;
            StringBuilder garbageInfo = new StringBuilder();
            if(!debug_flag){
                garbageCollection = scanner.nextLine();
            }else{
                garbageCollection = comm.ReceiveMessage();
            }

            //回收机制处理
            for(int i=0;i<N;i++){
                garbageInfo.append(0);
                garbageInfo.append("\n");
            }

            //发送消息处理
            if(!debug_flag){
                System.out.print(garbageCollection);
                System.out.print(garbageInfo);
                System.out.flush();
            }else{
                comm.SendMessage(garbageCollection);
                comm.SendMessage(String.valueOf(garbageInfo));
            }

        }

    }
}