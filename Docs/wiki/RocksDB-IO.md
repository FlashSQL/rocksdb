[홈페이지 원문](https://github.com/facebook/rocksdb/wiki/IO)

# RocksDB IO

RocksDB는 앞으로 발생할 I/O에 대해서 유저가 원하는 방식으로 동작하도록 할 수 있는 몇 가지 옵션들을 지원한다.

## Control Write I/O

### Range Sync

RocksDB에 저장되는 데이터들은 데이터 파일에 붙여쓰여지는 형태 (appending way)로 생성된다. 이 상황에서 파일 시스템은 메모리상에 dirty 페이지가 충분히 많아지고 threshold를 넘어갈 때까지 버퍼링한 뒤, 한번에 모든 dirty 페이지를 저장장치에 기록하게 된다. 이런 방식은 순간적으로 너무 많은 write I/O를 발생시키고, 실제 실시간 데이터 처리에 필요한 온라인 I/O 들이 앞선 write를 오랫동안 기다리게 하여 유저 쿼리에 대한 latency가 길어지는 문제가 있다. 이 문제를 해결하기 위해 RocksDB는 SST 파일에 대해  `options.bytes_per_sync` 옵션과 WAL 파일에 대해 `options.wal_bytes_per_sync` 옵션을 제공한다. 이 옵션을 통해서RocksDB는 주기적으로 OS에 힌트를 전달하여 미리 dirty 페이지를 비우도록 한다. 옵션을 통해 제공된 사이즈 만큼 버퍼링이 될때 마다 Linux에서 제공하는 `sync_file_range()`를 호출한다. Range sync의 대상 범위는 가장 오래된 위치 부터 옵션에서 지정한 크기 만큼이다.

### Rate Limiter

RocksDB는 `options.rate_limiter`를 통해서 전체 write 성능을 제한 할 수 있다. 이를 통해서 실시간 사용자의 온라인 쿼리 처리를 위한 온라인 I/O의 대역폭을 확보할 수 있다. 자세한 내용은 [Rate Limiter](https://github.com/facebook/rocksdb/wiki/Rate-Limiter) 페이지에서 확인할 수 있다.

### Write Max Buffer

RocksDB는 파일 시스템으로 데이터를 전달하기 전에 새롭게 추가되는 데이터를 자체적으로 버퍼링 하고, `fsync`가 호출 되거나 `options.writable_file_max_buffer_size` 옵션을 통해 주어진 크기만큼 데이터 쌓인 경우 파일 시스템으로 전달한다. 이 옵션은 `directIO`를 사용하거나 페이지 캐시가 없는 파일 시스템을 사용할 때 성능에 영향을 미치는 변수이다. [Direct IO](#Direct I/O) 모드가 아닌 경우에는 옵션을 크게 설정해도, `write()` 시스템 콜 호출 횟수만이 줄어 들 뿐 실제 I/O 패턴은 거의 변하지 않는다. 일반적으로 0으로 설정하여 전체 시스템 상에서 사용하는 메모리를 낭비하지 않도록 하는것이 좋다.



## Control Read I/O

### fadvise

SST 파일에 저장된 데이터를 읽어들이기 위해 파일을 `open`할때, `options.advise_random_on_open = true` 옵션을 통해 RocksDB가 `fadvise(FADV_RANDOM)` 을 호출하도록 할 수 있다. 값이 `false` 인 경우에는 `fadvise`를 호출하지 않는다. 매우 작은 단위의 `Get()`을 수행 하거나, 짧은 구간의 iteration을 수행할때는 read-ahead가 필요 없으므로 `fadvise`를 호출하는것이 도움이 된다. 그 외의 경우에는 read-ahead를 사용할 수 있도록 위 옵션을 `false`로 설정하여 `fadvise` 호출을 하지 않도록 하는 것이 성능에 도움이 된다.

단일 get이나 짧은 iteration과 대형 read가 섞인 혼합형 워크로드의 경우에는 이 옵션이 별 도움이 되지 않는다. 현재 RocksDB 자체적으로 read-ahead를 구현하여 이 문제를 해결하기 위해 프로젝트를 진행 중에 있다.

### Compaction inputs

컴팩션 대상이 되는 인풋 파일들은 매우 큰 순차 읽기 (long sequential reads)를 수행한다. 또한, 컴팩션이 끝난뒤에는 삭제되는 경우가 대부분이다. 이러한 형태의 Read I/O를 돕기 위해 RocksDB에서는 몇가지 옵션을 제공한다.

#### fadvise hint

`options.access_hint_on_compaction_start` 옵션 설정 여부에 따라, 컴팩션 인풋 대상이 된 파일들에 대해서 `fadvise` 호출을 설정하여 `FADV_RANDOM` 설정을 제거한다. 

#### Use a different file descriptor for compaction inputs

`options.new_table_reader_for_compaction_inputs = true` 옵션이 설정되어 있을 경우, 컴팩션 인풋 대상 파일들에 대해 새로운 파일 디스크립터를 생성하여 읽기 작업을 수행한다. 이러한 작업은 `fadvise` 옵션이 일반 파일과 섞이지 않도록 도움을 주지만, 새롭게 생성된 파일 디스크립터에 대해서 인덱스, 필터, 메타 블록을 다시 읽어와 메모리에 저장하는 작업을 유발하므로 추가적인 I/O가 발생하고 더 많은 memory를 요구한다.

#### readahead for compaction inputs

`options.compaction_readahead_size` 옵션을 통해서 컴팩션 발생시 자체적인 readahead를 수행하도록 설정할 수 있다. 이 옵션이 설정 될 경우 `options.new_table_reader_for_compaction_inputs = true`가 자동으로 설정된다. 파일 디스크립터를 새로 생성하므로 `options.access_hint_on_compaction_start` 옵션은 더 이상 사용하지 않게 된다. 

자체 readahead를 수행하는 것은 [DirectIO](#Direct I/O) 상황이나 readahead를 지원하지 않는 파일시스템 상황에서는 큰 도움이 된다. 



## Direct I/O

파일시스템에 I/O를 맡기지 않고 RocksDB가 생성한 I/O 패턴 그대로 사용하고 싶을 경우 direct I/O 모드를 사용하면 된다. `use_direct_reads` 옵션과 `use_direct_io_for_flush_and_compaction` 옵션을 통해 자신이 원하는 I/O에 대해서 direct I/O 모드를 설정 할 수 있다. Direct I/O 모드를 사용할 경우 위에서 설명한 I/O 옵션들이 사용되지 않을 수 있다. 자세한 내용은 [RocksDB Direct I/O](https://github.com/facebook/rocksdb/wiki/Direct-IO) 페이지에서 확인할 수 있다.



## Memory Mapping

`options.allow_mmap_reads` 와 `options.allow_mmap_writes`를 사용할 경우 RocksDB가 각각의 IO에 대해서 `mmap`을 수행하도록 할 수 있다. 이 옵션을 사용할 경우 `pread(), wrte()` 시스템 콜 호출 횟수를 줄일 수 있으며, 메모리 복사 작업도 줄일 수 있다. 

`ramfs`와 같은 메모리 파일시스템을 사용할때 `options.allow_mmap_reads` 옵션을 통해 메모리 복사 횟수를 줄이면 성능향상에 큰 도움이 된다. 그러나 그 외의 상황에서는 파일시스템의 mmap 처리 미숙으로 인해 성능 저하가 발생할 수 있으니, 상황에 맞춰서 꼭 필요한 경우에만 사용해야 한다.