package com.zd.GUI;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.AffineTransform;
import java.awt.geom.Point2D;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class DiskVisualization extends JFrame {
    private static final int DISK_SPACING = 50;
    private static final int UNIT_SIZE = 20;
    private static final Color DEFAULT_COLOR = Color.LIGHT_GRAY;

    private final List<Disk> disks = new ArrayList<>();
    private Point dragStart;
    private float scale = 1.0f;
    private Point offset = new Point(0, 0);

    public DiskVisualization(int diskCount, int unitsPerDisk) {
        setTitle("磁盘存储可视化");
        setSize(1200, 800);
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        initDisks(diskCount, unitsPerDisk);
        setupInteraction();
    }

    private void initDisks(int diskCount, int unitsPerDisk) {
        for (int i = 0; i < diskCount; i++) {
            disks.add(new Disk(i, unitsPerDisk));
        }
    }

    private void setupInteraction() {
        addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                dragStart = e.getPoint();
                handleClick(e);
            }
        });

        addMouseMotionListener(new MouseMotionAdapter() {
            @Override
            public void mouseDragged(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    Point current = e.getPoint();
                    offset.x += current.x - dragStart.x;
                    offset.y += current.y - dragStart.y;
                    dragStart = current;
                    repaint();
                }
            }
        });

        addMouseWheelListener(e -> {
            int rotation = e.getWheelRotation();
            scale = Math.max(0.5f, Math.min(3.0f, scale + (rotation < 0 ? 0.1f : -0.1f)));
            repaint();
        });
    }

    private void handleClick(MouseEvent e) {
        if (SwingUtilities.isLeftMouseButton(e)) {
            Point2D.Float logicPoint = toLogicPoint(e.getPoint());
            for (Disk disk : disks) {
                if (disk.contains(logicPoint)) {
                    int unitIndex = disk.getUnitIndexAt(logicPoint);
                    if (unitIndex != -1) {
                        disk.changeColor(unitIndex, getRandomColor());
                        repaint();
                        showUnitInfo(disk, unitIndex);
                    }
                }
            }
        }
    }

    @Override
    public void paint(Graphics g) {
        super.paint(g);
        Graphics2D g2d = (Graphics2D) g;

        AffineTransform originalTransform = g2d.getTransform();
        g2d.translate(offset.x, offset.y);
        g2d.scale(scale, scale);

        int x = DISK_SPACING;
        for (Disk disk : disks) {
            disk.draw(g2d, x, DISK_SPACING);
            x += disk.getWidth() + DISK_SPACING;
        }

        g2d.setTransform(originalTransform);
    }

    private Point2D.Float toLogicPoint(Point p) {
        return new Point2D.Float(
                (p.x - offset.x) / scale,
                (p.y - offset.y) / scale
        );
    }

    private Color getRandomColor() {
        return new Color(
                new Random().nextInt(256),
                new Random().nextInt(256),
                new Random().nextInt(256)
        );
    }

    private void showUnitInfo(Disk disk, int unitIndex) {
        String info = String.format("磁盘%d - 单元%d\n当前颜色: %s",
                disk.id + 1, unitIndex + 1,
                disk.getUnitColor(unitIndex));
        JOptionPane.showMessageDialog(this, info);
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            new DiskVisualization(10, 16384).setVisible(true);
        });
    }

    static class Disk {
        final int id;
        final int unitCount;
        final List<Color> unitColors;
        final int rows;
        final int cols;

        Disk(int id, int unitCount) {
            this.id = id;
            this.unitCount = unitCount;
            this.unitColors = new ArrayList<>(unitCount);

            // 初始化颜色
            for (int i = 0; i < unitCount; i++) {
                unitColors.add(DEFAULT_COLOR);
            }

            // 计算布局
            cols = (int) Math.sqrt(unitCount);
            rows = (unitCount + cols - 1) / cols;
        }

        void draw(Graphics2D g, int startX, int startY) {
            int unitSize = UNIT_SIZE;
            int padding = 2;

            // 绘制磁盘外框
            g.setColor(Color.DARK_GRAY);
            g.drawRect(startX, startY, getWidth(), getHeight());

            // 绘制存储单元
            for (int i = 0; i < unitCount; i++) {
                int row = i / cols;
                int col = i % cols;
                int x = startX + col * (unitSize + padding) + padding;
                int y = startY + row * (unitSize + padding) + padding;

                g.setColor(unitColors.get(i));
                g.fillRect(x, y, unitSize, unitSize);
            }
        }

        boolean contains(Point2D.Float point) {
            // 实现点击检测
            return false;
        }

        int getUnitIndexAt(Point2D.Float point) {
            // 计算点击位置对应的单元索引
            return -1;
        }

        void changeColor(int index, Color color) {
            unitColors.set(index, color);
        }

        int getWidth() {
            return cols * (UNIT_SIZE + 2) + 2;
        }

        int getHeight() {
            return rows * (UNIT_SIZE + 2) + 2;
        }

        Color getUnitColor(int index) {
            return unitColors.get(index);
        }
    }
}
