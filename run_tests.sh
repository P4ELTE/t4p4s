
declare -A exitcode
declare -A skips
declare -A skipped
declare -A testcases
declare -A copies
declare -A original

declare -a sorted_testcases
declare -a sorted_skipped

success_count=0
failure_count=0

SKIP_FILE=${SKIP_FILE-tests_to_skip.txt}

if [ -f "$SKIP_FILE" ]; then
    IFS=$'\n'
    while read -r skip_opt; do
        skips[$skip_opt]="$skip_opt"
    done < <(cat "$SKIP_FILE" | sed -e "s/^[ ]*#.*$//g" | sed -e "s/\w[ \t]{1,2}\w//g" | sed -e "s/[ \t]*//g" | grep -v '^[ \t]*$')
fi

PREFIX=${PREFIX-%}


echo Precompiling P4 files to JSON

cd examples

for file in `ls test/test-*.c`; do
    p4file="${file##test/test-}"
    p4file="${p4file%%.c}"

    found=0
    for p4base in "${file##test/test-}" "${file##test/}"; do
        p4file="${p4base%%.c}"
        for ext in ".p4" ".p4_14"; do
            for srcdir in "." "test" "psa" "local"; do
                SRC_P4="$srcdir/$p4file$ext"
                [ -f "${SRC_P4}" ] && found=1 && break 3
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

        testcases["$PREPART$TESTPART"]="$PREPART$TESTPART"

        [ "${skipped[$PREPART$TESTPART]}" != "" ] && continue
        [ "$testcase" == "test" ] && [ "${skipped[\"$PREPART$TESTPART=test\"]}" != "" ] && continue

        for skip in ${skips[@]}; do
            [ "$skip" == "$TESTPART" ]         && skipped[$PREPART$TESTPART]="$skip" && continue 2
            [ "$PREPART$skip" == "$TESTPART" ] && skipped[$PREPART$TESTPART]="$skip" && continue 2
            [ "$skip" == "$PREPART$TESTPART" ] && skipped[$PREPART$TESTPART]="$skip" && continue 2
            [ "$testcase" == "test" ] && [ "$skip=test" == "$TESTPART" ]         && skipped[$PREPART$TESTPART]="$skip" && continue 2
            [ "$testcase" == "test" ] && [ "$PREPART$skip=test" == "$TESTPART" ] && skipped[$PREPART$TESTPART]="$skip" && continue 2
        done

        std="@std"
        [ "$PREFIX" == "%" ] && std="@test"

        TARGET_DIR="../build/${p4file}${std}-${testcase}/cache"
        TARGET_JSON="${TARGET_DIR}/${p4file}.p4.json.cached"
        mkdir -p ${TARGET_DIR}

        [ "${copies["$p4file"]+x}" ] && copies["${p4file}"]+=" ${TARGET_JSON}"
        [ ! "${copies["$p4file"]+x}" ] && copies["${p4file}"]=""

        if [ ! "${original["$p4file"]+x}" ]; then
            [ -f "${TARGET_JSON}" ] && [ "${TARGET_JSON}" -nt $file ] && continue

            original["${p4file}"]="${TARGET_JSON}"

            # compile only the "original", the first encountered source file
            [ "$ext" == ".p4_14" ] && $P4C/build/p4test "${SRC_P4}" -I $P4C/p4include --toJSON ${TARGET_JSON} --Wdisable --p4v 14 &
            [ "$ext" == ".p4"    ] && $P4C/build/p4test "${SRC_P4}" -I $P4C/p4include --toJSON ${TARGET_JSON} --Wdisable --p4v 16 &
        fi
    done
done

wait

echo Copying JSON files for other tests in the same example


ORIG_IFS=$IFS
IFS=" "
for p4file in ${!original[@]}; do
    for dest in ${copies[$p4file]}; do
        cp ${original[$p4file]} $dest
    done
done

cd - >/dev/null


summary_echo() {
    echo "$*"
    echo "$*" >> ${SUMMARY_FILE}
    echo "$*" >> ${LAST_SUMMARY_FILE}
}

summary_only_echo() {
    echo "$*" >> ${SUMMARY_FILE}
    echo "$*" >> ${LAST_SUMMARY_FILE}
}


