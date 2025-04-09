package com.zd.GUI;

import javax.swing.*;
import java.awt.*;

public class DiskInfoPanel extends JPanel {
    public int DiskId;
    public int[] DiskHead;
    public int UnitId;
    public int Object;
    public int Tag;
    public int RequestNums;
    public double Score;

    public JTextArea ID;
    public JTextArea Head;
    public JTextArea unitInfo;
    public JTextArea scoreInfo;

    public DiskInfoPanel(int DiskId,int[] DiskHead,int UnitId,int Object,int Tag,int RequestNums,double Score){

        this.DiskId = DiskId;
        this.DiskHead = DiskHead;
        this.UnitId = UnitId;
        this.Object = Object;
        this.Tag = Tag;
        this.RequestNums = RequestNums;
        this.Score = Score;
        setLayout(new GridLayout(2,3));

        JTextArea Disk = new JTextArea("DISK");
        Disk.setEditable(false);
        Disk.setLineWrap(true);
        Disk.setWrapStyleWord(true);
        Disk.setOpaque(false);
        Disk.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
        JScrollPane diskPane = new JScrollPane(Disk);
        add(diskPane);

        ID = new JTextArea(String.format("ID: %d",this.DiskId));
        ID.setEditable(false);
        ID.setLineWrap(true);
        ID.setWrapStyleWord(true);
        ID.setOpaque(false);
        ID.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
        JScrollPane IDPane = new JScrollPane(ID);
        add(IDPane);

        StringBuilder diskHeadInfo = new StringBuilder("Head:");
        for (int j : DiskHead) {
            diskHeadInfo.append(String.format(" %d", j));
        }
        Head = new JTextArea(String.valueOf(diskHeadInfo));
        Head.setMinimumSize(new Dimension(20,40));
        Head.setEditable(false);
        Head.setLineWrap(true);
        Head.setWrapStyleWord(true);
        Head.setOpaque(false);
        Head.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
        JScrollPane HeadPane = new JScrollPane(Head);
        add(HeadPane);

        JTextArea Unit = new JTextArea("UNIT");
        Unit.setEditable(false);
        Unit.setLineWrap(true);
        Unit.setWrapStyleWord(true);
        Unit.setOpaque(false);
        Unit.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
        JScrollPane UnitPane = new JScrollPane(Unit);
        add(UnitPane);

        unitInfo = new JTextArea(String.format("ID:%d\nobj:%d\nTAG:%d",this.UnitId,this.Object,this.Tag));
        unitInfo.setEditable(false);
        unitInfo.setLineWrap(true);
        unitInfo.setWrapStyleWord(true);
        unitInfo.setOpaque(false);
        unitInfo.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
        JScrollPane unitInfoPane = new JScrollPane(unitInfo);
        add(unitInfoPane);

        scoreInfo = new JTextArea(String.format("Req Nums:%d\nScore:%.4f",this.RequestNums,this.Score));
        scoreInfo.setAlignmentX(Component.CENTER_ALIGNMENT);
        scoreInfo.setAlignmentY(Component.CENTER_ALIGNMENT);
        scoreInfo.setEditable(false);
        scoreInfo.setLineWrap(true);
        scoreInfo.setWrapStyleWord(true);
        scoreInfo.setOpaque(false);
        scoreInfo.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
        JScrollPane scoreInfoPane = new JScrollPane(scoreInfo);
        add(scoreInfoPane);

    }

    public void updateParams(int DiskId,int[] DiskHead,int UnitId,int Object,int Tag,int RequestNums,double Score){
        this.DiskId = DiskId;
        this.DiskHead = DiskHead;
        this.UnitId = UnitId;
        this.Object = Object;
        this.Tag = Tag;
        this.RequestNums = RequestNums;
        this.Score = Score;
        ID.setText(String.format("ID: %d",this.DiskId));
        StringBuilder headInfo = new StringBuilder("Head:");
        for (int j : this.DiskHead) {
            headInfo.append(String.format(" %d",j));
        }
        Head.setText(String.valueOf(headInfo));
        unitInfo.setText(String.format("ID:%d\nobj:%d\nTAG:%d",this.UnitId,this.Object,this.Tag));
        scoreInfo.setText(String.format("Req Nums:%d\nScore:%.4f",this.RequestNums,this.Score));

    }

    public void setDiskId(int diskId) {
        DiskId = diskId;
    }

}
