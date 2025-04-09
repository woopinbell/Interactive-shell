#!/bin/sh

set -u

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
SHELL_BIN=${SHELL_BIN:-"$ROOT_DIR/build/bin/shell"}
TMP_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/shell-smoke.XXXXXX") || exit 1
FAILURES=0
CASE_INDEX=0

trap 'rm -rf "$TMP_ROOT"' EXIT INT TERM

export SMOKE_VALUE="codex-smoke"

run_shell()
{
	input=$1
	stdout_file=$2
	stderr_file=$3

	printf "%b" "$input" | "$SHELL_BIN" >"$stdout_file" 2>"$stderr_file"
	return $?
}

report_pass()
{
	printf "ok - %s\n" "$1"
}

report_fail()
{
	printf "not ok - %s\n" "$1" >&2
	FAILURES=$((FAILURES + 1))
}

show_diff()
{
	expected_file=$1
	actual_file=$2

	if command -v diff >/dev/null 2>&1; then
		diff -u "$expected_file" "$actual_file" >&2 || true
	else
		printf "expected:\n" >&2
		cat "$expected_file" >&2
		printf "actual:\n" >&2
		cat "$actual_file" >&2
	fi
}

expect_case()
{
	name=$1
	input=$2
	expected_stdout=$3
	expected_stderr=$4
	expected_status=$5
	case_dir="$TMP_ROOT/case-$CASE_INDEX"

	mkdir -p "$case_dir"
	stdout_file="$case_dir/stdout"
	stderr_file="$case_dir/stderr"
	expected_stdout_file="$case_dir/expected.stdout"
	expected_stderr_file="$case_dir/expected.stderr"
	CASE_INDEX=$((CASE_INDEX + 1))
	run_shell "$input" "$stdout_file" "$stderr_file"
	status=$?
	printf "%b" "$expected_stdout" >"$expected_stdout_file"
	printf "%b" "$expected_stderr" >"$expected_stderr_file"
	if [ "$status" -ne "$expected_status" ]; then
		report_fail "$name (exit status)"
		printf "expected exit status %s, got %s\n" \
			"$expected_status" "$status" >&2
		return
	fi
	if ! cmp -s "$expected_stdout_file" "$stdout_file"; then
		report_fail "$name (stdout)"
		show_diff "$expected_stdout_file" "$stdout_file"
		return
	fi
	if ! cmp -s "$expected_stderr_file" "$stderr_file"; then
		report_fail "$name (stderr)"
		show_diff "$expected_stderr_file" "$stderr_file"
		return
	fi
	report_pass "$name"
}

expect_stderr_contains()
{
	name=$1
	input=$2
	expected_status=$3
	pattern=$4
	case_dir="$TMP_ROOT/case-$CASE_INDEX"

	mkdir -p "$case_dir"
	stdout_file="$case_dir/stdout"
	stderr_file="$case_dir/stderr"
	CASE_INDEX=$((CASE_INDEX + 1))
	run_shell "$input" "$stdout_file" "$stderr_file"
	status=$?
	if [ "$status" -ne "$expected_status" ]; then
		report_fail "$name (exit status)"
		printf "expected exit status %s, got %s\n" \
			"$expected_status" "$status" >&2
		return
	fi
	if [ -s "$stdout_file" ]; then
		report_fail "$name (stdout)"
		show_diff /dev/null "$stdout_file"
		return
	fi
	if ! grep -F "$pattern" "$stderr_file" >/dev/null 2>&1; then
		report_fail "$name (stderr pattern)"
		printf "expected stderr to contain: %s\n" "$pattern" >&2
		cat "$stderr_file" >&2
		return
	fi
	report_pass "$name"
}

expect_case "echo builtin" \
	"echo hello\n" \
	"hello\n" \
	"" \
	0

expect_case "single quote blocks expansion" \
	"echo '\$SMOKE_VALUE'\n" \
	"\$SMOKE_VALUE\n" \
	"" \
	0

expect_case "pipeline execution" \
	"echo hello | tr h H\n" \
	"Hello\n" \
	"" \
	0

expect_case "and or short circuit" \
	"false && echo no || echo yes\n" \
	"yes\n" \
	"" \
	0

expect_case "semicolon sequence" \
	"echo one; echo two\n" \
	"one\ntwo\n" \
	"" \
	0

redirection_target="$TMP_ROOT/redirection.txt"
expect_case "redirection apply" \
	"echo hi > $redirection_target\ncat < $redirection_target\n" \
	"hi\n" \
	"" \
	0

expect_case "heredoc expansion" \
	"cat <<EOF\nhello \$SMOKE_VALUE\nEOF\n" \
	"hello $SMOKE_VALUE\n" \
	"" \
	0

expect_case "parent state builtin" \
	"cd /\npwd\n" \
	"/\n" \
	"" \
	0

expect_case "export updates env store" \
	"export SMOKE_EXPORT=bar\nenv | grep '^SMOKE_EXPORT='\n" \
	"SMOKE_EXPORT=bar\n" \
	"" \
	0

expect_case "blank line keeps last status" \
	"false\n\n" \
	"" \
	"" \
	1

expect_case "exit terminates repl" \
	"exit 7\necho no\n" \
	"" \
	"" \
	7

expect_stderr_contains "syntax error status" \
	"|\n" \
	2 \
	"syntax error near unexpected token"

if [ "$FAILURES" -ne 0 ]; then
	printf "smoke tests failed: %s case(s)\n" "$FAILURES" >&2
	exit 1
fi

printf "smoke tests passed: %s case(s)\n" "$CASE_INDEX"
