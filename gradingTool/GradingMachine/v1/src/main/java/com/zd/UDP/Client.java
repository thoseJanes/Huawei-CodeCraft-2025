package com.zd.UDP;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Scanner;

public class Client {
    static boolean debug_flag = true;
    static String ToIP = "localhost";
    static int ToPort = 8080;
    static String IP = "localhost";
    static int Port = 8081;
    static Communication comm = new Communication(IP, Port, ToIP, ToPort);
    static  int FRE_COLUMN;
    static int T, M, N, V, G;
    static int[][] FRE_DEL;
    static int[][] FRE_WRITE;
    static int[][] FRE_READ;


    static final int FRE_PER_SLICING = 1800;

    static final int MAX_DISK_NUM = (10 + 1);

    static final int MAX_DISK_SIZE = (16384 + 1);

    static final int MAX_REQUEST_NUM = (30000000 + 1);

    static final int MAX_OBJECT_NUM = (100000 + 1);

    static final int REP_NUM = (3);

    static final int EXTRA_TIME = (105);

    static int[][] disk = new int[MAX_DISK_NUM][MAX_DISK_SIZE];

    static int[] diskPoint = new int[MAX_DISK_NUM];

    static int[] objectIds = new int[MAX_OBJECT_NUM];

    static Scanner scanner = new Scanner(System.in);

    static Request[] requests = new Request[MAX_REQUEST_NUM];

    static Object[] objects = new Object[MAX_OBJECT_NUM];

    static int currentRequest = 0;

    static int currentPhase = 0;

    static class Request {
        int objectId;

        int prevId;

        boolean isDone;
    }

    static class Object {
        int[] replica = new int[REP_NUM + 1];

        int[][] unit = new int[REP_NUM + 1][];

        int size;

        int lastRequestPoint;

        boolean isDelete;
    }

