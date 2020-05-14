
declare -A exitcode
declare -A skips
declare -A skipped
declare -A testcases

success_count=0
failure_count=0
skip_count=0

SKIP_FILE=${SKIP_FILE-tests_to_skip.txt}

if [ -f "$SKIP_FILE" ]; then
    IFS=$'\n'
    while read -r skip_opt; do
        skips[$skip_opt]="$skip_opt"
    done < <(cat "$SKIP_FILE" | sed -e "s/^[ ]*#.*$//g" | sed -e "s/\w[ \t]{1,2}\w//g" | sed -e "s/[ \t]*//g" | grep -v '^[ \t]*$')
fi

PREFIX=${PREFIX-%}

for file in `ls examples/test/test-*.c`; do
    p4file="${file##examples/test/test-}"
    p4file="${p4file%%.c}"

    found=0
    for p4base in "${file##examples/test/test-}" "${file##examples/test/}"; do
        p4file="${p4base%%.c}"
        for ext in ".p4" ".p4_14"; do
            for srcdir in "." "test" "psa"; do
                [ -f "examples/$srcdir/$p4file$ext" ] && found=1 && break 3
            done
        done
    done

    [ $found -eq 0 ] && echo "P4 file for $file not found" && continue

    for testcase in `cat $file | grep "&t4p4s_testcase_" | sed -e "s/^.*&t4p4s_testcase_//g" | sed -e "s/ .*//g"`; do
        PREPART="$PREFIX"
        if [ -z ${POSTFIX+x} ]; then
            TESTPART="${p4file}=${testcase}"
            [ "$PREFIX" == "" -a "$testcase" == "test" ] && TESTPART="${p4file}" && PREPART="%"
        else
            TESTPART="${p4file}$POSTFIX"
        fi

        testcases[$PREPART$TESTPART]="$PREPART$TESTPART"

        [ "${skipped[$PREPART$TESTPART]}" != "" ] && continue
        [ "$testcase" == "test" ] && [ "${skipped[\"$PREPART$TESTPART=test\"]}" != "" ] && continue

        for skip in ${skips[@]}; do
            [ "$skip" == "$TESTPART" ]         && skipped[$PREPART$TESTPART]="$skip" && ((++skip_count)) && continue 2
            [ "$PREPART$skip" == "$TESTPART" ] && skipped[$PREPART$TESTPART]="$skip" && ((++skip_count)) && continue 2
            [ "$skip" == "$PREPART$TESTPART" ] && skipped[$PREPART$TESTPART]="$skip" && ((++skip_count)) && continue 2
            [ "$testcase" == "test" ] && [ "$skip=test" == "$TESTPART" ]         && skipped[$PREPART$TESTPART]="$skip" && ((++skip_count)) && continue 2
            [ "$testcase" == "test" ] && [ "$PREPART$skip=test" == "$TESTPART" ] && skipped[$PREPART$TESTPART]="$skip" && ((++skip_count)) && continue 2
        done

        echo Running test: ./t4p4s.sh $PREPART$TESTPART $*
        ON_REDO_FAIL=continue ./t4p4s.sh redo=$PREPART$TESTPART $PREPART$TESTPART $*
        exitcode["$PREPART$TESTPART"]="$?"
        [ ${exitcode["$PREPART$TESTPART"]} -eq 0 ] && ((++success_count))
        [ ${exitcode["$PREPART$TESTPART"]} -ne 0 ] && ((++failure_count))

        # if there is an interrupt, finish executing test cases
        [ ${exitcode["$PREPART$TESTPART"]} -eq 254 ] && break 2
    done
done


resultcode=0
[ ${success_count} -gt 0 ] && echo "Successful (${success_count}):"

for test in ${!exitcode[@]}; do
    [ ${exitcode[$test]} -ne 0 ] && continue
    echo "    $test"
done | sort


declare -A fail_codes
fail_codes[1]="P4 to C compilation failed"
fail_codes[2]="C compilation failed"
fail_codes[3]="Execution finished with wrong output"
fail_codes[139]="C code execution: Segmentation fault"
fail_codes[254]="Execution interrupted"
fail_codes[255]="Switch execution error"

for fc in ${!fail_codes[@]}; do
    failcode_count=0
    for test in ${!exitcode[@]}; do
        [ ${exitcode[$test]} -eq $fc ] && ((++failcode_count))
    done

    [ ${failcode_count} -eq 0 ] && continue

    echo "Failed with code $fc ($failcode_count): ${fail_codes[$fc]}"

    for test in ${!exitcode[@]}; do
        [ ${exitcode[$test]} -ne $fc ] && continue

        echo "    $test"
        resultcode=1
    done | sort
done

[ ${skip_count} -gt 0 ] && echo "Skipped ($skip_count):"

for test in ${skipped[@]}; do
    echo "    $test"
done | sort

exit $resultcode
