@echo off
::这几句是为防止乱码的
REM 
chcp 65001
CLS
echo =====当前的Trilium数据存储位置环境变量是=====
set TRILIUM_DATA_DIR

echo =====重新设置Trilium数据存储位置环境变量=====
::这个 H:\ProgramFiles\my-trilium-data 可以替换成 你想要的 位置
::这样每次启动就用 指定的 临时 环境变量
set TRILIUM_DATA_DIR=%~dp0\my_data

echo =====修改后Trilium数据存储位置环境变量是=====
set TRILIUM_DATA_DIR

echo =====后台启动trilium=====
start /b %TriliumPath%\trilium.exe