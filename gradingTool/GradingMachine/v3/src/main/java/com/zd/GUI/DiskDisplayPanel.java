package com.zd.GUI;

import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionAdapter;
import java.awt.event.MouseMotionListener;
import java.awt.geom.AffineTransform;
import java.awt.geom.Point2D;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import com.zd.Util.Object;

public class DiskDisplayPanel extends JPanel {
    public int DiskNum;
    public int UnitNum;

    private final int DISKINTERVAL_Y = 40;
    private final int DISKINTERVAL_X = 40;
    private final int UNITSIZE = 10;
    private final int PORTION = 10;
    private final int LINEWIDTH = 1;
    private float scale = 1.0f;
    private final int cols;
    private final int rows;
    private Point offset = new Point(0, 0);
    private Point dragStart;
    public int chosenDisk = 0;
    public int chosenUnit = 0;
    private boolean chosenFlag = false;
    private Point2D.Float chosenPoint;
    private int chosenLineWidth = 10;
    private DiskInfoPanel diskInfoPanel;
    private int[] diskPoint;
    private int[][] disks;
    private Object[] objects;
    private List<Integer> paintObjectId = new ArrayList<>();
    private List<Integer> paintTagId = new ArrayList<>();
    public final static List<String> colors = Arrays.asList("#e6194B", "#3cb44b", "#ffe119", "#4363d8", "#f58231", "#911eb4", "#42d4f4", "#f032e6", "#bfef45", "#fabed4", "#469990", "#dcbeff", "#9A6324", "#800000", "#aaffc3", "#808000", "#ffd8b1", "#000075");
    public boolean isScoreMode = false;
    public boolean isHeadPosition = true;
    public final static List<String> scoresLevelColor = Arrays.asList("#00FFFF","#00868B","#FFFF00","#FFA500","#FF0000","#8B0000");
    public DiskDisplayPanel(int DiskNum,int UnitNum,DiskInfoPanel diskInfoPanel,int[][] disks, Object[] objects, int[] diskPoint){
        this.DiskNum = DiskNum;
        this.UnitNum = UnitNum;

        this.cols = (int) Math.sqrt(UnitNum*PORTION);
        this.rows = (UnitNum+this.cols-1)/this.cols;
        this.diskInfoPanel = diskInfoPanel;

        this.disks = disks;
        this.objects = objects;
        this.diskPoint = diskPoint;

        //添加鼠标点击存储单元
        addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                dragStart = e.getPoint();
                //鼠标点击存储单元，还没有做！！！
                if (SwingUtilities.isLeftMouseButton(e)) {
                    Point2D.Float logicPoint = toLogicPoint(e.getPoint());
                    float relX = logicPoint.x - DISKINTERVAL_X;
                    float relY = logicPoint.y - DISKINTERVAL_Y;
                    if (!(relX < 0 || relY < 0 || relX > cols*UNITSIZE || relY > (rows*UNITSIZE+DISKINTERVAL_Y)*DiskNum-DISKINTERVAL_Y)) {
                        int col = (int) (relX/UNITSIZE);//从0开始
                        int row = (int) ((relY%(UNITSIZE*rows+DISKINTERVAL_Y))/UNITSIZE);//从0开始
                        int unitIndex = row*cols+col;//从0开始
                        if(row < rows && unitIndex<UnitNum){
                            chosenDisk = (int) (relY/(UNITSIZE*rows+DISKINTERVAL_Y)+1);//从1开始
                            chosenUnit = unitIndex+1;//从1开始

                            //绘制指标图像和绘制磁盘周围蓝色框线标志
                            chosenFlag = true;
                            float centerX = (float) (UNITSIZE*col+DISKINTERVAL_X+UNITSIZE/2.0);
                            float centerY = (float) ((chosenDisk-1)*(UNITSIZE*rows+DISKINTERVAL_Y)+row*UNITSIZE+DISKINTERVAL_Y+UNITSIZE/2.0);
                            chosenPoint = new Point2D.Float(centerX,centerY);
                            repaint();
                        }
                    }
                }
            }
        });

        //添加鼠标拖拽
        addMouseMotionListener(new MouseMotionAdapter() {
            @Override
            public void mouseDragged(MouseEvent e) {
                if (SwingUtilities.isLeftMouseButton(e)) {
                    Point current = e.getPoint();
                    offset.x += current.x - dragStart.x;
                    offset.y += current.y - dragStart.y;
                    dragStart = current;
                    repaint();
                }
            }
        });

        addMouseWheelListener(e -> {
            // 获取鼠标在屏幕上的坐标
            Point screenPoint = e.getPoint();

            // 转换为逻辑坐标（缩放前的坐标系）
            Point2D.Float logicPoint = toLogicPoint(screenPoint);

            // 计算新旧缩放比例
            float oldScale = scale;
            int rotation = e.getWheelRotation();
            float scaleDelta = (rotation < 0) ? 0.1f : -0.1f; // 向上滚动放大
            float newScale = Math.max(0.5f, Math.min(3.0f, oldScale + scaleDelta));

            if (newScale != oldScale) {
                // 调整偏移量保持鼠标点位置不变
                offset.x += (logicPoint.x * (oldScale - newScale));
                offset.y += (logicPoint.y * (oldScale - newScale));
                scale = newScale;
                repaint();
            }
        });


    }

    //绘制磁盘存储网格
    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        Graphics2D g2d = (Graphics2D) g;

        AffineTransform originalTransform = g2d.getTransform();
        g2d.translate(offset.x, offset.y);
        g2d.scale(scale, scale);

        int startX = DISKINTERVAL_X;
        int startY = DISKINTERVAL_Y;
        for(int i=0 ;i<DiskNum; i++){

            //绘制磁盘外框
            g2d.setColor(Color.DARK_GRAY);
            g2d.drawRect(startX-LINEWIDTH,startY-LINEWIDTH,cols*UNITSIZE+1,rows*UNITSIZE+1);

            //绘制存储单元
            for(int j=0; j<UnitNum; j++){
                Color paintColor;
                int objectId = disks[i+1][j+1];
                int objectTag = objects[objectId].tag;
                paintColor = Color.WHITE;

                if(!objects[objectId].isDelete){
                    if(!paintObjectId.isEmpty() && paintObjectId.contains(objectId)){
                        if(!isScoreMode){
                            paintColor = Color.decode(colors.get(objectTag-1));
                        }else{
                            if(objects[objectId].requestsNum > 0 && objects[objectId].scores > 0){
                                int scoreLevel = objects[objectId].scoresLevel;
                                paintColor = Color.decode(scoresLevelColor.get(scoreLevel+1));
                            }else if(objects[objectId].requestsNum > 0 && objects[objectId].scores == 0){
                                paintColor = Color.decode(scoresLevelColor.get(1));//设置颜色为深蓝色
                            }else{
                                paintColor = Color.decode(scoresLevelColor.get(0));//设置颜色为青色
                            }
                        }
                    }
                    if(!paintTagId.isEmpty() && paintTagId.contains(objectTag)){
                        if(!isScoreMode){
                            paintColor = Color.decode(colors.get(objectTag-1));
                        }else{
                            if(objects[objectId].requestsNum > 0 && objects[objectId].scores > 0){
                                int scoreLevel = objects[objectId].scoresLevel;
                                paintColor = Color.decode(scoresLevelColor.get(scoreLevel+1));
                            }else if(objects[objectId].requestsNum > 0 && objects[objectId].scores == 0){
                                paintColor = Color.decode(scoresLevelColor.get(1));//设置颜色为深蓝色
                            }else{
                                paintColor = Color.decode(scoresLevelColor.get(0));//设置颜色为青色
                            }
                        }
                    }

                }
                int row = j/cols;
                int col = j%cols;
                int x = startX+col*UNITSIZE;
                int y = startY+row*UNITSIZE;
                g2d.setColor(paintColor);
                g2d.fillRect(x,y,UNITSIZE-LINEWIDTH,UNITSIZE-LINEWIDTH);
            }

            //绘制磁头
            if(isHeadPosition){
                int row = (diskPoint[i+1]-1)/cols;
                int col = (diskPoint[i+1]-1)%cols;
                int center_x = startX+col*UNITSIZE+UNITSIZE/4;
                int center_y = startY+row*UNITSIZE+UNITSIZE/4;
                g2d.setColor(Color.black);
                g2d.fillOval(center_x,center_y,UNITSIZE/2,UNITSIZE/2);
            }

            startX = DISKINTERVAL_X;
            startY += (DISKINTERVAL_Y+rows*UNITSIZE);

        }

        if(chosenFlag){
            int objectId = disks[chosenDisk][chosenUnit];
            int objectTag = objects[objectId].tag;
            int requestsNum = objects[objectId].requestsNum;
            float scores = objects[objectId].scores;
            diskInfoPanel.updateParams(chosenDisk,diskPoint[chosenDisk],chosenUnit,objectId,objectTag,requestsNum,scores);
            // 绘制三角形
            int size = (int)(UNITSIZE/4);
            int[] xPoints = {
                    (int)chosenPoint.x,
                    (int)(chosenPoint.x - size),
                    (int)(chosenPoint.x + size)
            };
            int[] yPoints = {
                    (int)(chosenPoint.y - size),
                    (int)(chosenPoint.y + size),
                    (int)(chosenPoint.y + size)
            };
            g2d.setColor(Color.BLUE);
            g2d.fillPolygon(xPoints, yPoints, 3);

            //绘制粗边框
            Point rectLogic = new Point(DISKINTERVAL_X,DISKINTERVAL_Y+(chosenDisk-1)*(DISKINTERVAL_Y+rows*UNITSIZE));
            int width = cols*UNITSIZE;
            int height = rows*UNITSIZE;
            g2d.setStroke(new BasicStroke(chosenLineWidth));
            g2d.setColor(Color.ORANGE);
            g2d.drawRect(rectLogic.x-chosenLineWidth/2,rectLogic.y-chosenLineWidth/2,width+chosenLineWidth,height+chosenLineWidth);
        }else{
            diskInfoPanel.updateParams(0,0,0,0,0,0,0);
        }

        g2d.setTransform(originalTransform);
    }

    // 保持原有的坐标转换方法
    private Point2D.Float toLogicPoint(Point p) {
        return new Point2D.Float(
                (p.x - offset.x) / scale,
                (p.y - offset.y) / scale
        );
    }

    private Point2D.Float toScreenPoint(Point2D.Float p) {
        return new Point2D.Float(
                p.x*scale+offset.x,
                p.y*scale+offset.y
        );
    }

    public void updateParams(int[][] disks,Object[] objects, int[] diskPoint){
        this.disks = disks;
        this.objects = objects;
        this.diskPoint = diskPoint;
        if(this.chosenFlag){
            int objectId = disks[chosenDisk][chosenUnit];
            int objectTag = objects[objectId].tag;
            int requestsNum = objects[objectId].requestsNum;
            float score = objects[objectId].scores;
            this.diskInfoPanel.updateParams(chosenDisk,diskPoint[chosenDisk],chosenUnit,objectId,objectTag,requestsNum,score);
        }
        repaint();
    }

    public void setAllColor(int tagNum){
        paintObjectId.clear();
        paintTagId = IntStream.iterate(1, n -> n + 1)
                .limit(tagNum)
                .boxed()
                .collect(Collectors.toList());
        repaint();
    }

    public void clearAllColor(){
        paintObjectId.clear();
        paintTagId.clear();
        repaint();
    };
    public void addColorObject(int ObjectId){
        paintTagId.clear();
        paintObjectId.clear();
        paintObjectId.add(ObjectId);
        repaint();
    };
    public void removeColorObject(int ObjectId){
        paintObjectId.remove((Integer) ObjectId);
        repaint();
    };

    public void addColorTag(int Tag){
        paintTagId.add((Integer) Tag);
        repaint();
    };
    public void removeColorTag(int Tag){
        paintTagId.remove((Integer) Tag);
        repaint();
    };

    public DiskInfoPanel getDiskInfoPanel() {
        return diskInfoPanel;
    }

    public void setHeadPosition(boolean headPosition) {
        isHeadPosition = headPosition;
        repaint();
    }

    public void setScoreMode(boolean scoreMode) {
        isScoreMode = scoreMode;
        repaint();
    }

    public static void main(String[] args){
        EventQueue.invokeLater(()->{
            JFrame frame = new JFrame();
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setSize(800,400);
            frame.setVisible(true);

            //frame.add(new DiskDisplayPanel(10,5792,new DiskInfoPanel(0,0,0,0,0,0,0),disks));

        });
    }
}
