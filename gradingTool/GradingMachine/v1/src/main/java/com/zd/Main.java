package com.zd;

import com.zd.UDP.Communication;
import com.zd.Util.Request;
import com.zd.Init.ParseMatrix;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Main {
    static int T, M, N, V, G;
    static  int FRE_COLUMN;
    static final String TIMESTAMP = "TIMESTAMP";
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

    static Request[] requests = new Request[MAX_REQUEST_NUM];

    static Object[] objects = new Object[MAX_OBJECT_NUM];

    static int currentRequest = 0;

    static int currentPhase = 0;
//    static {
//        for (int i = 0; i < MAX_REQUEST_NUM; i++) {
//            requests[i] = new Request();
//        }
//        for (int i = 0; i < MAX_OBJECT_NUM; i++) {
//            objects[i] = new Object();
//        }
//    }
    public static void main(String[] args) {
        //读取sample_practise.in文件并通过UDP输出
        String file_path = "src/main/resources/sample.in";
        List<String> lines = null;
        try{
            lines = Files.readAllLines(Paths.get(file_path));
        }catch (Exception e){
            e.printStackTrace();
        }

        //初始化T、M、N、V、G以及三个操作数量统计矩阵
        int[][] FRE_DEL = new int[M][FRE_COLUMN];
        int[][] FRE_WRITE = new int[M][FRE_COLUMN];
        int[][] FRE_READ = new int[M][FRE_COLUMN];
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
        FRE_DEL = ParseMatrix.ParseSingleMatrix(lines,1,1+M,FRE_COLUMN);
        FRE_WRITE = ParseMatrix.ParseSingleMatrix(lines,1+M,1+2*M,FRE_COLUMN);
        FRE_READ = ParseMatrix.ParseSingleMatrix(lines,1+2*M,1+3*M,FRE_COLUMN);

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

        int line_index = 1+3*M;
        for(int i=0;i<T+EXTRA_TIME;i++){
            //对齐时间片
            //发送时间片
            send_data = lines.get(line_index++);
            String timestamp = send_data.split("\\s+")[1];
            comm.SendMessage(timestamp);
            //接收时间片
            receive_data = comm.ReceiveMessage();
            if(!receive_data.equals(timestamp)){
                String message = String.format("时间片对齐失败，Java服务端时间片为:%s,C++客户端时间片为:TIMESTAMP %s",send_data,receive_data);
                System.out.println(message);
                System.exit(1);
            }

            //删除事件交互
            //JAVA向C++发送删除事件的输出
            send_data = lines.get(line_index++);//n_delete
            comm.SendMessage(send_data);
            if(!send_data.equals("0")){
                StringBuilder sb = new StringBuilder();
                int del_num = Integer.parseInt(send_data);
                for(int j = line_index;j<line_index+del_num;j++){
                    sb.append(lines.get(j));
                    sb.append("\n");
                }
                send_data = String.valueOf(sb);
                comm.SendMessage(send_data);
                line_index += del_num;
            }
            //删除事件接收C++消息，接收选手的消息
            int del_abort_num = Integer.parseInt(comm.ReceiveMessage());//接收被取消的读取请求数量
            if(del_abort_num>0){
                receive_data = comm.ReceiveMessage();//接收被取消的读取请求编号
            }



            for(int j=0; j<del_abort_num; j++){
                //处理被取消的读取请求编号，还没做，空着，不知道怎么处理

                //处理的注意事项如下：
                //***选手输出的请求编号必须为删除对象的当前未完成的读请求，否则判题器会报错。***
                //***选手需要保证n_abort与接下来输出的req_id数量一致，否则判题器可能会报错或一直等待选手输入而卡住***

            }


            //删除事件处理事宜，还没做，先空着

            //对象写入事件交互
            send_data = lines.get(line_index++);//n_write
            comm.SendMessage(send_data);
            if(!send_data.equals("0")){
                StringBuilder sb = new StringBuilder();
                int write_num = Integer.parseInt(send_data);
                for(int j = line_index;j<line_index+write_num;j++){
                    sb.append(lines.get(j));
                    sb.append("\n");
                }
                send_data = String.valueOf(sb);
                comm.SendMessage(send_data);
                line_index += write_num;

                //选手写入事件，当前时间片写入的对象数可能为0，此时选手无需有任何输出。
                receive_data = comm.ReceiveMessage();
                for(int j=0;j<write_num;j++){
                    //接收对象写入信息，需要记录磁盘的存储信息和对象的各个对象块的写入位置，但是还没有做
                    //注意事项如下：
                    //* 当前时间片写入的对象数可能为0，此时选手无需有任何输出。
                    //* 选手输出的写入对象顺序无需和输入保持一致，但是需要在该时间片处理所有的写入请求。
                    //* 选手输出的每个副本的存储单元数需要与该对象大小一致，否则判题器可能会报错或一直等待选手的输入而卡住。
                }
            }

            //对象读取事件交互
            send_data = lines.get(line_index++);//n_write
            comm.SendMessage(send_data);
            if(!send_data.equals("0")){
                StringBuilder sb = new StringBuilder();
                int read_num = Integer.parseInt(send_data);
                for(int j = line_index;j<line_index+read_num;j++){
                    sb.append(lines.get(j));
                    sb.append("\n");
                }
                send_data = String.valueOf(sb);
                comm.SendMessage(send_data);
                line_index += read_num;
            }

            //选手读取事件，选手输出三个磁盘动作
            receive_data = comm.ReceiveMessage();//接收磁盘的磁头动作，需要进行进一步处理，但是还没有做
            for(int j=0;j<N;j++){


            }
            //读取已经完成的读取请求数量
            receive_data = comm.ReceiveMessage();
            if(!receive_data.equals("0")){
                //读取已经完成的读取请求编号
                int n_rsp = Integer.parseInt(receive_data);
                receive_data = comm.ReceiveMessage();
                for(int j=0;j<n_rsp;j++){
                    //处理选手上报的已经完成的请求
                    //注意事项如下：
                    //* 一个请求只能上报一次读成功或者被取消，多次上报会被判题器报错
                    //* 在𝑇+105个时间分片后，未完成的读请求会被记为0分，不影响完成交互。
                    //* 请严格按照题目规定的格式输出，格式错误可能会被判题器判错。

                }
            }
        }

    }
}