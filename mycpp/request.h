# pragma once


class Request {
    
public:    
    
    Request():object_id(-1),prev_id(-1),is_done(false),read_pos(1) {}
    
    int object_id; //请求的对象id
    int prev_id; //该对象的上一个请求id
    bool is_done; //是否完成
    int start_time; //请求开始时间
    int read_pos; // 准备读取位置
} ;