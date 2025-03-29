package com.zd.GUI;

import com.zd.Impl.DiskPointImpl;
import com.zd.Impl.GeneralInformationImpl;
import com.zd.Impl.StorageObjectImpl;
import com.zd.Service.DiskService;
import com.zd.ServiceImpl.DiskServiceImpl;
import com.zd.Util.GeneralInformation;
import com.zd.Util.StorageObject;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class ReplayPanel extends JPanel {
    private static final int EXTRA_TIME = 105;
    public DiskDisplayPanel diskDisplayPanel;
    public DiskInfoPanel diskInfoPanel;
    public ScoreInfoPanel scoreInfoPanel;
    public JProgressBar playBar;
    public JLabel playInfo;
    private int currTimeStamp;
    public int currentSlice;
    public int totalSlice;
    public final static int COLNUM = 8;
    public final static int WEIGHTX = 100;
    public final static int WEIGHTY = 100;

    public ReplayPanel(int currentSlice, int totalSlice, DiskDisplayPanel diskDisplayPanel,DiskInfoPanel diskInfoPanel, ScoreInfoPanel scoreInfoPanel,int currTimeStamp){
        this.currentSlice = currentSlice;
        this.totalSlice = totalSlice+EXTRA_TIME;
        this.currTimeStamp = currTimeStamp;
        this.diskDisplayPanel = diskDisplayPanel;
        this.diskInfoPanel = diskInfoPanel;
        this.scoreInfoPanel = scoreInfoPanel;
        setLayout(new GridBagLayout());
        GridBagConstraints playInfoConstraints = new GridBagConstraints();
        playInfoConstraints.gridx = 0;
        playInfoConstraints.gridy = 0;
        playInfoConstraints.gridwidth = COLNUM;
        playInfoConstraints.gridheight = 1;
        playInfoConstraints.weightx = WEIGHTX;
        playInfoConstraints.weighty = WEIGHTY;
        playInfoConstraints.fill = GridBagConstraints.VERTICAL;
        playInfoConstraints.anchor = GridBagConstraints.WEST;
        playInfo = new JLabel(String.format("%d/%d",this.currentSlice,this.totalSlice));
        add(playInfo,playInfoConstraints);

        GridBagConstraints playBarConstraints = new GridBagConstraints();
        playBarConstraints.gridx = 0;
        playBarConstraints.gridy = 1;
        playBarConstraints.gridwidth = COLNUM;
        playBarConstraints.gridheight = 1;
        playBarConstraints.weightx = WEIGHTX;
        playBarConstraints.weighty = WEIGHTY;
        playBarConstraints.fill = GridBagConstraints.BOTH;
        playBarConstraints.anchor = GridBagConstraints.WEST;
        playBar = new JProgressBar();
        playBar.setMaximum(totalSlice);
        playBar.setMinimum(0);
        playBar.setValue(this.currentSlice);
        add(playBar,playBarConstraints);

        JButton next = new JButton("Next");
        next.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int maxTimeStamp = Math.min(ReplayPanel.this.totalSlice,ReplayPanel.this.currTimeStamp);
                if(ReplayPanel.this.currentSlice <  maxTimeStamp){
                    ReplayPanel.this.currentSlice = ReplayPanel.this.currentSlice+1;
                    ReplayDataBaseRunnable replayRun = new ReplayDataBaseRunnable(diskDisplayPanel,diskInfoPanel,scoreInfoPanel,ReplayPanel.this.currentSlice,"Next");
                    Thread thread = new Thread(replayRun);
                    thread.start();
                }else{
                    ReplayPanel.this.currentSlice = maxTimeStamp;
                }
                playBar.setValue(ReplayPanel.this.currentSlice);
                playInfo.setText(String.format("%d/%d",ReplayPanel.this.currentSlice,ReplayPanel.this.totalSlice));
            }
        });
        GridBagConstraints buttonConstraints = new GridBagConstraints();
        buttonConstraints.gridx = COLNUM/2;
        buttonConstraints.gridy = 2;
        buttonConstraints.weightx = WEIGHTX;
        buttonConstraints.weighty = WEIGHTY;
        buttonConstraints.gridwidth = COLNUM/2;
        buttonConstraints.gridheight = 1;
        buttonConstraints.fill = GridBagConstraints.HORIZONTAL;
        buttonConstraints.anchor = GridBagConstraints.CENTER;
        add(next,buttonConstraints);

        JButton prev = new JButton("Prev");
        prev.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                if(ReplayPanel.this.currentSlice > 1){
                    ReplayPanel.this.currentSlice = ReplayPanel.this.currentSlice-1;
                    ReplayDataBaseRunnable replayRun = new ReplayDataBaseRunnable(diskDisplayPanel,diskInfoPanel,scoreInfoPanel,ReplayPanel.this.currentSlice,"Prev");
                    Thread thread = new Thread(replayRun);
                    thread.start();
                }else{
                    ReplayPanel.this.currentSlice = 1;
                }
                //更新DiskPlay和ScoreInfoPLane和DiskInfoPlane中的所有信息
                //首先计算参数disk、diskPoint等等
                //然后调用DiskPlay.updateParams()方法，ScoreInfoPLane.updateParams()方法、DiskInfoPlane.updateParams()方法


                playBar.setValue(ReplayPanel.this.currentSlice);
                playInfo.setText(String.format("%d/%d",ReplayPanel.this.currentSlice,ReplayPanel.this.totalSlice));
            }
        });
        buttonConstraints.gridx = 0;
        buttonConstraints.gridy = 2;
        buttonConstraints.weightx = WEIGHTX;
        buttonConstraints.weighty = WEIGHTY;
        buttonConstraints.gridwidth = COLNUM/2;
        buttonConstraints.gridheight = 1;
        buttonConstraints.fill = GridBagConstraints.HORIZONTAL;
        buttonConstraints.anchor = GridBagConstraints.CENTER;
        add(prev,buttonConstraints);

    }

    public void updateParams(int currTimeStamp){
        this.currentSlice = currTimeStamp;
        this.currTimeStamp = currTimeStamp;
        playBar.setValue(ReplayPanel.this.currentSlice);
        playInfo.setText(String.format("%d/%d",ReplayPanel.this.currentSlice,ReplayPanel.this.totalSlice));
    }



    public static void main(String[] args){
        EventQueue.invokeLater(()->{
            JFrame frame = new JFrame();
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setSize(800,400);
            frame.setVisible(true);
            //frame.add(new ReplayPanel(156,200));

        });
    }
}
