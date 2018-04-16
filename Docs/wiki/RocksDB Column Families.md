[원문](https://github.com/facebook/rocksdb/wiki/Column-Families)

# RocksDB Column Families

`RocksDB 3.0` 버전부터 컬럼 패밀리 기능이 추가되었다.

각각의 key-value 쌍들은 하나의 컬럼 패밀리에 정확하게 매칭된다. 따로 컬럼 패밀리를 설정하지 않을 경우 `default` 컬럼 패밀리에 소속된다.

컬럼 패밀리는 데이터들을 논리적으로 파티션 분리를 하며 몇 가지 알아둬야할 특징이 있다.

- 서로 다른 컬럼 패밀리들 간의 atomic write를 보장한다. `Write({cf1, key1, val1}, {cf2, key2, value2})` 와 같이 여러 컬럼패밀리에 대해서 한번의 `Write()` 호출로 atomic write를 보장 할 수 있다.
- 컬럼 패밀리들 간의 consistent view가 보장된다.
- 서로 다른 컬럼 패밀리들 별로 각각 독립된 데이터베이스 설정 옵션을 적용할 수 있다.
- 새로운 컬럼 패밀리를 추가하거나 삭제하는 것은, 무시 가능한 수준으로 매우 빠른 연산이다.



## API

### Backward compatibility

컬럼 패밀리를 지원하기 위해서 코드 상에서 매우 많은 변화가 필요했지만, 여전히 기존 API들을 사용할 수 있게 제공한다. 이전 버전 사용자들은 RocksDB 3.0 버전 사용을 위해서 어떤 수정도 할 필요가 없다. 기존 API를 사용하는 key-value 쌍들은 모두 `default` 컬럼패밀리로 자동 설정 된다. 3.0 이후 버전에서 이전 버전으로 다운그레이드 하는 경우에도 자동적으로 마이그레이션 된다. 1개의 컬럼패밀리보다 많은 컬럼 패밀리를 사용하지 않을 경우, 디스크 포맷에는 아무런 변화가 없으며, 이는 3.0 이후 버전에서 2.X 버전으로 다운그레이드 하는데 아무런 문제가 없음을 의미한다. 상/하위 호완성은 모든 제품에서 매우 중요한 문제이므로, Facebook에서는 고객을 위해 최선의 노력을 다한다.

### Example usage

[https://github.com/facebook/rocksdb/blob/master/examples/column_families_example.cc](https://github.com/facebook/rocksdb/blob/master/examples/column_families_example.cc)

### Reference

```c++
/* Options, ColumnFamilyOptions, DBOptions */
```

위의 세가지 옵션 인자들은 `include/rocksdb/options.h`에 정의 되어 있다. `Options` 구조체는 RocksDB 전체의 동작에 관한 옵션들을 내포 하고 있다. 기존에는 `Options`에 모든 옵션들이 정의되어 있었지만, 이후 버전 부터는 컬럼패밀리에 대한 옵션은 `ColumnFamilyOptions`에 저장되고, RocksDB 전체 인스턴스에 대한 특수 옵션들은 `DBOptions`에 저장된다. `Options` 구조체는 `ColumnFamilyOptions`와 `DBOptions`를 상속하므로, 원하는 경우 `Options` 구조체에 모든 옵션을 정의할 수 있다. 



```c++
/* ColumnFamilyHandle */
```

`ColumnFamilyHandle` 구조를 통해서 각각의 컬럼 패밀리들을 접근하고 관리 할 수 있다. 일종의 파일 디스크립터와 역할이 비슷하다. DB pointer를 삭제하기 전해 `ColumnFamilyHandle`을 먼저 삭제 해야 하며, `ColumnFamilyHandle`이 이미 drop 된 컬럼패밀리를 포인팅 하고 있으면, 여전히 사용 가능하다. 실제 데이터 삭제는 해당 데이터를 가리키는 모든 `ColumnFamilyHandle`이 삭제되어야 수행된다.



```c++
DB::Open(const DBOptions& db_options, const std::string& name, 
         const std::vector<ColumnFamilyDescriptor>& column_families, 
         std::vector<ColumnFamilyHandle*>* handles, DB** dbptr);
```

데이터베이스를 읽기/쓰기 모드로 연결할때, 해당 데이터베이스에 존재하는 모든 컬럼패밀리를 정의해야 한다. 그렇지 않을 경우 `Status::InvalidArgument()`가 반환된다. `ColumnFamilyDescriptor`를 통해 정의하면 되는데, 이 구조체에는 단순히 컬럼 패밀리 이름과 `ColumnFamilyOptions`가 정의되어 있다. 데이터베이스 연결에 성공할 경우 `Status`와 함께 `handles` 벡터에 `ColumnFamilyHandle` 들이 전달된다. DB 포인터인 `dbptr`을 삭제 하기 전에 항상 `ColumnFamilyHandle`들을 모두 삭제해야 하는것을 명심해야 한다.



```c++
DB::OpenForReadOnly(const DBOptions& db_options, const std::string& name, 
                    const std::vector<ColumnFamilyDescriptor>& column_families, 
                    std::vector<ColumnFamilyHandle*>* handles, 
                    DB** dbptr, bool error_if_log_file_exist = false)
```

Read-only 모드로 데이터베이스를 연결할 때는, 모든 컬럼 패밀리를 정의할 필요 없이, 일부의 컬럼패밀리 만을 정의한뒤 연결을 시도해도 상관없다.



```c++
DB::ListColumnFamilies(const DBOptions& db_options, const std::string& name, 
                       std::vector<std::string>* column_families)
```

`ListColumnFamilies` 메소드는 현재 연결된 데이터베이스의 모든 컬럼패밀리 리스트를 반환하는 메소드이다. 



```c++
DB::CreateColumnFamily(const ColumnFamilyOptions& options, 
                       const std::string& column_family_name, 
                       ColumnFamilyHandle** handle){}
    
DropColumnFamily(ColumnFamilyHandle* column_family){}
```

`CreateColumnFamily` 메소드는 전달된 옵션을 통해 컬럼 패밀리를 생성하고 `ColumnFamilyHandle`을 전달해주며, `DropColumnFamily` 메소드는 선택된 컬럼패밀리를 삭제하는 역할을 한다. 실제 데이터의 삭제는 `delete column_family;`를 통해 컬럼패밀리를 포인팅 하고 있는 포인터를 삭제해야지 발생한다.



```c++
DB::NewIterators(const ReadOptions& options, 
                 const std::vector<ColumnFamilyHandle*>& column_families,
                 std::vector<Iterator*>* iterators)
```

`NewIterators` 메소드는 다중 컬럼 패밀리를 동시에 접근하면서 consistent view를 제공해주는 이터레이터를 생성한다.



#### Write Batch

`WriteBatch`를 생성한뒤 `WriteBatch`의 API들을 사용할 경우, 다중 컬럼 패밀리들에 대해서 멀티플 write를 atomic하게 처리할 수 있다.



#### All other API Calls

다른 거의 대부분의 API에도 `ColumnFamilyHandle*`을 인자로 받는 메소드가 오버로딩 되어있으므로, 원하는 경우 특정한 컬럼패밀리를 대상으로 요청을 전달할 수 있다.



## Implementation

컬럼 패밀리를 구성하는 기본 아이디어중 하나는 WAL (write-ahead log)를 공유하되, memtable과 table file을 공유하지 않는 것이다. WAL을 공유할 경우 다중 컬럼 패밀리에 대해서 atomic writes를 지원할 수 있는 이점을 얻을 수 있으며, memtable과 table file을 분리할 경우 각각의 컬럼 패밀리 옵션들을 따로 지정할 수 있고 불필요한 컬럼 패밀리들과 관련된 데이터를 빠르게 삭제할 수 있다.

각 컬럼패밀리가 flush 될 때 마다 새로운 WAL을 생성하고, 모든 컬럼패밀리들에 대한 새로운 write 정보는 새로 생성된 WAL에 저장되게 된다. 이 경우에도 기존 WAL에 다른 다중 컬럼패밀리들에 대한 write 정보가 포함되어 있으므로, 곧바로 기존 WAL을 삭제 할 수 없다. 기존 WAL이 포함하고 있는 모든 컬럼 패밀리들이 flush되고 해당 WAL이 지니고 있는 write 정보에 대한 모든 데이터들이 table file에 영구적으로 저장되어야만, 기존 WAL을 삭제할 수 있다. 이러한 특성으로 인해 컬럼 패밀리 구현 방식에 몇가지 이슈들과 몇가지 튜닝 포인트가 있다. 하나의 데이터베이스에 다중 컬럼패밀리를 사용할 경우 반드시 모든 컬럼패밀리들이 주기적으로 flush 되게 설정하여, WAL 파일이 쌓이지 않도록 하는 것이 중요하다. 또한, `Options::max_total_wal_size` 등의 옵션 설정을 통해 접근한지 오래된 컬럼 패밀리들이 빠르게 flush 될 수 있도록 해야한다. 