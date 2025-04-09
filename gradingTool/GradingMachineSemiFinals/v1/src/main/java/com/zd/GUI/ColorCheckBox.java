package com.zd.GUI;

import javax.swing.*;
import java.awt.*;

public class ColorCheckBox extends JCheckBox {
    String text;
    Color boxColor;
    public ColorCheckBox(String text, Color color){
        super(text);
        this.text = text;
        this.boxColor = color;
        setIcon(new ColorCheckBoxIcon());
    }

    // 设置复选框颜色
    public void setBoxColor(Color color) {
        this.boxColor = color;
        repaint();
    }



    class ColorCheckBoxIcon implements Icon{
        private final int SIZE = 14;

        public void paintIcon(Component c, Graphics g, int x, int y) {
            Graphics2D checkbox = (Graphics2D) g.create();
            checkbox.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);

            checkbox.setColor(Color.DARK_GRAY);
            checkbox.drawRect(x,y,SIZE-1,SIZE-1);
            checkbox.setColor(boxColor);
            checkbox.fillRect(x+1,y+1,SIZE-2,SIZE-2);

            // 绘制选中标记
            if (isSelected()) {
                checkbox.setColor(Color.BLACK);
                checkbox.setStroke(new BasicStroke(2));
                checkbox.drawLine(x+2, y+7, x+5, y+10);
                checkbox.drawLine(x+5, y+10, x+11, y+2);
            }

            checkbox.dispose();

        }

        @Override
        public int getIconWidth() {
            return SIZE;
        }

        @Override
        public int getIconHeight() {
            return SIZE;
        }

    }
}