LOG_DIR=build/all-run-logs
SUMMARY_FILE=${LOG_DIR}/$(date +"%Y%m%d_%H%M%S")_log.txt
LAST_SUMMARY_FILE=${LOG_DIR}/last_log.txt
mkdir -p ${LOG_DIR}
rm -f ${LOG_DIR}/*.result.txt
rm -f ${SUMMARY_FILE}
rm -f ${LAST_SUMMARY_FILE}


# sorting testcases
readarray -t sorted_testcases < <(printf '%s\0' "${testcases[@]}" | sort -z | xargs -0n1)
readarray -t sorted_skipped < <(printf '%s\0' "${skipped[@]}" | sort -z | xargs -0n1)

echo "Will write summary to ${LAST_SUMMARY_FILE}"
summary_only_echo Will skip ${#skipped[@]} test cases:

previous_case=""
for test in ${sorted_skipped[@]}; do
    [ "$previous_case" != "" ] && [ "${test%%=*}" != "$previous_case" ] && summary_only_echo ""
    summary_only_echo "    $test"

    previous_case=${test%%=*}
done | sort


total_count=0
for TESTCASE in ${testcases[@]}; do
    [ "${skipped[$TESTCASE]+x}" ] && continue
    ((++total_count))
done

summary_only_echo Will run the following ${total_count} test cases:

previous_case=""
for TESTCASE in ${sorted_testcases[@]}; do
    [ "${skipped[$TESTCASE]+x}" ] && continue

    [ "$previous_case" != "" ] && [ "${TESTCASE%%=*}" != "$previous_case" ] && summary_only_echo ""
    previous_case=${TESTCASE%%=*}
    summary_only_echo "    ${TESTCASE}"
done

echo Will run ${total_count} cases and skip ${#skipped[@]} cases


current_idx=1

for TESTCASE in ${sorted_testcases[@]}; do
    [ "${skipped[$TESTCASE]+x}" ] && continue

    echo
    echo
    echo Running test case ${current_idx}/${total_count}: ./t4p4s.sh $TESTCASE $*
    ./t4p4s.sh $TESTCASE $*
    exitcode["$TESTCASE"]="$?"

    ((++current_idx))

    [ ${exitcode["$TESTCASE"]} -eq 0 ] && ((++success_count))
    [ ${exitcode["$TESTCASE"]} -ne 0 ] && ((++failure_count))

    # if there is an interrupt, finish executing test cases
    [ ${exitcode["$TESTCASE"]} -eq 254 ] && break 2
done

resultcode=0

[ ${#skipped[@]} -gt 0 ] && echo "Skipped ${#skipped[@]} test cases"

if [ ${success_count} -eq $((${#testcases[@]}-${#skipped[@]})) ]; then
    echo "All ${success_count} test cases ran successfully"
else
    [ ${success_count} -gt 0 ] && summary_echo "Successful (${success_count}):"

    for test in ${!exitcode[@]}; do
        [ ${exitcode[$test]} -ne 0 ] && continue
        summary_echo "    $test"
    done | sort

    declare -A fail_codes
    fail_codes[1]="P4 to C compilation failed"
    fail_codes[2]="C compilation failed"
    fail_codes[3]="Execution finished with wrong output"
    fail_codes[139]="C code execution: Segmentation fault"
    fail_codes[254]="Execution interrupted"
    fail_codes[255]="Switch execution error"

    for fc in 1 2 3 139 254 255; do
        failcode_count=0
        for test in ${!exitcode[@]}; do
            [ ${exitcode[$test]} -eq $fc ] && ((++failcode_count))
        done

        [ ${failcode_count} -eq 0 ] && continue

        summary_echo "Failed with code $fc ($failcode_count examples): ${fail_codes[$fc]}"

        for test in ${!exitcode[@]}; do
            [ ${exitcode[$test]} -ne $fc ] && continue

            part1=`echo "${test%%=*}" | tr -d '%'`
            part2="${test##*=}"
            fctxt=`printf "%03d" $fc`
            [ -f "build/${part1}@test-${part2}/log/last.lit.txt" ] && cp "build/${part1}@test-${part2}/log/last.lit.txt" "${LOG_DIR}/fail${fctxt}_${part1}-${part2}.result.txt"

            summary_echo "    $test"
            resultcode=1
        done | sort
    done
fi

IFS=ORIG_IFS

echo "Summary written to ${LAST_SUMMARY_FILE}"

exit $resultcode
