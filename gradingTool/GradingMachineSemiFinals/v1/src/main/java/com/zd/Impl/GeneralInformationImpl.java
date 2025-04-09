package com.zd.Impl;

import com.zd.Dao.GeneralInformationDao;
import com.zd.Dao.JdbUtils_C3P0;
import com.zd.Util.GeneralInformation;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;

public class GeneralInformationImpl implements GeneralInformationDao {
    @Override
    public int addGeneralInformation(GeneralInformation generalInfo){
        String sql = "INSERT INTO gradingmachine_generalinformation(time_stamp,scores,aborted_requests,total_requests,finished_requests) " +
                " VALUES(?,?,?,?,?);";
        Object[] params = {generalInfo.timeStamp,generalInfo.Scores,generalInfo.abortedRequests,generalInfo.totalRequests,generalInfo.finishedRequests};
        Connection conn = null;
        PreparedStatement pstm = null;
        ResultSet rs = null;
        int updaterow = 0;
        try{
            conn = JdbUtils_C3P0.getConnection();
            updaterow = JdbUtils_C3P0.executeUpdate(conn,pstm,sql,params);

        }catch(Exception e){
            e.printStackTrace();
        }finally {
            JdbUtils_C3P0.closeResult(conn,pstm,rs);
        }
        return updaterow;
    }

    @Override
    public GeneralInformation getGeneralInformationAtTimeStamp(int timeStamp) {
        GeneralInformation generalInformation = null;

        Connection conn = null;
        PreparedStatement pstm = null;
        ResultSet rs = null;
        Object[] params = {timeStamp};
        String sql = "SELECT * FROM gradingmachine_generalinformation WHERE time_stamp = ?";

        try{
            conn = JdbUtils_C3P0.getConnection();
            if(conn != null){
                rs = JdbUtils_C3P0.executeQuery(conn,pstm,rs,sql,params);
                if(rs.next()){
                    double scores = rs.getDouble("scores");
                    int aborted_requests = rs.getInt("aborted_requests");
                    int total_requests = rs.getInt("total_requests");
                    int finished_requests = rs.getInt("finished_requests");
                    generalInformation = new GeneralInformation(timeStamp,scores,total_requests,aborted_requests,finished_requests);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            JdbUtils_C3P0.closeResult(conn,pstm,rs);
        }

        return generalInformation;
    }

    @Override
    public void clearAll() {
        Connection conn = null;
        PreparedStatement psmt = null;
        Object[] params = {};

        //String sql = "DELETE FROM gradingmachine_generalinformation;";
        String sql = "TRUNCATE TABLE gradingmachine_generalinformation;";
        try{
            conn = JdbUtils_C3P0.getConnection();
            JdbUtils_C3P0.executeUpdate(conn,psmt, sql,params);
            ;
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            JdbUtils_C3P0.closeResult(conn,psmt,null);
        }
    }
}
