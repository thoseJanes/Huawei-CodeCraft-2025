# Git使用

1.安装git

2.下载初赛题目，解压

3.在解压目录下(任务书所在目录)打开gitBash（安装git后右键点击目录空白处即可选择打开gitBash）

4.键入pull命令拉取项目。
```
git pull https://github.com/birdsFloat/Competition.git main
```

5.其它操作
    
+ add &lt;filepath\>更新项目文件
+ commit -m "&lt;commitMessage\>"将文件在本地提交
+ status查看文件树
+ push &lt;project\> &lt;branch\>同步到github
    
>添加文件并同步到github示例
```
#添加新文件/有改动的文件到暂存区
git add data/dataInspect.ipynb
#提交到本地
git commit -m "Update dataInspect.ipynb. Add new function 'drawDoubleYAxis()'"
#查看当前文件树状态
git status
#同步github项目的mian分支到本地（push之前pull，防止本地还未同步到github最新修改，导致项目冲突无法push）
git pull https://github.com/birdsFloat/Competition.git main
#同步本地git项目到github服务器的main分支
git push https://github.com/birdsFloat/Competition.git main
```

>同步所有文件
```
#添加目录下所有新文件/存在修改的文件到暂存区（但是会忽略目录下.gitignore中匹配的文件）
git add .
#commit+pull+push
```

6.如果存在冲突无法提交项目，可以创建新分支

7.github貌似不能提交单个文件大小大于50mb的文件。
