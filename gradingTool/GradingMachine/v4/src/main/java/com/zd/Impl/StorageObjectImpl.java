package com.zd.Impl;

import com.zd.Dao.JdbUtils_C3P0;
import com.zd.Dao.StorageObjectDao;
import com.zd.Util.StorageObject;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class StorageObjectImpl implements StorageObjectDao {
    public static final int REP_NUM = 3;
    @Override
    public void addStorageObject(int objectId, StorageObject storageObject, int generate_timestamp) {

        PreparedStatement psmt = null;
        List<Object> paramList = new ArrayList<>(Arrays.asList(objectId, storageObject.size, storageObject.tag, generate_timestamp, null));

        StringBuilder insert = new StringBuilder("INSERT INTO gradingmachine_object(object_id,size,tag,generate_timestamp,delete_timestamp");
        StringBuilder value = new StringBuilder(") VALUES(?,?,?,?,?");

        for(int i=1;i<=REP_NUM;i++){
            insert.append(String.format(",replica_%d",i));
            value.append(",?");
            StringBuilder replica = new StringBuilder();
            replica.append(storageObject.replica[i]);
            for(int j=1;j<=storageObject.size;j++){
                replica.append(String.format(",%d",storageObject.unit[i][j]));
            }
            paramList.add(replica.toString());
        }
        value.append(");");

        Object[] params = paramList.toArray();
        Connection conn = null;
        String sql = String.valueOf(insert.append(value));

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
    public void updateStorageObjectDeleteInfo(int objectId, int deleteTimeStamp) {
        Connection conn = null;
        String sql = String.format("UPDATE gradingmachine_object SET delete_timestamp=%d WHERE object_id=%d;",deleteTimeStamp,objectId);
        PreparedStatement psmt = null;
        Object[] params = {};

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
    public StorageObject getStorageObject(int objectId, int timeStamp) {
        Connection conn = null;
        StorageObject storageObject = null;
        String sql = "select * from gradingmachine_object where object_id=?;";
        PreparedStatement psmt = null;
        Object[] params = {objectId};
        ResultSet rs = null;
        try{
            conn = JdbUtils_C3P0.getConnection();
            if(conn != null){
                rs = JdbUtils_C3P0.executeQuery(conn,psmt,rs,sql,params);
                if(rs.next()){
                    storageObject = new StorageObject();
                    storageObject.size = rs.getInt("size");
                    storageObject.tag = rs.getInt("tag");
                    storageObject.isDelete = rs.getInt("delete_timestamp") <= timeStamp;
                    int[] replica = new int[REP_NUM+1];
                    int[][] unit = new int[REP_NUM+1][];
                    unit[0] = new int[rs.getString("replica_1").split(",").length];
                    for(int i=1;i<=REP_NUM;i++){
                        String[] storageInfo = rs.getString(String.format("replica_%d",i)).split(",");
                        unit[i] = new int[storageInfo.length];
                        replica[i] = Integer.parseInt(storageInfo[0]);
                        for(int j=1;j<storageInfo.length;j++){
                            unit[i][j] = Integer.parseInt(storageInfo[j]);
                        }
                    }
                    storageObject.replica = replica;
                    storageObject.unit = unit;

                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            JdbUtils_C3P0.closeResult(conn,psmt,rs);
        }


        return storageObject;
    }

    @Override
    public List<StorageObject> getStorageObjectsAtTimeStamp(int timeStamp){
        Connection conn = null;
        List<StorageObject> storageObjectList = new ArrayList<>();
        storageObjectList.add(0,new StorageObject());
        String sql = "select * from gradingmachine_object;";
        PreparedStatement psmt = null;
        Object[] params = {};
        ResultSet rs = null;
        int size = 0;
        try{
            conn = JdbUtils_C3P0.getConnection();
            if(conn != null){
                rs = JdbUtils_C3P0.executeQuery(conn,psmt,rs,sql,params);
                while(rs.next()){
                    StorageObject _storageObject = new StorageObject();
                    _storageObject.size = rs.getInt("size");
                    _storageObject.tag = rs.getInt("tag");
                    _storageObject.isDelete = rs.getInt("delete_timestamp") >= timeStamp;
                    int[] replica = new int[REP_NUM+1];
                    int[][] unit = new int[REP_NUM+1][];
                    unit[0] = new int[rs.getString("replica_1").split(",").length];
                    for(int i=1;i<=REP_NUM;i++){
                        String[] storageInfo = rs.getString(String.format("replica_%d",i)).split(",");
                        unit[i] = new int[storageInfo.length];
                        replica[i] = Integer.parseInt(storageInfo[0]);
                        for(int j=1;j<storageInfo.length;j++){
                            unit[i][j] = Integer.parseInt(storageInfo[j]);
                        }
                    }
                    _storageObject.replica = replica;
                    _storageObject.unit = unit;
                    storageObjectList.add(_storageObject);
                    size++;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            JdbUtils_C3P0.closeResult(conn,psmt,rs);
        }

        StorageObject[] storageObjects = new StorageObject[size];
        return storageObjectList;
    }

    @Override
    public void clearAll() {
        Connection conn = null;
        PreparedStatement psmt = null;
        Object[] params = {};

        //String sql = "DELETE FROM gradingmachine_object;";
        String sql = "TRUNCATE TABLE gradingmachine_object;";
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
