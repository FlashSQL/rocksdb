# Almost Sequential Log Data (ASL) 컴팩션 최적화 방안

본 워크로드에서의 상황은 ever increasing key 값을 가지고 있음을 가정한다.



## General 상황인 경우

L0에 대한 compaction 이거나, 중간 중간에 sequential 하지 않은 데이터가 있다고 가정하는 상황에서의 최적화 방안. 

- Merge 테스트를 위해서 데이터 read는 막을 수 없다. 
- read된 데이터들에 대해 merge 테스트를 수행하고 merge 되지 않는 데이터들은 체크한다.
- 체크된 데이터들에 대하여 `move` 사용 가능 여부를 확인한다.
- *block* 단위 IO에 대하여 `move`를 사용해서 write 횟수를 줄인다.


## Fully Sequential Log Data (FSL) 상황인 경우

Compaction 할 때 merge 비교 작업을 할 필요 자체가 없다. 모든 경우에 대하여 range만 확인한 뒤 move를 통해 진행 할 수 있다.

- block 단위로 range에 맞추어 move를 진행
- FSL 이므로 겹치는 구간도 없음 -> merge가 전혀 필요없는 상황




## 적용가능한 현재 구현 내용

### FIFO 컴팩션

ASL 데이터의 경우 TTL을 통해 소거하게 되므로, FIFO 방식으로 컴팩션을 수행하게 되면, 모든 데이터가 소거될 확률이 크므로 WAF와 SA를 모두 줄일 수 있음. 

단, TTL 이전에 발생하는 컴팩션에 대해서는 위의 최적화 기법들을 구현해야 함. 