package com.zd.Dao;

import com.mchange.v2.c3p0.ComboPooledDataSource;

import java.sql.*;

public class JdbUtils_C3P0 {
    private static ComboPooledDataSource ds = null;
    static{
        try{
            ds = new ComboPooledDataSource();
        }catch (Exception e){
            e.printStackTrace();
        }
    }

    //从连接池中获取连接
    public static Connection getConnection() {
        try {
            return ds.getConnection();
        } catch (SQLException e) {
            // TODO Auto-generated catch block
            throw new RuntimeException();
        }
    }

    //释放连接回连接池
    public static void closeResult(Connection conn, Statement stmt, ResultSet rs) {
        if (rs != null) {
            try {
                rs.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
            rs = null;
        }
        if (stmt != null) {
            try {
                stmt.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
            stmt = null;
        }
        if (conn != null) {
            try {
                conn.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
            conn = null;
        }
    }

    // 编写查询公共类
    public static ResultSet executeQuery(Connection connection, PreparedStatement preparedStatement , ResultSet rs, String sql, Object[] params) throws SQLException {
        // 预编译的SQL，在后面直接执行就行了
        preparedStatement = connection.prepareStatement(sql);

        for (int i = 0; i < params.length; i++) {
            // setObject,占位符从1开始，但是数组是从0开始
            preparedStatement.setObject(i+1,params[i]);
        }

        rs = preparedStatement.executeQuery();
        return rs;
    }

    // 编写查询公共类
    public static int executeUpdate(Connection connection, PreparedStatement preparedStatement, String sql, Object[] params) throws SQLException {
        preparedStatement = connection.prepareStatement(sql);

        for (int i = 0; i < params.length; i++) {
            // setObject,占位符从1开始，但是数组是从0开始
            preparedStatement.setObject(i+1,params[i]);
        }

        int updateRows = preparedStatement.executeUpdate();
        return updateRows;
    }

}
