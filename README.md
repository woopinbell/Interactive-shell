# Interactive Shell

Interactive Shell을 설계하고 구현하는 C 프로젝트입니다.

## 기본 빌드 뼈대

```sh
make        # build/ 디렉터리를 준비하고, 소스가 있으면 바이너리를 빌드합니다.
make clean  # 오브젝트 파일을 정리합니다.
make fclean # build/ 디렉터리를 전체 정리합니다.
make re     # 빌드 디렉터리를 다시 준비합니다.
```

현재는 저장소 구조만 먼저 잡아 둔 상태라서 `src/` 아래에 실제 `.c` 파일이 생기기 전까지는 `make`가 디렉터리만 준비하고 안내 메시지를 출력합니다.

## 프로젝트 목표

- 기본 빌드 환경 구성
- 최소 유틸리티와 자려구조 준비
- 입력 루프와 상태 관리 추가
- 토큰화, 파싱, 실행 흐름 구현

## 초기 디렉터리 구조

```text
include/shell/     공개 헤더
src/core/          REPL과 셸 컨텍스트
src/parse/         토큰화와 파싱
src/exec/          실행기와 리디렉션
src/builtin/       내장 명령어
tests/unit/        단위 테스트
tests/integration/ 통합 테스트
build/             Make가 생성하는 빌드 산출물
```

## 예정 범위

- REPL 기반 명령 입력
- 환경 변수와 종료 코드 추적
- 다중 파이프와 리디렉션 지원
- heredoc
- 대표적인 내장 명령어 구현
- &&, ||, ; 연산자 지원
