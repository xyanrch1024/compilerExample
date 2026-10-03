#!/usr/bin/env bash
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TINYC="${1:-$ROOT/build/tinyc}"
LLC="${LLC:-llc}"
CXX="${CXX:-c++}"

if [[ ! -x "$TINYC" ]]; then
    echo "tinyc not found at $TINYC" >&2
    exit 1
fi

fail=0
pass=0

ok() {
    echo "ok  $1"
    pass=$((pass + 1))
}

bad() {
    echo "FAIL $1"
    fail=$((fail + 1))
}

compile_and_run() {
    local src="$1"
    local expect_out="$2"
    local expect_exit="$3"
    local allow_warning="${4:-}"
    local name
    name="$(basename "$src")"
    local dir
    dir="$(mktemp -d)"

    if ! "$TINYC" "$src" >"$dir/out.ll" 2>"$dir/err.txt"; then
        bad "$name (compiler exit $?)"
        sed 's/^/    /' "$dir/err.txt"
        rm -rf "$dir"
        return
    fi
    if [[ -n "$allow_warning" ]]; then
        if ! grep -q "$allow_warning" "$dir/err.txt"; then
            bad "$name (missing warning)"
            sed 's/^/    /' "$dir/err.txt"
            rm -rf "$dir"
            return
        fi
    elif [[ -s "$dir/err.txt" ]]; then
        bad "$name (unexpected diagnostics)"
        sed 's/^/    /' "$dir/err.txt"
        rm -rf "$dir"
        return
    fi
    if ! "$LLC" -relocation-model=pic -o "$dir/out.s" "$dir/out.ll" 2>"$dir/llc.txt"; then
        bad "$name (llc)"
        sed 's/^/    /' "$dir/llc.txt"
        rm -rf "$dir"
        return
    fi
    if ! "$CXX" "$dir/out.s" -o "$dir/prog" 2>"$dir/link.txt"; then
        bad "$name (link)"
        sed 's/^/    /' "$dir/link.txt"
        rm -rf "$dir"
        return
    fi
    "$dir/prog" >"$dir/run.out"
    local got_exit=$?
    local got_out
    got_out="$(cat "$dir/run.out")"
    if [[ "$got_out" != "$expect_out" || "$got_exit" != "$expect_exit" ]]; then
        bad "$name (run out='$got_out' exit=$got_exit, expected out='$expect_out' exit=$expect_exit)"
        rm -rf "$dir"
        return
    fi
    ok "$name"
    rm -rf "$dir"
}

expect_error() {
    local src="$1"
    shift
    local name
    name="$(basename "$src")"
    local dir
    dir="$(mktemp -d)"
    local status=0
    "$TINYC" "$src" >"$dir/out.ll" 2>"$dir/err.txt" || status=$?
    if [[ "$status" -eq 0 ]]; then
        bad "$name (compiler succeeded)"
        rm -rf "$dir"
        return
    fi
    if [[ -s "$dir/out.ll" ]]; then
        bad "$name (emitted IR after errors)"
        rm -rf "$dir"
        return
    fi
    local needle
    for needle in "$@"; do
        if ! grep -qF "$needle" "$dir/err.txt"; then
            bad "$name (missing '$needle')"
            sed 's/^/    /' "$dir/err.txt"
            rm -rf "$dir"
            return
        fi
    done
    ok "$name"
    rm -rf "$dir"
}

compile_and_run "$ROOT/test.c" $'50' 0
compile_and_run "$ROOT/test2.c" $'5\n0\n1\n2' 42
compile_and_run "$ROOT/tests/ok_prec.c" $'1\n7' 0
compile_and_run "$ROOT/tests/ok_scope.c" $'2\n1' 0
compile_and_run "$ROOT/tests/ok_branch_assign.c" $'4' 0
compile_and_run "$ROOT/tests/ok_return_in_if.c" "" 7
compile_and_run "$ROOT/tests/ok_both_return.c" "" 3
compile_and_run "$ROOT/tests/ok_return_while.c" "" 9
compile_and_run "$ROOT/tests/ok_while_local.c" $'0\n1\n2' 0
compile_and_run "$ROOT/tests/warn_no_return.c" $'1' 0 "control reaches end of function"
compile_and_run "$ROOT/tests/ok_call.c" $'6' 0
compile_and_run "$ROOT/tests/ok_call_order.c" $'4' 0
compile_and_run "$ROOT/tests/ok_call_stmt.c" $'2' 0
compile_and_run "$ROOT/tests/ok_func_shadow.c" $'1\n8' 0

expect_error "$ROOT/tests/err_redef.c" \
    "err_redef.c:3: error: redefinition of 'a'" \
    "err_redef.c:2: note: previous definition is here"
expect_error "$ROOT/tests/err_undecl.c" \
    "err_undecl.c:2: error: assignment to undeclared identifier 'a'"
expect_error "$ROOT/tests/err_uninit.c" \
    "err_uninit.c:3: error: 'a' is used uninitialized"
expect_error "$ROOT/tests/err_cond.c" \
    "err_cond.c:4: error: condition of if must be bool, got int"
expect_error "$ROOT/tests/err_assign_bool.c" \
    "err_assign_bool.c:6: error: cannot assign bool to int"
expect_error "$ROOT/tests/err_print_bool.c" \
    "err_print_bool.c:6: error: print expects int, got bool"
expect_error "$ROOT/tests/err_unreachable.c" \
    "err_unreachable.c:3: error: unreachable code"
expect_error "$ROOT/tests/err_while_assign.c" \
    "err_while_assign.c:9: error: 'x' is used uninitialized"
expect_error "$ROOT/tests/err_scope.c" \
    "err_scope.c:6: error: use of undeclared identifier 'a'"
expect_error "$ROOT/tests/err_chain.c" \
    "err_chain.c:8: error: operands of '<' must be int"
expect_error "$ROOT/tests/err_partial_assign.c" \
    "err_partial_assign.c:8: error: 'a' is used uninitialized"
expect_error "$ROOT/tests/err_multi.c" \
    "err_multi.c:2: error: use of undeclared identifier 'a'" \
    "err_multi.c:4: error: redefinition of 'b'" \
    "err_multi.c:3: note: previous definition is here" \
    "err_multi.c:5: error: return value must be int, got bool"
expect_error "$ROOT/tests/err_syntax.c" \
    "err_syntax.c:" \
    ": error:"
expect_error "$ROOT/tests/err_redef_fn.c" \
    "err_redef_fn.c:4: error: redefinition of function 'foo'" \
    "err_redef_fn.c:1: note: previous definition is here"
expect_error "$ROOT/tests/err_no_main.c" \
    "err_no_main.c:1: error: program must define 'main'"
expect_error "$ROOT/tests/err_undef_fn.c" \
    "err_undef_fn.c:2: error: call to undeclared function 'bar'" \
    "err_undef_fn.c:3: error: call to undeclared function 'baz'"
expect_error "$ROOT/tests/err_printf_name.c" \
    "err_printf_name.c:1: error: function name 'printf' is reserved"

echo
echo "$pass passed, $fail failed"
if [[ "$fail" -ne 0 ]]; then
    exit 1
fi
