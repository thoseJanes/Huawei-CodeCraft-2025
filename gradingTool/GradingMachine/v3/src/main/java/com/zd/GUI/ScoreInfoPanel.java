package com.zd.GUI;

import javax.swing.*;
import java.awt.*;

public class ScoreInfoPanel extends JPanel {
    public double Score;
    public int AbortRequests;
    public int CompletedRequests;
    public int TotalRequests;
    JLabel ScoreLabel;
    JLabel AbortLabel;
    JLabel CompletedLabel;
    JLabel TotalLabel;
    public ScoreInfoPanel(double Score, int AbortRequests,int CompletedRequests,int TotalRequests){
        this.Score = Score;
        this.AbortRequests = AbortRequests;
        this.CompletedRequests = CompletedRequests;
        this.TotalRequests = TotalRequests;
        String ScoreInfo = String.format("Score:  %.4f 分",this.Score);
        String AbortInfo = String.format("AbortRequests:  %d",this.AbortRequests);
        String CompletedInfo = String.format("CompletedRequests:  %d",this.CompletedRequests);
        String TotalInfo = String.format("TotalRequests:  %d",this.TotalRequests);

        setLayout(new GridLayout(2,2));
        ScoreLabel = new JLabel(ScoreInfo);
        ScoreLabel.setHorizontalAlignment(SwingConstants.CENTER);
        ScoreLabel.setVerticalAlignment(SwingConstants.CENTER);
        ScoreLabel.setHorizontalTextPosition(SwingConstants.CENTER);
        ScoreLabel.setVerticalTextPosition(SwingConstants.CENTER);

        AbortLabel = new JLabel(AbortInfo);
        AbortLabel.setHorizontalAlignment(SwingConstants.CENTER);
        AbortLabel.setVerticalAlignment(SwingConstants.CENTER);
        AbortLabel.setHorizontalTextPosition(SwingConstants.CENTER);
        AbortLabel.setVerticalTextPosition(SwingConstants.CENTER);

        CompletedLabel = new JLabel(CompletedInfo);
        CompletedLabel.setHorizontalAlignment(SwingConstants.CENTER);
        CompletedLabel.setVerticalAlignment(SwingConstants.CENTER);
        CompletedLabel.setHorizontalTextPosition(SwingConstants.CENTER);
        CompletedLabel.setVerticalTextPosition(SwingConstants.CENTER);

        TotalLabel = new JLabel(TotalInfo);
        TotalLabel.setHorizontalAlignment(SwingConstants.CENTER);
        TotalLabel.setVerticalAlignment(SwingConstants.CENTER);
        TotalLabel.setHorizontalTextPosition(SwingConstants.CENTER);
        TotalLabel.setVerticalTextPosition(SwingConstants.CENTER);

        add(ScoreLabel);
        add(AbortLabel);
        add(CompletedLabel);
        add(TotalLabel);
    }

    public void updateParams(float Score, int abortRequests, int completedRequests, int totalRequests){
        this.Score = Score;
        this.AbortRequests = abortRequests;
        this.CompletedRequests = completedRequests;
        this.TotalRequests = totalRequests;
        String ScoreInfo = String.format("Score:  %.4f 分",this.Score);
        String AbortInfo = String.format("AbortRequests:  %d",this.AbortRequests);
        String CompletedInfo = String.format("CompletedRequests:  %d",this.CompletedRequests);
        String TotalInfo = String.format("TotalRequests:  %d",this.TotalRequests);
        ScoreLabel.setText(ScoreInfo);
        AbortLabel.setText(AbortInfo);
        CompletedLabel.setText(CompletedInfo);
        TotalLabel.setText(TotalInfo);
    }

}
