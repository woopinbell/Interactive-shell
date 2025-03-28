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
이후 parser는 simple command -> pipeline -> and/or -> sequence 순서로 연산자 우선순위를 반영해 상위 AST를 조립합니다.
heredoc redirection은 delimiter 문자열과 quoted delimiter 규칙에 따른 확장 허용 여부를 AST에 함께 기록합니다.
parsed word의 expansion은 lexing이 아니라 실행 전 word preparation 단계에서 수행하며 single quote part는 확장을 막습니다.
실행 전 준비 단계는 heredoc 본문도 미리 수집하고 quoted delimiter 여부에 따라 body expansion을 켜거나 끕니다.
executor는 AST를 받아 preparation을 거쳐 실행하는 공용 entrypoint를 제공하고 waitpid/system error를 shell status 규칙으로 변환합니다.
실행 파일 해석은 절대/상대 경로와 PATH 탐색을 분리해 후보를 찾고 이후 exec 단계가 그 결과를 재사용합니다.
simple external command는 fork, execve, wait 흐름으로 실행되고 child envp는 env store 직렬화를 사용합니다.
builtin registry가 추가되어 echo, pwd, env, true, false 같은 stateless builtin은 parent에서 바로 실행됩니다.

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
- pipeline/and-or/sequence precedence 파싱
- heredoc delimiter metadata 기록
- parsed word preparation expansion
- heredoc input collection and expansion policy
- executor entrypoint and shell status mapping
- executable path resolution
- simple external command execution
- builtin dispatch and stateless builtins
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
