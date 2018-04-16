# RocksDB Time to Live

## RocksDB can be opened with Time to Live(TTL) support

### USE-CASES

TTL 관련 API는 key/value 쌍들이 데이터베이스에 입력 된 후 일정한 `ttl` 시간 동안만 데이터베이스에 존재하도록 할 때 사용한다. `ttl` 시간이 소요 되기 이전에는 데이터베이스에 항상 존재하며, `ttl` 이 지난 데이터들은 가능한 빨리 데이터베이스에서 삭제한다.

### BEHAVIOUR

- TTL은 초단위로 설정할 수 있다.
- `(int32_t) Timestamp (creation)` 의 값이 `Put` 작업시 key/value의 value 뒤에 덧붙여 진다.
- TTL 이 만료된 데이터만이 컴팩션 시에 소거된다.
- Get/Iterator 작업은 TTL 만료와 관계없이 엔트리를 반환한다. 컴팩션시 소거된 경우만 반환하지 않는다.
- 서로다른 TTL 값은, 서로 다른 Open (데이터베이스 연결) 인스턴스에 적용할 수 있다.
- 예를 들어, t를 열린 시간/ttl을 TTL 시간이라고 할때, Open1을 t=0, ttl=4 로 하고 k1/k2를 put 한뒤 close를 한다. 이후, open2를 t=3에 ttl=5로 설정하더라도, Open1에서 설정한 ttl=4 이후인 4초 이후에는 k1/k2 데이터가 삭제된다. 
- read_only 모드로 연결된 데이터베이스에서는 컴팩션이 발생하지 않으므로, TTL 데이터에 대한 소거도 진행되지 않는다.

### CONSTRAINTS

TTL 정보를 설정하지 않거나, 음수의 값으로 설정할 경우 TTL=inifinity 인 것처럼 영원히 유지된다.

### !!!WARNING!!!

- TTL open API를 사용한 데이터베이스는 항상 value 뒤에 timestamp 데이터가 들어가 있으므로,  일반 Open API를 사용할 경우 잘못 된 value값을 반환한다. 그러므로, 항상 TTL open을 사용하거나 open만 사용하거나 하는 식으로, 일관적인 open API를 사용해야 한다.
- 너무 작은 TTL 시간을 사용할 경우 데이터베이스 전체가 삭제 될 수 있으니, 원하는 경우가 아니면 적당히 큰 시간을 TTL로 입력해야 한다.

### API

`rocksdb/utilities/db_ttl.h`에 정의되어 있다.

```c++
static Status DBWithTTL::Open(const Options& options, 
                              const std::string& name, StackableDB** dbptr, 
                              int32_t ttl = 0, bool read_only = false);
```

