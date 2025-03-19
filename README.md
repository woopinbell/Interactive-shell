# Interactive Shell

Interactive Shell을 설계하고 구현하는 C 프로젝트입니다.

## 기본 빌드 뼈대

```sh
make        # 현재 구현된 셸 바이너리까지 빌드합니다.
make objects # 현재 누적된 공통/코어 오브젝트만 빌드합니다.
make clean  # 오브젝트 파일을 정리합니다.
make fclean # build/ 디렉터리를 전체 정리합니다.
make re     # 빌드 디렉터리를 다시 준비합니다.
make run    # 최소 REPL을 실행합니다.
```

현재는 `main`에서 셸 컨텍스트와 입력 어댑터를 묶는 최소 REPL 단계입니다.
interactive 모드에서는 `readline`으로 프롬프트 입력을 받고, non-interactive 모드에서는 stdin 스트림을 같은 인터페이스로 읽으며 라인 단위 루프를 돕니다.
프롬프트 대기 중 `SIGINT`는 현재 입력을 취소하고 새 프롬프트로 돌아가며 `SIGQUIT`은 무시합니다.
파싱 단계로 넘어가기 전에 lexer가 공통으로 사용할 토큰 kind와 quote 상태 타입을 먼저 고정합니다.
현재 lexer는 공백 분리, `&&`, `||`, `;`, `|`, `<`, `>`, `<<`, `>>` 연산자 토큰화와 일반 단어 수집까지 지원합니다.
quoted/plain 인접 조각은 하나의 logical word로 조립되고 각 part는 plain, single, double 상태를 따로 보존합니다.
`$VAR`, `$?`, parameter 식별자 스캔과 lookup helper도 분리해 다음 확장 단계의 기반을 준비합니다.
simple command, argv word, redirection, heredoc placeholder를 담는 AST 타입도 parse 계층에 먼저 고정합니다.
simple command 위에는 pipeline, and/or list, sequence list 계층을 쌓아 `&&`, `||`, `;`를 담는 상위 AST 구조를 준비합니다.
parser는 먼저 word/redirection을 simple command로 모으고 `>`, `|`, `&&` 뒤 누락 같은 기본 syntax error를 감지합니다.

## 프로젝트 목표

- 기본 빌드 환경 구성
- 최소 유틸리티와 자려구조 준비
- 입력 루프와 상태 관리 추가
- interactive / non-interactive 입력 어댑터 연결
- 토큰 종류와 quote 상태 타입 정의
- 셸 연산자와 일반 단어 lexer 구현
- quoted 인접 word part 조립
- parameter expansion helper 분리
- command/redirection AST 타입 정의
- pipeline/and-or/sequence AST 타입 정의
- simple command 파싱과 기본 syntax error 처리
- 토큰화, 파싱, 실행 흐름 구현

## 초기 디렉터리 구조

```text
include/shell/support/ 공통 메모리/버퍼/vector/검증 helper 헤더
include/shell/         나머지 공개 헤더
src/support/       메모리 할당, 버퍼, vector, 검증 같은 공통 유틸리티
src/core/          REPL, 셸 컨텍스트, 환경 저장소
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
