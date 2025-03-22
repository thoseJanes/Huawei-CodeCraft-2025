package com.zd.GUI;

import com.zd.Util.Request;
import com.zd.Util.Object;

import javax.swing.*;
import javax.swing.border.Border;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.AffineTransform;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.beans.PropertyChangeSupport;
import java.util.ArrayList;
import java.util.List;

public class MainFrame extends JFrame {
    private DiskDisplayPanel displayPanel;
    private ScoreInfoPanel scorePanel;
    private List<ColorCheckBox> checkBoxes;
    private JTextField searchField;
    private JSlider progressSlider;
    private Color[][] gridColors;

    private float Score;
    private int AbortRequests;
    private int CompletedRequests;
    private int TotalRequests;

    Request[] requests;
    Object[] objects;
    int[][] disk;
    private int T, M,N,V;
    private int[] diskPoint;

    public MainFrame(int T,int M,int N,int V,int[] diskPoint,Request[] requests,Object[] objects,int[][]disk,float Score,int AbortRequests,int CompletedRequests,int TotalRequests) {
        this.T = T;
        this.M = M;
        this.N = N;
        this.V = V;
        this.diskPoint = diskPoint;
        this.requests = requests;
        this.objects = objects;
        this.disk = disk;
        this.Score = Score;
        this.AbortRequests = AbortRequests;
        this.CompletedRequests = CompletedRequests;
        this.TotalRequests = TotalRequests;

        setTitle("CodeCraft2025");
        setSize(1200, 800);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        initUI();
        repaint();
    }

    private void initUI() {
        // 主布局分为左右两部分
        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        splitPane.setDividerLocation(800);



        // 右侧控制面板
        JPanel rightPanel = new JPanel();
        rightPanel.setLayout(new BoxLayout(rightPanel,BoxLayout.Y_AXIS));
        Border border = BorderFactory.createLineBorder(Color.GRAY);

        //播放面板
        ReplayPanel playPanel = new ReplayPanel(0,T);
        playPanel.setBorder(border);



        //信息面板
        DiskInfoPanel diskPanel = new DiskInfoPanel(0,0,0,0,0,0,0);
        scorePanel = new ScoreInfoPanel(Score,AbortRequests,CompletedRequests,TotalRequests);
        diskPanel.setBorder(border);
        scorePanel.setBorder(border);


        // 左侧显示面板
        displayPanel = new DiskDisplayPanel(N,V,diskPanel,disk,objects,diskPoint);

        splitPane.setLeftComponent(displayPanel);

        //添加Tag标签面板
        ColorCheckBoxPanel tagPanel = new ColorCheckBoxPanel(M,displayPanel);
        tagPanel.setBorder(border);

        rightPanel.add(tagPanel);
        rightPanel.add(displayPanel.getDiskInfoPanel());
        rightPanel.add(scorePanel);
        rightPanel.add(playPanel);

        splitPane.setRightComponent(rightPanel);
        add(splitPane);
    }

    public void updateParams(int[][] disks,Object[] objects, int[] diskPoint, float Score, int abortRequests, int completedRequests, int totalRequests){
        displayPanel.updateParams(disks,objects,diskPoint);
        scorePanel.updateParams(Score, abortRequests, completedRequests, totalRequests);
    }

}
