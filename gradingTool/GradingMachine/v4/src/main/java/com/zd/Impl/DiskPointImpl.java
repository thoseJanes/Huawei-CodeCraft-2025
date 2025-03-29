package com.zd.Impl;

import com.zd.Dao.JdbUtils_C3P0;
import com.zd.Dao.DiskPointDao;
import com.zd.Util.DiskPointInfomation;
import com.zd.Util.StorageObject;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.ArrayList;
import java.util.List;

public class DiskPointImpl implements DiskPointDao {
    @Override
    public void addDiskPointInfo(DiskPointInfomation diskPointInfo) {
        int timeStamp = diskPointInfo.timeStamp;
        int[] diskPoint = diskPointInfo.diskPoint;
        Connection conn = null;
        PreparedStatement psmt = null;

        StringBuilder diskPointInfoString = new StringBuilder();
        for(int i=1;i<diskPoint.length;i++){
            diskPointInfoString.append(String.format("%d,",diskPoint[i]));
        }

        Object[] params = {timeStamp,String.valueOf(diskPointInfoString)};
        String sql = "INSERT INTO gradingmachine_diskpoint(time_stamp,disk_point) VALUES(?,?);";
        try{
            conn = JdbUtils_C3P0.getConnection();
            JdbUtils_C3P0.executeUpdate(conn,psmt,sql,params);
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            JdbUtils_C3P0.closeResult(conn,psmt,null);
        }
    }

    @Override
    public void clearAll() {
        Connection conn = null;
        PreparedStatement psmt = null;
        Object[] params = {};

        //String sql = "DELETE FROM gradingmachine_diskpoint;";
        String sql = "TRUNCATE TABLE gradingmachine_diskpoint;";
        try{
            conn = JdbUtils_C3P0.getConnection();
            JdbUtils_C3P0.executeUpdate(conn,psmt, sql,params);

        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            JdbUtils_C3P0.closeResult(conn,psmt,null);
        }
    }

    public int[] getDiskPointByTimestamp(int timestamp){
        Connection conn = null;
        StorageObject storageObject = null;
        String sql = "select * from gradingmachine_diskpoint where time_stamp=?";
        PreparedStatement psmt = null;
        Object[] params = {timestamp};
        ResultSet rs = null;
        int[] diskPoint = null;

        try{
            conn = JdbUtils_C3P0.getConnection();
            if(conn != null){
                rs = JdbUtils_C3P0.executeQuery(conn,psmt,rs,sql,params);
                if(rs.next()){
                    String[] diskPointInfo = rs.getString("disk_point").split(",");
                    diskPoint = new int[diskPointInfo.length+1];
                    diskPoint[0] = 0;
                    for(int i=1;i<=diskPointInfo.length;i++){
                        diskPoint[i] = Integer.parseInt(diskPointInfo[i-1]);
                    }
                }

            }
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            JdbUtils_C3P0.closeResult(conn,psmt,rs);
        }

        return diskPoint;
    }
}
