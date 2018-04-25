# RocksDB Write Stalls

RocksDB는 온라인 요청으로 들어오는 write 비율을 flush나 compaction이 따라갈 수 없을 때, 쓰기 요청을 지연시키기 위한 시스템을 가지고 있다. 이러한 지연 시스템이 없을 경우, 하드웨어가 지원 가능한 대역폭 이상의 쓰기 요청을 수행할 시 데이터베이스는 아래와 같은 문제점을 겪을 수 있다.

- Space amplification이 증가하여, 디스크 용량 부족이 발생할 수 있다.
- Read amplification이 증가하여, 읽기 성능을 저하시킬 수 있다.

이를 방지하기 위한 기본 아이디어는 쓰기 요청 속도를 강제로 낮추어 데이터베이스가 충분히 처리할 수 있도록 만드는 것이다. 그러나 이 방식을 적용하여 자동적으로 쓰기 속도를 낮추도록 할 경우, 데이터베이스가 쓰기 요청을 잘못 이해하여, 아주 일시적인 순간의 대량 쓰기요청을 강제로 지연시키거나, 하드웨어의 기본 성능을 과소평가 하는 등의 문제로 인해, 예측하지 못한 성능 저하나 쿼리 타임아웃을 겪을 수도 있다.

실제로 현재 사용중인 데이터베이스가 write stall을 겪고 있는지 확인하기 위해서는 아래의 항목을 살펴보면 된다.

- LOG 파일을 살펴보면, write stall이 발생한 시점에 기록이 남겨져 있다.
- LOG 파일에 기록되는 [Compaction stats](https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide#compaction-stats)를 살펴보면 된다.

Write stall은 아래의 몇가지 이유로 인해 발생할 수 있다.

- **Too many memtables**. 플러쉬 되길 기다리는 memtable의 수가 `max_write_buffer_number` 옵션 이상일 경우, 다른 write는 모두 정지가 되고 flush가 끝날때까지 기다린다. 또한 `max_write_buffer_number`가 3 초과이고 플러쉬를 기다리는 memtable의 수가 `max_write_buffer_number - 1` 이상일 때, write stall이 발생한다. 이 경우에는 LOG파일에 다음과 같은 내용이 기록된다:

  > Stopping writes because we have 5 immutable memtables (waiting for flush), max_write_buffer_number is set to 5

  > Stalling writes because we have 4 immutable memtables (waiting for flush), max_write_buffer_number_is set to 5

- **Too many level-0 SST files**. L0 SST 파일의 갯수가 `level0_slowdown_writes_trigger`에 도달하면 write stall이 발생한다. `level0_stop_writes_trigger` 에 도달할 경우에는 write가 완전히 정지되고, L0 대상 컴팩션이 완료될 때까지 기다린다. 이 경우에는 아래와 같은 내용이 LOG파일에 기록된다:

  > Stalling writes because we have 4 level-0 files

  > Stopping writes becaused we have 20 level-0 files

- **Too many pending compaction bytes**. 컴팩션 대기 중인 파일의 크기가 `soft_pending_compaction_bytes`에 도달하면 write stall이 발생하고, `hard_pending_compaction_bytes`에 도달하면 write가 완전히 멈추고 컴팩션이 끝날 때 까지 기다린다. 이 경우에는 아래와 같은 내용이 LOG 파일에 기록된다.

  > Stalling writes because of estimated pending compaction bytes 500000000

  > Stopping writes because of estimated pending compaction bytes 1000000000

일단 stall이 되면 `delayed_write_rate` 옵션에 지정된 비율 만큼 write 성능을 제한하고, 컴팩션 대기중인 파일 사이즈가 매우 커진 경우에는 `delayed_write_rate` 보다 더 낮은 비율로 성능을 제한하기도 한다. Write stall 조건은 컬럼 패밀리별로 따로 판단을 하지만, 일단 하나의 컬럼패밀리라도 write stall이 되면 전체 데이터베이스에 대해서 write stall 작업을 수행한다. 경우에 따라서는 [Low Priority Write](https://github.com/facebook/rocksdb/wiki/Low-Priority-Write) 옵션을 통해서 write stall을 해결할 수 있다.

flush에 의해서 발생한 경우에는 `max_background_flushed`를 늘려서 플러쉬 쓰레드를 늘리거나,  `max_write_buffer_number`를 늘려서 memtable 크기를 줄이는 튜닝을 할 수 있다.

L0파일이 너무 많거나, 컴팩션 대기 파일이 너무 커서 발생한 경우에는, 컴팩션 속도가 write 속도를 따라가지 못하는 것이다. 이러한 경우에는 WAF를 줄이는 다양한 옵션을 적용해 볼 수 있다.

- `max_background_compactions`를 증거 시켜서, 더 많은 컴팩션 쓰레드가 활동하도록 한다.
- `write_buffer_size`를 증가 시켜서 memtable 크기를 키우고, 컴팩션 횟수를 줄여 WAF를 감소시킨다.
- `min_write_buffer_number_to_merge`를 증가 시킨다. 