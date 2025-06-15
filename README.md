# README
华为软件精英挑战赛，复赛18名。

本项目有三份代码，其中：  
BoBoXinXin负责判题器编写，代码主要位于gradingTool中  
Pierce-qiang编写了一个精简高效的策略，代码主要位于mycpp文件夹中  
thoseJanes编写了一个长期规划策略，代码主要位于linuxCode文件夹中，logging模块位于tools中，是从muduo库中改用的。

在复赛练习赛时，thoseJanes编写的策略得分较好，但在复赛正式赛时，题目进行了更新，磁盘时间步令牌数会随时间桶变化，长期规划方法依赖于均匀时间步，题目更新三小时内没能成功修正运行，**实际上成功运行的是Pierce-qiang编写的精悍策略**。

thoseJanes采用的策略介绍见：[规划策略](README/thoseJanes/2025华为软挑策略整理.md)
