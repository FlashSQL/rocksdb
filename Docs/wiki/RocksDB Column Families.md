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

