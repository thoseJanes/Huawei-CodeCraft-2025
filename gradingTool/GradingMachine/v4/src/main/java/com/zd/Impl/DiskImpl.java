package com.zd.Impl;

import com.zd.Dao.BaseDao;
import com.zd.Dao.DiskDao;
import com.zd.Dao.JdbUtils_C3P0;
import com.zd.Util.DiskOperation;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.ArrayList;
import java.util.List;

public class DiskImpl implements DiskDao {
    public static final int REP_NUM = 3;

    @Override
    public void addDiskDeleteInfo(int timeStamp,int objectId){
        String type = "delete";
        addDiskInfo(timeStamp,objectId,type);
    }

    @Override
    public void addDiskWriteInfo(int timeStamp, int objectId) {
        String type = "write";
        addDiskInfo(timeStamp,objectId,type);
    }

    @Override
    public void clearAll() {
        Connection conn = null;
        PreparedStatement psmt = null;
        Object[] params = {};


        //String sql = "DELETE FROM gradingmachine_disk;";
        String sql = "TRUNCATE TABLE gradingmachine_disk;";
        try{
            conn = JdbUtils_C3P0.getConnection();
            JdbUtils_C3P0.executeUpdate(conn,psmt, sql,params);
            
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            JdbUtils_C3P0.closeResult(conn,psmt,null);
        }
    }

    @Override
    //startTimeStamp包括，endTimeStamp不包括，即左闭右开
    public List<DiskOperation> getDiskOperation(int startTimeStamp, int endTimeStamp){
        List<DiskOperation> diskOperations = new ArrayList<>();
        Connection conn = null;
        PreparedStatement pstm = null;
        ResultSet rs = null;
        String sql = "SELECT * FROM gradingmachine_disk WHERE time_stamp BETWEEN ? AND ?;";
        Object[] params = {startTimeStamp+1, endTimeStamp};

        try{
            conn = JdbUtils_C3P0.getConnection();
            if(conn != null){
                rs = JdbUtils_C3P0.executeQuery(conn,pstm,rs,sql,params);
                while(rs.next()){
                    int objectId = rs.getInt("object_id");
                    int timeStamp = rs.getInt("time_stamp");
                    String operationType = rs.getString("operation_type");
                    DiskOperation _diskOperation = new DiskOperation(objectId,operationType,timeStamp);
                    diskOperations.add(_diskOperation);
                }

            }
        } catch (Exception e) {
            e.printStackTrace();
        }finally{
            JdbUtils_C3P0.closeResult(conn,pstm,rs);
        }
        return diskOperations;
    }

    public void addDiskInfo(int timeStamp, int objectId, String type){
        Connection conn = null;
        PreparedStatement psmt = null;
        Object[] params = {timeStamp,type,objectId};


        String sql = "INSERT INTO gradingmachine_disk(time_stamp,operation_type,object_id) VALUES(?,?,?);";
        try{
            conn = JdbUtils_C3P0.getConnection();
            JdbUtils_C3P0.executeUpdate(conn,psmt, sql,params);
            
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            JdbUtils_C3P0.closeResult(conn,psmt,null);
        }
    }

}

