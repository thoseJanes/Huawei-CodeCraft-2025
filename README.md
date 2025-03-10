# Competition

1.安装git

2.下载初赛题目，解压

3.在解压目录下(任务书所在目录)打开gitBash（安装git后右键点击目录空白处即可选择打开gitBash）

4.键入pull命令拉取项目。
```
git pull https://github.com/birdsFloat/Competition.git main
```

5.其它操作
    add \<filepath\>更新项目文件，commit -m "\<commitMessage\>"将文件在本地提交，status查看文件树，push \<project\> \<branch\>同步到github）
    >添加文件并同步到github示例
    ```
    git add data/dataInspect.ipynb
    git commit -m "Update dataInspect.ipynb. Add new function 'drawDoubleYAxis()'"
    git status
    git pull https://github.com/birdsFloat/Competition.git main 
    git push https://github.com/birdsFloat/Competition.git main
    ```

6.如果存在冲突无法提交项目，可以创建新分支

7.github貌似不能提交单个文件大小大于50mb的文件。
