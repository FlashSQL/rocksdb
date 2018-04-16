[원문](https://github.com/facebook/rocksdb/wiki/Compaction-Filter)

# RocksDB Compaction Filter

RocksDB는 백그라운드 작업 중에 사용자 임의로 정의된 로직으로 key/value 쌍을 지우거나 변경하는 방식을 지원한다. TTL (Time-to-Live)에 따라서 만료된 키를 제거하거나, 백그라운드에서 넓은 키 범위를 제거하는 것을 임의로 구현할 수 있는 것은, 사용자의 기호에 따른 가비지 콜렉션 (GC)를 정의하는 편리한 방법이 될 수 있다. 삭제 뿐만 아니라 존재하는 키의 update도 백그라운드에서 수행할 수 있는 좋은 방법이다. 

컴팩션 필터를 사용하기 위해서는, RocksDB를 사용하는 애플리케이션 레벨에서 `CompactionFilter` 인터페이스를 구현해야 한다. 해당 인터페이스는 `rocksdb/compaction_filter.h`에 정의되어 있으며 구현 이후에는 `ColumnFamilyOptions`에 지정해야 한다. 또는, `CompactionFilterFactory` 인터페이스를 구현하여, 서로 다른 sub compaction마다 각각의 고유한 컴팩션 필터를 사용하도록 설정할 수 있다. 컴팩션 필터 팩토리의 경우에는 `CompactionFilter::Context` 파라미터를 통해서 현재 진행되는 컴팩션이 full 컴팩션인지 manual 컴팩션인지 등과 같은 몇가지 정보들을 얻어 와서 사용할 수도 있다. 컴팩션 필터 팩토리는 이러한 `Context` 파라미터의 조건에 따라서 서로 다른 컴팩션 필터를 반환 할 수 있다. 

```c++
options.compaction_filter = new CustomCompactionFilter();
// or
options.compaction_filter_factory.reset(new CustomCompactionFilterFactor());
```

두가지의 컴팩션 필터 정의 방법은 서로 다른 thread-safety 조건을 요구한다. 하나의 컴팩션 필터만 사용할 경우, 다중의 sub-compaction들이 병렬적으로 동시에 수행되면서 하나의 컴팩션 필터 인스턴스만 사용하므로, 반드시 thread-safe를 고려하여 구현되어야 한다. 컴팩션 필터 팩토리를 사용할 경우, 각 sub-compaction이 컴팩션 필터 팩토리를 통해 컴팩션 필터 인스턴스를 새로 생성하여 사용하므로, 하나의 쓰레드당 한개의 컴팩션 필터 인스턴스가 할당되고, 이는 thread-safe를 고려하지 않고 구현해도 됨을 의미한다. 그러나 컴팩션 필터 팩토리 인스턴스 자체는 여러 sub-compaction들이 동시에 접근 할 수 있음을 명심해야 한다.

데이터 플러시 작업도 특수한 타입의 컴팩션이라고 볼 수 있지만, 이 경우에는 컴팩션 필터를 적용하지 않는다. 

컴팩션 필터와 함께 정의 할 수 있는 또 다른 두가지 API가 존재한다. 하나는 컴팩션 시에 key/value 쌍을 필터링 여부를 알려주는 `Filter/FilterMergeOperand` 이며, 다른 하나는 밸류 값을 변경하고나, key range를 drop 시킬수 있도록 하는 `FilterV2`이다. 

sub-compaction이 새로운 키 값을 발견하고 해당 밸류값이 normal 상태일 경우, 컴팩션 필터를 적용하고, 아래의 조건에 따라서 필터링을 수행한다.

- 키를 보존하기로 결정했을 경우, 아무 변화도 일어나지 않는다.
- 키를 필터링 하기로 결정했을 경우, 밸류 값을 deletion marker로 변경한다. 컴팩션 아웃풋 레벨이 최하위 레벨일 경우, deletion marker를 굳이 사용할 필요는 없다.
- 밸류 값을 변경하기로 결정했을 경우, 새로운 밸류 값으로 변경 된다.
- `kRemoveAndSkipUntil` 플래그를 반환하여 key range를 drop하기로 결정했을 경우, `skip_until`이 지정하는 키 이전까지의 모든 키/밸류 쌍을 스킵한다. 이 경우에 스킵하는 대상 키들에 대해서 따로 deletion marker를 표현하지 않는데, 키에 대한 deletion marker가 없으므로 스킵하는 키 버전 보다 더 오래된 버전의 키가 컴팩션 아웃풋에 포함 될 수 있다. RocksDB를 사용하는 응용프로그램이 스킵할 키의 오래된 버전이 존재 하지 않거나, 오래된 버전의 키가 아웃풋에 포함되어도 괜찮다면, 곧바로 key를 drop하는 것이 데이터베이스 관리에 더 효과적이다.

컴팩션 인풋중에서 같은 키값에 대해 다중 버전이 존재한다면, 컴팩션 필터는 가장 최신 버전의 키에 대해서만 적용된다. 최신 버전이 deletion marker를 가지고 있다면, 컴팩션 필터는 해당 키에 대해서 어떠한 작업도 수행하지 않는다. 삭제 되었지만 deletion marker가 없는 경우에는 컴팩션 필터가 적용된다. 

merge가 필요한 경우에는 merge 작업에 포함되어 있는 각각의 merge operand를 수행하기 전에 컴팩션 필터를 매번 적용한 뒤, merge operand를 적용한다.

현재 컴팩션 인풋 key/value 에 대한 시점보다 더 최근의 스냅샷이 존재 할 경우 컴팩션 필터를 적용하지 않는다. 