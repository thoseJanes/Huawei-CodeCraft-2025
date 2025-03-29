package com.zd.GUI;

import com.zd.Util.StorageObject;
import com.zd.Util.Request;

import javax.swing.*;
import javax.swing.border.Border;
import java.awt.*;
import java.util.List;

public class MainFrame extends JFrame {
    private DiskDisplayPanel diskPanel;
    private ScoreInfoPanel scorePanel;
    private DiskInfoPanel diskInfoPanel;
    private ReplayPanel playPanel;
    private ColorCheckBoxPanel tagPanel;
    private List<ColorCheckBox> checkBoxes;
    private JTextField searchField;
    private JSlider progressSlider;
    private Color[][] gridColors;

    private double Score;
    private int AbortRequests;
    private int CompletedRequests;
    private int TotalRequests;

    List<Request> requests;
    List<StorageObject> storageObjects;
    int[][] disk;
    private int T, M,N,V;
    private int[] diskPoint;
    private int currTimeStamp;

    public MainFrame(int T, int M, int N, int V, int[] diskPoint, List<Request> requests, List<StorageObject> storageObjects, int[][]disk, double Score, int AbortRequests, int CompletedRequests, int TotalRequests, int currTimeStamp) {
        this.T = T;
        this.M = M;
        this.N = N;
        this.V = V;
        this.diskPoint = diskPoint;
        this.requests = requests;
        this.storageObjects = storageObjects;
        this.disk = disk;
        this.Score = Score;
        this.AbortRequests = AbortRequests;
        this.CompletedRequests = CompletedRequests;
        this.TotalRequests = TotalRequests;
        this.currTimeStamp = currTimeStamp;

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

        //信息面板
        diskInfoPanel = new DiskInfoPanel(0,0,0,0,0,0,0);
        scorePanel = new ScoreInfoPanel(Score,AbortRequests,CompletedRequests,TotalRequests);
        diskInfoPanel.setBorder(border);
        scorePanel.setBorder(border);


        // 左侧显示面板
        diskPanel = new DiskDisplayPanel(N,V, diskInfoPanel,disk, storageObjects,diskPoint);

        splitPane.setLeftComponent(diskPanel);

        //添加Tag标签面板
        tagPanel = new ColorCheckBoxPanel(M,diskPanel);
        tagPanel.setBorder(border);

        //播放面板
        playPanel = new ReplayPanel(0,T,diskPanel,diskInfoPanel,scorePanel,currTimeStamp);
        playPanel.setBorder(border);

        rightPanel.add(tagPanel);
        rightPanel.add(diskInfoPanel);
        rightPanel.add(scorePanel);
        rightPanel.add(playPanel);

        splitPane.setRightComponent(rightPanel);
        add(splitPane);
    }

    public void updateParams(int[][] disks, List<StorageObject> storageObjects, int[] diskPoint, double Score, int abortRequests, int completedRequests, int totalRequests, int currTimeStamp){
        diskPanel.updateParams(disks, storageObjects,diskPoint);
        scorePanel.updateParams(Score, abortRequests, completedRequests, totalRequests);
        playPanel.updateParams(currTimeStamp);
    }

}
