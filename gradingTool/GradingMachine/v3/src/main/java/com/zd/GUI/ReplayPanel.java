package com.zd.GUI;

import javax.swing.*;
import javax.swing.border.Border;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;

public class ReplayPanel extends JPanel {
    public int currentSlice;
    public int totalSlice;
    public final static int COLNUM = 8;
    public final static int WEIGHTX = 100;
    public final static int WEIGHTY = 100;
    public ReplayPanel(int currentSlice, int totalSlice){
        this.currentSlice = currentSlice;
        this.totalSlice = totalSlice;
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
        JLabel playInfo = new JLabel(String.format("%d/%d",this.currentSlice,this.totalSlice));
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
        JProgressBar playBar = new JProgressBar();
        playBar.setMaximum(totalSlice);
        playBar.setMinimum(0);
        playBar.setValue(this.currentSlice);
        add(playBar,playBarConstraints);

        JButton next = new JButton("Next");
        next.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                if(ReplayPanel.this.currentSlice < ReplayPanel.this.totalSlice){
                    ReplayPanel.this.currentSlice = ReplayPanel.this.currentSlice+1;
                }else{
                    ReplayPanel.this.currentSlice = ReplayPanel.this.totalSlice;
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
                }else{
                    ReplayPanel.this.currentSlice = 1;
                }
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

    public static void main(String[] args){
        EventQueue.invokeLater(()->{
            JFrame frame = new JFrame();
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setSize(800,400);
            frame.setVisible(true);
            frame.add(new ReplayPanel(156,200));

        });
    }
}
