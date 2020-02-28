
declare -A exitcode
declare -A skips
declare -A testcases

success_count=0
failure_count=0
skip_count=0

SKIP_FILE=${SKIP_FILE-tests_to_skip.txt}

if [ -f "$SKIP_FILE" ]; then
    IFS=$'\n'
    while read -r skip_opt; do
        skips[$skip_opt]="$skip_opt"
    done < <(cat "$SKIP_FILE" | sed -e "s/^[ ]*-[ ]*//g" | sed -e 's/[ ]*-->.*$//g' | grep -v '^[ \t]*$' | grep -v '^[ \t]*#')
fi

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
        tested="%${p4file}=${testcase}"
        [ "$testcase" == "test" ] && tested="%${p4file}"
        testcases[$tested]="$tested"

        for skip in ${skips[@]}; do
            [ "$skip" == "$tested" ] && ((++skip_count)) && continue 2
        done

        echo Running test: ./t4p4s.sh $tested $*
        ./t4p4s.sh $tested $*
        exitcode["$tested"]="$?"
        [ ${exitcode["$tested"]} -eq 0 ] && ((++success_count))
        [ ${exitcode["$tested"]} -ne 0 ] && ((++failure_count))
    done
done


resultcode=0
[ ${success_count} -gt 0 ] && echo "Successful ($success_count):"

for test in ${!exitcode[@]}; do
    [ ${exitcode[$test]} -ne 0 ] && continue
    echo "    - $test"
done | sort


declare -A fail_codes
fail_codes[1]="P4 to C compilation failed"
fail_codes[2]="C compilation failed"
fail_codes[3]="Execution finished with wrong output"
fail_codes[139]="C code execution: Segmentation fault"
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

        echo "    - $test"
        resultcode=1
    done | sort
done

[ ${skip_count} -gt 0 ] && echo "Skipped ($skip_count):"

for test in ${testcases[@]}; do
    [ "${skips[$test]}" == "" ] && continue
    echo "    - $test"
done | sort

exit $resultcode
