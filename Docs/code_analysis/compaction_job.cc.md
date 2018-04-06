> RocksDB version: 5.13.fb branch of github
>
> Author: Gihwan Oh (wurikiji@gmail.com)

# compaction_job.cc

본 문서는 **RocksDB**의 `compaction_job.cc` 소스코드를 분석하고 간략하게 정리한 문서이다.

소스코드의 전체 클래스/구조체/메소드/변수를 정리하지는 않으며, 실제 컴팩션 알고리즘을 수행시 중요하게 사용되는 정보들만을 정리하였다. 

전체 소스코드는 [RocksDB github](https://github.com/facebook/rocksdb/blob/5.13.fb/db/compaction_job.cc)에서 확인할 수 있다.

`ThreadOperation` 관련 참고자료: [std::memory_order](http://en.cppreference.com/w/cpp/atomic/memory_order)



## Compaction Stage

`STAGE_COMPACTION_PREPARE` -> `STAGE_COMPACTION_RUN` -> `STAGE_COMPACTION_PROCESS_KV` -> `STAGE_COMPACTION_INSTALL` -> `STAGE_COMPACTION_SYCN_FILE`



## class CompactionJob

실제 컴팩션을 위해 사용되는 클래스. 컴팩션 작업과 관련한 모든 내용이 이 클래스에 포함되어 있음. 컴팩션 대상 파일/대상 레벨 등과 같은 정보는 `compact_->compaction`에 존재함. 

```c++
struct CompactionJob::CompactionState compact_; // 멤버 변수
	// -> CompactionState 구조체 안에 Compaction compaction; 멤버 변수가 선언 되어있음.
	// Compaction 클래스는 대상 레벨, 인풋 파일 등과 같은 컴팩션에 필요한 논리적 데이터 정보를 포함.
```

하나의 쓰레드에서 하나의 `CompactionJob` 클래스를 가지고 수행함. 서로 다른 쓰레드는 다른 클래스를 가지고 있음. 

클래스의 멤버변수는 변수명 뒤에 suffix로 `_` (underscore) 가 붙어있음. 메소드 등에서 사용되는 변수 뒤에 `_`가 붙어있을 경우 멤버 변수라고 생각하면 분석하기 편함. 



### member structs

```c++
struct CompactionJob::SubcompactionState; // 서브 컴팩션들의 정보 저장 구조체 

struct CompactionJob::CompactionState; // 전체 컴팩션 진행 상황
```



### member method

#### constructor

현재 쓰레드의 상태를 `OP_COMPACTION` 상태로 변환하고 `ReportStarteCompaction()` 메소드 호출을 통해 컴팩션 시작을 기록함. 기타 변수 초기화를 진행함. 

#### other methods

##### life cycle methods

컴팩션 진행 순서에 따라 사용되는 메소드들. 스테이지 진행 순서는 [Compaction Stage](#Compaction Stage) 참조.

db_impl_compaction_flush.cc 에서 `Prepare()`, `Run()`, `Install()` 순으로 호출 한다. `FinishCompactionOutputFile`의 경우에는 `CompactionJob` 클래스의 `ProcessKeyValueCompaction` 메소드 내부에서 호출 된다. 

```c++
// STAGE_COMPACTION_PREPARE 스테이지로 진입
// 최하위 레벨에서의 컴팩션인지 여부를 CompactionJob->bottommost_level에 저장
// 필요한 경우 subcompaction을 구성함 (GenSubcompactionBoundaries() 호출)
void CompactionJob::Prepare(){}
```

```c++
// STAGE_COMPACTION_RUN 스테이지로 진입
Status CompactionJob::Run() {
    // 일반 로그 (print 하는 로그) 기록하고 
    // subcompaction 갯수 많큼 Thread 저장할 vector 생성
    // 생성한 thread pool에 개별 sub compaction 할당해서 수행
    // compaction 수행하는 메소드는 ProcessKeyvalueCompaction();
    
    // 리소스 관리의 효율성을 위해 0번 subcompaction은 현재 쓰레드에서 수행 
    // ProcessKeyValueCompaction 에서 STAGE_COMPACTION_PROCESS_KV 스테이지로 진입
    ProcessKeyValueCompaction(&compact_ -> sub_compact_states[0]);
    
    // 전체 subcompaction 끝날때 까지 thread.join()
    // output directory에 대해서 Fsync()
    // 각종 stat 정보 모으고 종료. 
}
```

```c++
// STAGE_COMPACTION_INSTALL 스테이지로 진입
Status CompactionJob::Install(const MutableCFOoptions& mutable_cf_options) {
    // delete할 파일을 deletion 목록에 저장.
    InstallCompactionResults(mutable_cf_options);
    
    // 각종 stat 정보 계산후 로그 저장. abort 된 애들 정리 
    CleanupCompaction();
}
```

```c++
// STAGE_COMPACTION_SYNC_FILE 스테이지로 진입
// 각 subcompaction 마다 따로 호출됨
// ProcessKeyValueCompaction 메소드에서 직접 호출함
Status CompactionJob::FinishCompactionOutputFile( /* ... */ ) {
	// sync할 output 파일 갯수 획득
    // sync할 start key, end key 획득
    range_del_agg->AddToBuilder(); // 를 통해 range deletion 빌더 구성
    
    // builder는 TableBuilder 
    // TODO: 분석이 필요함. 
    sub_compact->builder->Finish(); // 를 통해 WriteBlock을 수행함
    
    sub_compact->outfile->Sync(db_options_.use_fsync); // 
    sub_compact->outfile->Close(); 
    
    // 아웃풋이 전혀 생기지 않은 파일에 대해 파일 deletion 작업을 수행함
    // TODO: 이거 필요없는듯  괜히 시스템콜 낭비, 위에서 Sync 호출되면 성능 저하.
    // SstManagerImple 에 새로 생성한 파일 등록하고
    sub_compact->builder.reset(); // 종료. 
}
```



##### compaction methods

실제 컴팩션을 수행하는 데 주요한 알고리즘을 구현하고 있는 메소드들.

```c++
void CompactionJob::genSubcomapctionBoundaries() {
    // 모든 인풋 파일들의 start, end 키 값을 모음
    // 정렬 후, 중복 되는 key 값을 제거
    // 2개 1쌍으로 묶어서 여러 쌍의 (start, end) range 를 만듬
    // 각 range를 통합하여 subcompaction에 사용할 boundary 쌍을 여러개 만듬
}
```

```c++
// STAGE_COMPACTION_PROCESS_KV 스테이지로 진입
void CompactionJob::ProcessKeyValueCompaction(SubcompactionState* sub_compact) {
    // 통계를 위한 각종 IO 측정 변수및 헬퍼 메소드들이 사용됨. 
    // cfd->ioptions()->comapction_filter 혹은
    // sub_compact->compaction->CreateCompactionFilter().get() 을 통해서 필터 획득
    // MergeHelper 를 통해 merge 수행을 준비. TODO: MergeHelper 상세 분석
    
    /* TODO: 시간을 잡고 집중해서 분석할 필요가 있음 */
    
    while (status.ok() && !cfd -> IsDropped() && c_iter->Valid()) {
        // MergeHelper로 머지를 진행하고, c_iter로 계속 순회하면서 
        // sub_compact->builder->Add()를 통해서 디비를 새로 생성 한다. 
        // output file이 가득 차있는지 확인 하고 필요한 경우 새로운 파일로 교체 
    }
    // 필요한 통계 정보 저장하고 
    
    // 마지막 파일 close 하고
    FinishCompactionOutputFile();
    //input 리셋 하고 끝
}
```
