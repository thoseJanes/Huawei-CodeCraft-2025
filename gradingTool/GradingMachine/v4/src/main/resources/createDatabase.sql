CREATE DATABASE `gradingmachine`;
USE `gradingmachine`;
DROP TABLE if exists `gradingmachine_disk`;
create table `gradingmachine_disk`(
`id` int not null auto_increment,
`time_stamp` int not null,
`operation_type` varchar(20) not null,
`object_id` int not null,
primary key (`id`)
)engine=InnoDB;

drop table if exists `gradingmachine_object`;
create table `gradingmachine_object`(
`object_id` int not null,
`size` int not null,
`tag` int not null,
`replica_1` varchar(40) not null,
`replica_2` varchar(40) not null,
`replica_3` varchar(40) not null,
`generate_timestamp` int not null,
`delete_timestamp` int null default 0,
primary key (`object_id`)
)engine=InnoDB;


drop table if exists `gradingmachine_diskpoint`;
create table `gradingmachine_diskpoint`(
`time_stamp` int not null auto_increment,
`disk_point` varchar(60) not null,
primary key (`time_stamp`)
)engine=InnoDB;

drop table if exists `gradingmachine_generalinformation`;
create table `gradingmachine_generalinformation`(
`time_stamp` int not null auto_increment,
`scores` float not null,
`aborted_requests` int not null,
`total_requests` int not null,
`finished_requests` int not null,
primary key (`time_stamp`)
)engine=InnoDB;