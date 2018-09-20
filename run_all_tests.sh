
declare -A exitcode

success_count=0
failure_count=0

for file in `ls examples/test-*.c`; do
    p4file="${file##examples/test-}"
    p4file="${p4file%%.c}"

    found=0
    for p4base in "${file##examples/test-}" "${file##examples/}"; do
        p4file="${p4base%%.c}"
        for ext in ".p4" ".p4_14"; do
            [ -f "examples/$p4file$ext" ] && found=1 && break 2
        done
    done

    [ $found -eq 0 ] && echo "P4 file for $file not found" && continue

    for testcase in `cat $file | grep "&t4p4s_testcase_" | sed -e "s/^.*&t4p4s_testcase_//g" | sed -e "s/ .*//g"`; do
        tested="%${p4file}=${testcase}"
        [ "$testcase" == "test" ] && tested="%${p4file}"

        ./t4p4s.sh $tested $*
        exitcode["$tested"]="$?"
        [ ${exitcode["$tested"]} -eq 0 ] && ((++success_count))
        [ ${exitcode["$tested"]} -ne 0 ] && ((++failure_count))
    done
done


resultcode=0
[ ${success_count} -gt 0 ] && echo Successes:

for test in ${!exitcode[@]}; do
    [ ${exitcode[$test]} -ne 0 ] && continue
    echo "    - $test"
done | sort

[ ${failure_count} -gt 0 ] && echo Failures:

for test in ${!exitcode[@]}; do
    [ ${exitcode[$test]} -eq 0 ] && continue
    echo "    - $test --> ${exitcode[$test]}"
    resultcode=1
done | sort

exit $resultcode
