package com.zd.UDP;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;

public class Communication {
    private static final int MAX_UDP_PACKET_SIZE = 65507;
    int Port;
    String IP;
    DatagramSocket socket = null;
    int ToPort;
    String ToIP;

    //构建函数
    public Communication(String IP, int Port, String ToIP, int ToPort){
        this.IP = IP;
        this.Port = Port;
        this.ToIP = ToIP;
        this.ToPort = ToPort;
        try{
            this.socket = new DatagramSocket(Port);
            this.socket.setReceiveBufferSize(MAX_UDP_PACKET_SIZE);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    //向对方发送消息，只要传入想发送的字符串即可
    public void SendMessage(String send_data){
        byte[] send_datas = send_data.getBytes();
        DatagramPacket send_packet;
        InetSocketAddress client_socket_address = new InetSocketAddress(ToIP, ToPort);
        try{
            send_packet = new DatagramPacket(send_datas,0,send_datas.length,client_socket_address);
            String sender = Port == 8080 ? "Java" : "C++";
            String receiver = ToPort == 8080 ? "Java" : "C++";
            String message = String.format("%s To %s : %s",sender,receiver,send_data);
            System.out.println(message);
            this.socket.send(send_packet);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    //从对方接收消息
    public String ReceiveMessage(){
        String receive_data = null;
        try{
            byte[] receive_container = new byte[MAX_UDP_PACKET_SIZE];
            DatagramPacket recieve_packet = new DatagramPacket(receive_container, 0, receive_container.length);
            socket.receive(recieve_packet); //阻塞式接收包裹
            byte[] receive_datas = recieve_packet.getData();
            // 数据完整性检查
            if (recieve_packet.getLength() == MAX_UDP_PACKET_SIZE) {
                System.err.println("警告：接收缓冲区已满，数据可能被截断！");
            }
            receive_data = new String(receive_datas,0,recieve_packet.getLength());
            String sender = ToPort == 8080 ? "Java" : "C++";
            String receiver = Port == 8080 ? "Java" : "C++";
            String message = String.format("%s To %s : %s",sender,receiver,receive_data);
            System.out.println(message);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return receive_data;
    }
}
