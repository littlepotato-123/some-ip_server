# 项目启动

需要先配置好对应的数据库

```mysql
//建立yourdb库
create database yourdb;

//创建someip表
use yourdb;
create table someip(
    ServiceId SMALLINT UNSIGNED,  
    MethodId SMALLINT UNSIGNED, 
    Length INT UNSIGNED, 
    ClientId SMALLINT UNSIGNED,  
    SessionId SMALLINT UNSIGNED, 
    MessageType TINYINT UNSIGNED,
    PayLoad varchar(1000),
    update_time timestamp  default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP COMMENT '更新时间'
    )engine=innoDB;	
```

```shell
make
./server
```