    static {
        for (int i = 0; i < MAX_REQUEST_NUM; i++) {
            requests[i] = new Request();
        }
        for (int i = 0; i < MAX_OBJECT_NUM; i++) {
            objects[i] = new Object();
        }
    }

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
            FRE_COLUMN = (int) Math.ceil(((double)T)/FRE_PER_SLICING);
            FRE_DEL = new int[M][FRE_COLUMN];
            FRE_WRITE = new int[M][FRE_COLUMN];
            FRE_READ = new int[M][FRE_COLUMN];
            skipPreprocessing();
            send_data = "OK\n";
            comm.SendMessage(send_data);
            for (int i = 1; i <= N; i++) {
                diskPoint[i] = 1;
            }
        }else{
            T = scanner.nextInt();
            M = scanner.nextInt();
            N = scanner.nextInt();
            V = scanner.nextInt();
            G = scanner.nextInt();
            skipPreprocessing();
            skipPreprocessing();
            skipPreprocessing();
            System.out.println("OK");
            System.out.flush();
            for (int i = 1; i <= N; i++) {
                diskPoint[i] = 1;
            }
        }


        for (int t = 1; t <= T + EXTRA_TIME; t++) {
            timestampAction();
            deleteAction();
            writeAction();
            readAction();
        }
        scanner.close();
    }

    private static void skipPreprocessing() {
        if(!debug_flag){
            for (int i = 1; i <= M; i++) {
                for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
                    scanner.nextInt();
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
            for(int i=0; i<M; i++){
                String[] line = init_matrix[i+M].split("\\s+");
                for(int j=0; j<FRE_COLUMN;j++){
                    FRE_WRITE[i][j] = Integer.parseInt(line[j]);
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
            int timestamp = scanner.nextInt();
            System.out.printf("TIMESTAMP %d\n", timestamp);
            System.out.flush();
        }else{
            String timestamp = comm.ReceiveMessage();
            comm.SendMessage(timestamp);
            String message = String.format("判题器的时间片为：TIMESTAMP %s，程序的时间片为：TIMESTAMP %s",timestamp,timestamp);
            System.out.println(message);
        }

    }

    private static void deleteAction() {
        int nDelete;
        int abortNum = 0;;
        if(!debug_flag){
            nDelete = scanner.nextInt();
            for (int i = 1; i <= nDelete; i++) {
                objectIds[i] = scanner.nextInt();
            }
        }else{
            nDelete = Integer.parseInt(comm.ReceiveMessage());
            if(nDelete > 0){
                String receive_data = comm.ReceiveMessage();
                String[] obj_id = receive_data.split("\n");
                for(int i=1; i<=nDelete; i++){
                    objectIds[i] = Integer.parseInt(obj_id[i-1]);
                }
            }

        }

        for (int i = 1; i <= nDelete; i++) {
            int id = objectIds[i];
            int currentId = objects[id].lastRequestPoint;
            while (currentId != 0) {
                if (!requests[currentId].isDone) {
                    abortNum++;
                }
                currentId = requests[currentId].prevId;
            }
        }
        //删除阶段选手的输出
        if(!debug_flag){
            System.out.printf("%d\n", abortNum);
            for (int i = 1; i <= nDelete; i++) {
                int id = objectIds[i];
                int currentId = objects[id].lastRequestPoint;
                while (currentId != 0) {
                    if (!requests[currentId].isDone) {
                        System.out.printf("%d\n", currentId);
                    }
                    currentId = requests[currentId].prevId;
                }
                for (int j = 1; j <= REP_NUM; j++) {
                    doObjectDelete(objects[id].unit[j], disk[objects[id].replica[j]], objects[id].size);
                }
                objects[id].isDelete = true;
            }
            System.out.flush();
        }else{
            comm.SendMessage(String.valueOf(abortNum));
            StringBuilder sb = new StringBuilder();
            for (int i = 1; i <= nDelete; i++) {
                int id = objectIds[i];
                int currentId = objects[id].lastRequestPoint;
                while (currentId != 0) {
                    if (!requests[currentId].isDone) {
                        sb.append(String.format("%d\n",currentId));
                    }
                    currentId = requests[currentId].prevId;
                }
                for (int j = 1; j <= REP_NUM; j++) {
                    doObjectDelete(objects[id].unit[j], disk[objects[id].replica[j]], objects[id].size);
                }
                objects[id].isDelete = true;
            }
            if(sb.length() > 0){
                comm.SendMessage(String.valueOf(sb));
            }
        }


    }

    private static void doObjectDelete(int[] objectUnit, int[] diskUnit, int size) {
        for (int i = 1; i <= size; i++) {
            diskUnit[objectUnit[i]] = 0;
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
        for (int i = 1; i <= nWrite; i++) {
            int id, size;
            if(!debug_flag){
                id = scanner.nextInt();
                size = scanner.nextInt();
                scanner.nextInt();
            }else{
                String[] line = lines[i-1].split("\\s+");
                id = Integer.parseInt(line[0]);
                size = Integer.parseInt(line[1]);
            }

            objects[id].lastRequestPoint = 0;
            for (int j = 1; j <= REP_NUM; j++) {
                objects[id].replica[j] = (id + j) % N + 1;
                objects[id].unit[j] = new int[size + 1];
                objects[id].size = size;
                objects[id].isDelete = false;
                doObjectWrite(objects[id].unit[j], disk[objects[id].replica[j]], size, id);
            }

            if(!debug_flag){
                System.out.printf("%d\n", id);
                for (int j = 1; j <= REP_NUM; j++) {
                    System.out.printf("%d", objects[id].replica[j]);
                    for (int k = 1; k <= size; k++) {
                        System.out.printf(" %d", objects[id].unit[j][k]);
                    }
                    System.out.println();
                    System.out.flush();
                }
            }else{
                sb.append(String.format("%d\n",id));
                for (int j = 1; j <= REP_NUM; j++) {
                    sb.append(String.format("%d",objects[id].replica[j]));
                    for (int k = 1; k <= size; k++) {
                        sb.append(String.format(" %d", objects[id].unit[j][k]));
                    }
                    sb.append("\n");
                }


            }

        }
        if(sb.length()>0){
            comm.SendMessage(String.valueOf(sb));
        }
    }

    private static void doObjectWrite(int[] objectUnit, int[] diskUnit, int size, int objectId) {
        int currentWritePoint = 0;
        for (int i = 1; i <= V; i++) {
            if (diskUnit[i] == 0) {
                diskUnit[i] = objectId;
                objectUnit[++currentWritePoint] = i;
                if (currentWritePoint == size) {
                    break;
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

            requests[requestId].objectId = objectId;
            requests[requestId].prevId = objects[objectId].lastRequestPoint;
            objects[objectId].lastRequestPoint = requestId;
            requests[requestId].isDone = false;
        }
        if (currentRequest == 0 && nRead > 0) {
            currentRequest = requestId;
        }
        processCurrentReadRequest();
    }

    private static void processCurrentReadRequest() {
        int objectId;
        if(!debug_flag){
            if (currentRequest == 0) {
                for (int i = 1; i <= N; i++) {
                    System.out.println("#");
                }
                System.out.println("0");
            } else {
                currentPhase++;
                objectId = requests[currentRequest].objectId;
                for (int i = 1; i <= N; i++) {
                    if (i == objects[objectId].replica[1]) {
                        if (currentPhase % 2 == 1) {
                            System.out.printf("j %d\n", objects[objectId].unit[1][currentPhase / 2 + 1]);
                        } else {
                            System.out.println("r#");
                        }
                    } else {
                        System.out.println("#");
                    }
                }

                if (currentPhase == objects[objectId].size * 2) {
                    if (objects[objectId].isDelete) {
                        System.out.println("0");
                    } else {
                        System.out.printf("1\n%d\n", currentRequest);
                        requests[currentRequest].isDone = true;
                    }
                    currentRequest = 0;
                    currentPhase = 0;
                } else {
                    System.out.println("0");
                }
            }
            System.out.flush();
        }else{
            StringBuilder action = new StringBuilder();
            if (currentRequest == 0) {
                for (int i = 1; i <= N; i++) {
                    action.append("#\n");
                }
                comm.SendMessage(String.valueOf(action));
                comm.SendMessage("0");
            } else {
                currentPhase++;
                objectId = requests[currentRequest].objectId;
                for (int i = 1; i <= N; i++) {
                    if (i == objects[objectId].replica[1]) {
                        if (currentPhase % 2 == 1) {
                            action.append(String.format("j %d", objects[objectId].unit[1][currentPhase / 2 + 1]));
                        } else {
                            action.append("r#");
                        }
                    } else {
                        action.append("#");
                    }
                    action.append("\n");
                }
                comm.SendMessage(String.valueOf(action));

                if (currentPhase == objects[objectId].size * 2) {
                    if (objects[objectId].isDelete) {
                        comm.SendMessage("0");
                    } else {
                        comm.SendMessage("1");
                        comm.SendMessage(String.format("%d", currentRequest));
                        requests[currentRequest].isDone = true;
                    }
                    currentRequest = 0;
                    currentPhase = 0;
                } else {
                    comm.SendMessage("0");
                }
            }
        }

    }
}