# ProcMon
Windows의 Driver Filter에서 Process, File, Registry의 Access 이벤트를 사용자에게 보여주는 프로그램

![sc1](https://user-images.githubusercontent.com/17915949/218378648-e6e8c793-555b-4bc9-8461-413923584283.png)

## 기능
1. 실행중인 process들이 접근중인 process, file, registry 항목들을 driver filter에서 실시간으로 가져와 사용자에게 보여줌
2. 해당 기능을 수행한 process의 PID, 발생한 시간, 수행한 작업(terminate, create, read 등등), 수행한 대상(process PID, 파일명, 레지스트리)을 표시
3. PID, 작업, 대상으로 필터링하여 검색 가능

## 설치

1. driver 인증서가 테스트 인증서이므로 Windows를 테스트 모드에 진입해야 함
2. CMD를 관리자 권한으로 실행
3. bcdedit.exe -set loadoptions ENABLE_INTEGRITY_CHECKS
4. bcdedit.exe -set TESTSIGNING OFF
5. 재부팅
6. ProcMon.exe를 관리자 권한으로 실행
7. 테스트 모드를 해제하려면 2. 3. 재실행 후 bcdedit.exe -set TESTSIGNING ON
