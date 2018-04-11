> RocksDB version: 5.13.fb branch 
>
> Author: Gihwan Oh (wurikiji@gmail.com)

# compaction_iterator.cc

본 문서는 **RocksDB**의 `compaction_iterator.cc` 소스코드를 분석하고 간략하게 정리한 문서이다. 

소스코드의 전체 클래스/구조체/메소드/변수를 정리하지는 않으며, 실제 컴팩션 알고리즘을 수행시 중요하게 사용되는 정보들만을 정리하였다. 

전체 소스코드는 [RocksDB github](https://github.com/facebook/rocksdb/blob/5.13.fb/db/compaction_iterator.cc)에서 확인할 수 있다.



## class CompactionIterator

`merge`가 끝난 키들이 `MergeHelper`로부터 out 되서 나오면 iterator의 input으로 설정된다. 이 input이 에 대해서 무결성 체크나 deletion 체크가 끝나서 유효한 애들이 output으로 나간다. 

`SequenceNubmer`는 트랜잭션의 seq nubmer로 wal에 저장되는 순서를 의미한다. `snapshot`은 현재 사용중인 스냅샷들 

### member variables

변수명 뒤에 suffix로 언더 스코어 (`_`)가 붙어 있을 경우 멤버 변수이다. 여러 메소드 등에서 접근하는 변수 뒤에 언더스코어 여부를 통해, 분석 시 멤버 변수 여부를 쉽게 파악할 수 있다.

### member structs/class

`CompactionProxy` 는 `Compaction` 클래스 대신에, 이터레이션에 필요한 일부 기능만을 구현하고 `Compaction` 클래스의 wrapper interface로 사용하기 위한 클래스이다.

### member methods

```c++
void CompactionIterator::Next() {
    // merge 된 데이터가 있는지 확인
    // merge 된 데이터가 있고, merge output에 아직 소진 하지 않은 데이터가 있을 경우
    // 해당 아웃풋을 사용
    // merge 된 데이터를 모두 소진 했을 경우 NextFromInput()으로 다음 merge 수행
    
    // merge된 데이터 자체가 없을 경우 (merge_out_iter_가 없음)
    // 다음 compaction input으로 변경하고 NextFromInput()을 통해 merge 수행
    
    PrepareOutput(); // 호출을 통해 이터레이션 마무리
}
```

```c++
void CompactionIterator::InvokeFilterIfNeeded() {
    // compaction filter를 옵션으로 생성하고 snapshot number가 최신일때
    // CompactionFilter::FilterV2() 호출로 체크함. 
}
```

```c++
void CompactionIterator::NextFromInput() {
    // input 파일이 아직 존재하고, 셧다운 중이 아닌경우에 수행. 
    // valid_는 현재 진행 중인 NextFromInput() 작업이 유효한 작업인지 아닌지 판별하는 변수
    while(valid_ ) { // 다른 조건은 코드 참조 
        if (!ParseInternelKey()) {
            // corrupt 된 데이터 && not expected => valid_=false; assert; Corruption; break;
            // corrupt 된 데이터 && expected => valid_ = true; break; => caller보고 처리하게 함
        }
        // 현재 이터레이션 대상 키 설정하고, 커밋된 키의 경우 컴팩션 필터 적용
        InvokeFilterIfNeeded();
        if (clear_and_output_next_key_/* 이전 iteration에서 single deletion 했으면 */) {
            // value_.clear()해서 메모리 재활용하고, key 는 놔둠
        } else if ( /* single deletion 해야 하는 key이면 */ ) {
            // 1. put 이 들어왔거나 / 더이상 존재하지 않는 key일때 => deletion 가능 조건
            // 2. snapshot을 이미 보내줬거나, 앞으로 snapshot이 필요없을때 => 트랜잭션 충돌 방지
            
            // 바로 다음 키로 넘겨버리면 컴팩션 ouput이 되지 않으므로, deletion과 동일
            input_ -> Next(); 
            if (/* 다음 키 값도 현재 키 값이랑 같을 때 */) {
                if ( /* 
                		나 == 사용자가 볼 수 잇는 최상위 스냅샷  
                		|| 다음 키 스냅샷 > 사용자가 요청 가능한 가장 오래된 스냅샷 
                		|| 다음키 스냅샷이 사용자가 요청 가능한 가장 오래된 스냅샷
                        */ ) {
                    // 스냅샷 값이 이전 키 다음 스냅샷인가? -> 필요할 수도 있다.
                    if ( /* 다음 키도 deletion  대상이면 */ ) {
                        // 같은 파일/같은 키 연속 delete -> put 이 이상하게 발생했었다.
                        // 다음 iteration에서 처리 하도록 한다. 
                    } else if ( /* 이미 return 했거나 || write conflict 보다 오래된 애거나 ||
                    				write conflict 스냅샷과 같은 스냅샷이거나*/ ) {
                        // 이미 return 했거나, mismatch 상황이므로 패스 해도 된다.
                        input -> Next();
                    } else { 
                        // write conflict 이면
                        valid_ = true; // 현재 값을 output으로 사용
                        clear_and_output_next_key_ = true; // 사용 한뒤 value 날려라 
                    }
                } else {
                    valid_ = true;
                }
            } else {
                // input 마지막이거나 다른 키값이거나 
                // n+1 이상 레벨에도 같은 키가 존재할 경우 현재 키값 single delete 로 리턴
                // 아닐 경우 현재 키 drop함
            }            
        } else if ( /* 이전 키의 earlist 스냅샷 seq num == 지금 earlist 스냅샷 seq num */ ) {
            // 스킵 한다고 함. 왠지 모르겠음? 
            // 같은 키 일경우 이전 seq num > 지금 seq num이면 지금 key를 나중에 또 가져갈 일이 없음. 
            // 다른 키이면 ? 이 코드 들어와도 되는건가?
        } else if ( /* deletion 타입 && 지금 seq num <= 전체 스냅샷 earliest seq num
        			&& 전체 earliest 스냅샷이랑 같은 스냅샷에 존재 
        			&& incremental 스냅샷 필요 없고 
                    && n+1 이상 레벨에도 없을 거임*/) {
            // 이미 스냅샷으로 리턴 되었거나
            // 그게 아니면 하위 레벨에 다른 스냅샷이 없는게 확실 하므로 
            // 그냥 지워도 된다.
            input_ -> Next();
        } else if ( /* merge type 이면 */ ) {
         	merge_helper_ -> MergeUntil(); // 머지 진행함. 
            merge_out_iter_.SeekToFirst(); // 머지 끝난거 제일 앞으로 ㄱㄱ
            if (/* 머지 오류 */) {
                // 오류 리턴
            } else if ( /* 머지 정상 종료 && valid iterator */ ) {
                // merge iterator에서 key 가져와서
                // 현재 ikey_에 집어 넣고
                valid_ = true;
            } else {
                // 머지는 끝났는데, iterator가 없음
                // -> 즉 전부다 filter 에 걸려서 날라 갔다. 
                has_current_user_key_ = false;
                // 더 skip 할 구간 있는지 확인. 
            }
        } else {
            // new key 이거나
            // different snapshot strip 이거나
            // 지워도 되는 데이터 인지 확인. 하고 지워도 되면 input_->Next();
            // 지우면 안되면 valid_ = true;
        }
    }
}
```

```c++
void CompactionIterator::PrepareOutput() {
    // 필요한 경우 압축률을 높이기 위해서 sequence number를 0으로 초기화 시킴. 
}
```

