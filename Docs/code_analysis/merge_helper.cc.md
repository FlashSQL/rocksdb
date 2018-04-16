> RocksDB version: 5.13.fb branch
>
> Author: Gihwan Oh (wurikiji@gmail.com)

# merge_helper.cc

본 문서는 `merge_helper.cc` 코드에 작성되어있는 `MergeHelper` 클래스 및  그 하위 멤버 변수/멤버 메소드 들을 분석한 문서이다. 

본 문서의 분석은 [RocksDB 최적화](https://github.com/flashsql/rocksdb) 연구와 관련하여, 필요한 부분만을 선택해서 분석한 내용을 담고 있다. 최대한 간략하게 작성하려고 하였으며, 자세한 분석내용은 1차 작성 이후 연구 진행 상황에 따라서 리뷰 프로세스시에 추가된다.

본 문서에서 다루는 `MergeHelper` 및 `MergeOperator`는 흔히 말하는 compaction에서의 merge가 아닌 RocksDB에서 여러 operation 수행에 따른 데이터 변환 작업을 의미하는, operation merge 작업이다. 



## class MergeHelper

### Constructor

user-defined comparator, user-defined merge operator, compaction filter, lates snapshot sequence number, snapshot checker, compaction level 등의 정보를 받아와서 초기화를 진행한다. 



### Member variables





### Meber methods

```c++
Status MergeHelper::TimedFullMerge(/* MergeOperator*, &key, *value, operands, 기타 등등 */) {
    // 머지 하면서 시간 측정 하는 함수. 
    MergeOperator::MergeOperationInput merge_in(/* 키 밸류 operands */); // 초기화
    MergeOperator::MergeOperationOutput merge_out(/* 결과 저장소, 임시 결과 저장소 */); // 초기화
    
    merge_operator->FullMergeV2(merge_in, &merge_out);
    // 종료 하기. 
}
```

```c++
Status MergeHelper::MergeUntil(/* InternalIterator*, RangeDelAggregator*, SequenceNumber stop,
								at bottom */) 
{
    for (/* iterator valid 인 동안 */) {
        if (/* first_key */) {
            // pass
            first_key = false;
        } else if (/* different key */) {
            // 다른 키 찾았다 stop
            hit_the_next_user_key = true;
            break;
        } else if (/* 같은키 && stop 보다 낮은 lsn && stop과 같은 snapshot */) {
            break;
        }
        if (/* kTypeMerge가 아니다 */) {
            // put/delete/single-delete 중에 하나로 생성된 놈들이다.
        }
    }
}
```

```c++
CompactionFilter::Decision MergeHelper::FilterMerge(/* key, value */) {
    compaction_filter_ -> FilterV2(/* 레벨, 키, KMergeOperand, 밸류, 필터 밸류, skip until */);
}
```

