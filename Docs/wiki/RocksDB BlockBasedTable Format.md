# RocksDB BlockBasedTable Format

이 문서는 LevelDB의 [table format](https://github.com/google/leveldb/blob/master/doc/table_format.md) 페이지를 복제하여, RocksDB에서 변경한 내용들을 반영해 수정하였다. 

BlockBasedTable은 RocksDB에서 SST 테이블의 기본 포맷이다. 



### File format

```
<beginning_of_file>
[data block 1]
[data block 2]
...
[data block N]
[meta block 1: filter block]				("filter" Meta Block 참조)
[meta block 2: stats block]					("properties" Meta Block 참조)
[meta block 3: compression dictionary block]  ("compression dictionary" Meta Block 참조)
...
[meta block K: future extended block] (향 후 메타블록 추가를 대비하기 위함.)
[metaindex block]
[index block]
[Footer]							(fixed size; file_size - sizeof(Footer)를 통해 시작 offset 획득)
```

이 테이블로 구성된 파일은 내부적으로 `BlockHandles` 포인터를 가지고 있다. 내용은 아래와 같다.

```
offset:		varint64
size:		varint64
```

varint64 포맷에 대해서는 [이 문서](https://developers.google.com/protocol-buffers/docs/encoding#varints)를 참고하길 바란다.

1. key/value 페어들은 파일에 정렬된 순서로 저장되며, 일련의 데이터 블록들로 파티션이 나뉘어 진다. 데이터 블록은 파일의 시작부터 순서대로 저장되며, `block_builder.cc`에 정의된 포맷으로 구성된다. 해당 코드 파일의 주석을 통해서 블록 포맷을 확인할 수 있다. 
2. 데이터 블록의 저장이 끝난 이후에는 메타 블록들을 저장한다. 지원되는 메타 블록들은 본 문서의 아랫부분에 기술한다. 향 후 메타 블록이 추가 될 수도 있다. 메타 블록들의 포맷 또한 `block_builder.cc` 파일에 정의되어 있다.
3. `metaindex`는 메타블록에 대한 인덱스를 구성하고 있으며, key는 메타블록의 이름이고 value값은 `BlockHandle` 포인터이다. 
4. `index` 블록은 데이터 블록에 대한 인덱스를 구성하고 있다. key는 해당 데이터블록의 key값들 중 최대값과 같거나 가장 가까운 문자열을 사용하며, 밸류는 역시나 `BlockHandle` 포인터이다. [`kTwoLevelIndexSearch`](https://github.com/facebook/rocksdb/wiki/Partitioned-Index-Filters)를 사용할 경우 `index` 블록은 2nd level 인덱스를 가지고 있다. 이 말은, `index` 블록의 엔트리가 2차 인덱스 블록을 가리키는 포인터라는 의미이다. 이 경우 파일 포맷이 아래와 같이 변경된다.

```
[index block - 1st level]
[index block - 1st level]
...
[index block - 1st level]
[index block - 2nd level]
```

5. 각 파일의 마지막에는 항상 고정된 크기의 footer가 존재하며, `metaindex`블록 및 `index` 블록의 위치를 가리키는 `BlockHandle`과 매직 넘버로 구성되어 있다.

```
metaindex_handle:	char[p];		// metaindex를 위한 Block handle
index_handle:		char[q];		// index를 위한 Block handle
padding:			char[40-p-q];	// 0으로 초기화된 패딩 바이트. 
								   // (40bytes == 2 * BlockHandle::kMaxEncodedLength)
magic:				fixed64;		// 0x88e241b85f4cff7 (little-endian)
```



### `Filter` Meta Block

**Full filter**

Full filter일 경우 하나의 SST 파일에 한개의 필터 블록이 존재한다.

**Partitioned Filter**

Full filter가 여러개의 블록으로 파티셔닝 된 필터이다. 키에 따란 정확한 필터 블록 파티션을 찾기 위해 top-level 인덱스 블록이 추가된다. 자세한 내용은 [이곳](https://github.com/facebook/rocksdb/wiki/Partitioned-Index-Filters)을 참조.

**Block-based filter**

> Deprecated 되었으므로 해석을 하지 않는다. 



### `Properties` Meta Block

