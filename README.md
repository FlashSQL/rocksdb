> Author: Gihwan Oh (wurikiji@gmail.com)

# Optimizing RocksDB

This project aims to optimize RocksDB's storage engine for almost-sequential log data (ASLog).

This project consists of two main components. One is to optimize compaction in RocksDB for *ASLog*. The other is to optimize a cleaning operation for data which have timestamp. 

You can see the progress [here](https://trello.com/b/sX6luTOp/rocksdb#)(\*Language: Korean). Most of documents are written in Korean.

## Optimize Compaction for ASLog

Refer to [Strategies for optimizing ASLog](Docs/manual/Almost Sequential Log Data (ASL) 최적화 방안).

 

# Other Information

#### RocksDB Code Analysis (Korean)

Refer to [code analysis](Docs/code_analysis). We only provide few docs of code written in Korean. Documents will be added gradually as we proceed our project. 



#### RocksDB wiki Korean translations

Refer to [translated wiki](Docs/wiki). We only provide wiki docs what we need on this project. 

