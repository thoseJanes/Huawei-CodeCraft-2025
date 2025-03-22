package com.zd.GUI;

import javax.swing.*;
import javax.swing.border.Border;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.IntStream;


public class ColorCheckBoxPanel extends JPanel{
    public int tagNum;
    public List<Color> ColorList = new ArrayList<>();
    public List<Integer> SelectedTagList = new ArrayList<>();
    public List<ColorCheckBox> ColorCheckBoxList = new ArrayList<>();
    public int SearchObject;
    public final static List<String> colors = Arrays.asList("#e6194B", "#3cb44b", "#ffe119", "#4363d8", "#f58231", "#911eb4", "#42d4f4", "#f032e6", "#bfef45", "#fabed4", "#469990", "#dcbeff", "#9A6324", "#800000", "#aaffc3", "#808000", "#ffd8b1", "#000075");
    public final static int COLNUM = 8;
    public final static int WEIGHTX = 100;
    public final static int WEIGHTY = 100;
    public DiskDisplayPanel diskDisplayPanel;
    public boolean isScoreMode = false;
    public boolean isHeadPosition = true;

    public ColorCheckBoxPanel(int tagNum,DiskDisplayPanel diskDisplayPanel){

        this.tagNum = tagNum;
        this.diskDisplayPanel = diskDisplayPanel;
        setLayout(new GridBagLayout());
        //添加文本显示标签
        int row_index = 0, col_index = 0;
        GridBagConstraints constraintTagLabel = new GridBagConstraints();
        constraintTagLabel.gridx = 0;
        constraintTagLabel.gridy = 0;
        constraintTagLabel.gridwidth = COLNUM;
        constraintTagLabel.gridheight = 2;
        constraintTagLabel.weightx = WEIGHTX;
        constraintTagLabel.weighty = WEIGHTY;
        constraintTagLabel.fill = GridBagConstraints.VERTICAL;
        constraintTagLabel.anchor = GridBagConstraints.WEST;
        row_index += constraintTagLabel.gridheight;
        JLabel tagLabel = new JLabel("Tag Selector",JLabel.RIGHT);
        add(tagLabel,constraintTagLabel);

        //添加ALL复选框
        GridBagConstraints constraint_ALL = new GridBagConstraints();
        constraint_ALL.gridx = 0;
        constraint_ALL.gridy = 2;
        constraint_ALL.gridwidth = COLNUM;
        constraint_ALL.gridheight = 1;
        constraint_ALL.weightx = WEIGHTX;
        constraint_ALL.weighty = WEIGHTY;
        row_index += constraintTagLabel.gridheight;
        constraint_ALL.fill = GridBagConstraints.VERTICAL;
        constraint_ALL.anchor = GridBagConstraints.WEST;
        JCheckBox allTagSelect = new JCheckBox("All");
        allTagSelect.setSize(16,16);
        allTagSelect.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                if(allTagSelect.isSelected()){
                    SelectedTagList = IntStream.range(1,tagNum+1).boxed().collect(Collectors.toList());
                    for(ColorCheckBox colorCheckBox : ColorCheckBoxList){
                        colorCheckBox.setSelected(true);
                    }
                    diskDisplayPanel.setAllColor(tagNum);
                }else{
                    SelectedTagList.clear();
                    for(ColorCheckBox colorCheckBox : ColorCheckBoxList){
                        colorCheckBox.setSelected(false);
                    }
                    diskDisplayPanel.clearAllColor();
                }
            }
        });
        add(allTagSelect,constraint_ALL);

        //添加Tag标签
        for (int i = 1; i <= tagNum; i++) {
            Color color = Color.decode(colors.get(i-1));
            ColorList.add(color);
            ColorCheckBox checkBox = new ColorCheckBox(String.valueOf(i),color);
            ColorCheckBoxList.add(checkBox);
            checkBox.addActionListener(new ActionListener() {
                @Override
                public void actionPerformed(ActionEvent e) {
                    if (checkBox.isSelected()) {
                        SelectedTagList.add(Integer.valueOf(checkBox.text));
                        diskDisplayPanel.addColorTag(Integer.parseInt(checkBox.text));
                    } else {
                        SelectedTagList.remove(Integer.valueOf(checkBox.text));
                        diskDisplayPanel.removeColorTag(Integer.parseInt(checkBox.text));
                    }
                }
            });
            GridBagConstraints constraint = new GridBagConstraints();
            constraint.gridx = col_index;
            constraint.gridy = row_index;
            constraint.fill = GridBagConstraints.VERTICAL;
            constraint.anchor = GridBagConstraints.WEST;
            constraint.gridheight = 1;
            constraint.gridwidth = 2;
            constraint.weightx = WEIGHTX;
            constraint.weighty = WEIGHTY;
            add(checkBox,constraint);
            row_index += (col_index+2)/COLNUM;
            col_index = (col_index+2)%COLNUM;

        }

        row_index ++;
        //添加对象搜索框和搜索按钮
        JTextField searchObject = new JTextField("Search Object...");
        GridBagConstraints search_constraints = new GridBagConstraints();
        search_constraints.gridx = 1;
        search_constraints.gridy = row_index;
        search_constraints.gridwidth = 4;
        search_constraints.gridheight = 2;
        search_constraints.fill = GridBagConstraints.HORIZONTAL;
        search_constraints.anchor = GridBagConstraints.CENTER;
        search_constraints.weightx = WEIGHTX;
        search_constraints.weighty = WEIGHTY;
        searchObject.addFocusListener(new FocusListener() {
            @Override
            public void focusGained(FocusEvent e) {
                if(searchObject.getText().equals("Search Object...")){
                    searchObject.setText(null);
                }
            }

            @Override
            public void focusLost(FocusEvent e) {
                if(searchObject.getText().isEmpty()){
                    searchObject.setText("Search Object...");
                }
            }
        });
        add(searchObject,search_constraints);

        JButton search_button= new JButton("Search");
        search_button.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                String object = searchObject.getText();
                try {
                    SelectedTagList.clear();
                    for(ColorCheckBox colorCheckBox : ColorCheckBoxList){
                        colorCheckBox.setSelected(false);
                    }
                    allTagSelect.setSelected(false);
                    SearchObject = Integer.parseInt(object);
                    diskDisplayPanel.addColorObject(SearchObject);
                } catch (NumberFormatException ex) {
                    throw new RuntimeException(ex);
                }
            }
        });
        search_constraints.gridx = search_constraints.gridx+search_constraints.gridwidth;
        search_constraints.gridwidth = GridBagConstraints.REMAINDER;
        col_index = 0;
        row_index += search_constraints.gridheight;
        add(search_button,search_constraints);

        //添加功能按钮
        GridBagConstraints button_constraints = new GridBagConstraints();
        button_constraints.gridx = 0;
        button_constraints.gridy = row_index;
        button_constraints.weightx = WEIGHTX;
        button_constraints.weighty = WEIGHTY;
        button_constraints.gridwidth = 2;
        button_constraints.gridheight = 1;
        button_constraints.fill = GridBagConstraints.HORIZONTAL;
        button_constraints.anchor = GridBagConstraints.CENTER;
        JButton clear_button = new JButton("Clear");
        clear_button.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                diskDisplayPanel.clearAllColor();
                searchObject.setText("Search Object...");
                allTagSelect.setSelected(false);
                for(int i=0;i<tagNum;i++){
                    ColorCheckBoxList.get(i).setSelected(false);
                }
            }
        });
        add(clear_button,button_constraints);
        JButton head_position_button = new JButton("Head Position");
        head_position_button.setOpaque(true);
        head_position_button.setBackground(Color.GREEN);
        head_position_button.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                isHeadPosition = !isHeadPosition;
                diskDisplayPanel.setHeadPosition(isHeadPosition);
                if(isHeadPosition){
                    head_position_button.setBackground(Color.GREEN);
                }else{
                    head_position_button.setBackground(Color.WHITE);
                }
            }
        });
        button_constraints.gridx = button_constraints.gridx+button_constraints.gridwidth;
        button_constraints.gridwidth = 3;
        add(head_position_button,button_constraints);
        button_constraints.gridx = button_constraints.gridx+button_constraints.gridwidth;
        button_constraints.gridwidth = COLNUM-button_constraints.gridx;
        JButton mode_button = new JButton("Score Mode");
        mode_button.setOpaque(true);
        mode_button.setBackground(Color.WHITE);
        mode_button.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                isScoreMode = !isScoreMode;
                diskDisplayPanel.setScoreMode(isScoreMode);
                if(isScoreMode){
                    mode_button.setBackground(Color.GREEN);
                }else{
                    mode_button.setBackground(Color.WHITE);
                }
            }
        });
        add(mode_button,button_constraints);
        row_index += 1;
        col_index = 0;

//        Border border = BorderFactory.createEtchedBorder();
//        setBorder(border);

    }

    public List<Color> getColorList(){
        return ColorList;
    }

    public List<ColorCheckBox> getColorCheckBoxList(){
        List<ColorCheckBox> BoxList = new ArrayList<>();
        for (int i = 1; i <= tagNum; i++) {
            ColorCheckBox checkBox = new ColorCheckBox(String.valueOf(i),ColorList.get(i));
            BoxList.add(checkBox);
        }
        return BoxList;
    }

    public static void main(String[] args) {
        EventQueue.invokeLater(()->{
            JFrame frame = new JFrame();
            frame.setTitle("ColorCheckBox");
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setVisible(true);
        });
    }
}
