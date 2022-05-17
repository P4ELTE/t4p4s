
[ "$P4C" == "" ] && echo The environment variable \$P4C has to be defined to run $0 && exit 1
[ "$RTE_SDK" == "" ] && echo The environment variable \$RTE_SDK has to be defined to run $0 && exit 1

# how many Ctrl-C presses will force termination
MAX_INTERRUPTS=${MAX_INTERRUPTS-1}


P4TESTS_EXTRA_OPTS=""
EXTRA_OPT_PREFIX="p4opts+=--p4opt="
for txt in $*; do
    if [[ $txt = "${EXTRA_OPT_PREFIX}"* ]]; then
        EXTRA_OPT="${txt#${EXTRA_OPT_PREFIX}}"
        P4TESTS_EXTRA_OPTS="${P4TESTS_EXTRA_OPTS} -D ${EXTRA_OPT} "
    fi
done

declare -A exitcode
declare -A skips
declare -A skipped
declare -A testcases
declare -A models
declare -A dirname
declare -A src_p4
declare -A ext_p4
declare -A p4files

declare -a sorted_testcases
declare -a sorted_skipped

success_count=0
failure_count=0

START_DIR=${START_DIR-examples}
START_DIR=`realpath "${START_DIR}"`

BASE_DIR=$(realpath `pwd`)

HTML_REPORT=${HTML_REPORT-no}
RUN_COUNT=${RUN_COUNT-1}
TIMEOUT=${TIMEOUT-60}
FORCE_TIMEOUT=${FORCE_TIMEOUT-60}
SKIP_FILE=${SKIP_FILE-tests_to_skip.txt}

if [ -f "$SKIP_FILE" ]; then
    IFS=$'\n'
    while read -r skip_opt; do
        skips[$skip_opt]="$skip_opt"
    done < <(cat "$SKIP_FILE" | sed -e "s/^[ ]*#.*$//g" | sed -e "s/\w[ \t]{1,2}\w//g" | sed -e "s/[ \t]*//g" | grep -v '^[ \t]*$')
fi

PREFIX=${PREFIX-%}
std="@std"
[ "$PREFIX" == "%" ] && std="@test"
PRECOMPILE=${PRECOMPILE-yes}

rm /tmp/t4p4s_precompile_output_*
for file in `find -L "${START_DIR}" -name "test-*.c"`; do
    base=$(basename $file)
    noc="${base%%.c}"
    noprefix="${noc##test-}"

    found=0
    for p4file in "$noc" "$noprefix"; do
        for file2 in `find -L "${START_DIR}" -type f -regex ".*${p4file}[.]p4\(_[0-9][0-9]*\)?"`; do
            base2=`basename "$file2"`
            base2=${base2%.*}
            [ "$base2" != "$noprefix" ] && [ "$base2" != "test-$noprefix" ] && continue

            SRC_P4="$file2"
            found=1
            break 2
        done
    done

    [ $found -eq 0 ] && echo "P4 file for $file not found" && continue

    for testcase_row in `cat $file | grep "&t4p4s_testcase_" | grep -ve "^[[:blank:]]*//" | sed -e "s/[[:blank:]]//g" | sed -e "s/[{}\"]//g"`; do
        testcase=`echo $testcase_row | cut -f1 -d,`
        model=`echo $testcase_row | cut -f3 -d,`

        PREPART="$PREFIX"
        if [ -z ${POSTFIX+x} ]; then
            TESTPART="${p4file}=${testcase}"
        else
            TESTPART="${p4file}$POSTFIX"
        fi

        testid="$PREPART$TESTPART"
        testcases["$testid"]="$testid"
        p4files["$testid"]="$p4file"
        src_p4["$testid"]="$SRC_P4"
        models["$testid"]=$model
        ext="${SRC_P4##*.}"
        ext_p4["$testid"]="$ext"

        dirname["$testid"]=`echo "$testid" | sed -e "s/=/${std}-/g" | tr -d '%'`

        [ "${skipped[$PREPART$TESTPART]}" != "" ] && continue
        [ "$testcase" == "test" ] && [ "${skipped[\"$PREPART$TESTPART=test\"]}" != "" ] && continue

        for skip in ${skips[@]}; do
            [ "$skip" == "$TESTPART" ]         && skipped[$PREPART$TESTPART]="$skip" && continue 2
            [ "$PREPART$skip" == "$TESTPART" ] && skipped[$PREPART$TESTPART]="$skip" && continue 2
            [ "$skip" == "$PREPART$TESTPART" ] && skipped[$PREPART$TESTPART]="$skip" && continue 2
            [ "$testcase" == "test" ] && [ "$skip=test" == "$TESTPART" ]         && skipped[$PREPART$TESTPART]="$skip" && continue 2
            [ "$testcase" == "test" ] && [ "$PREPART$skip=test" == "$TESTPART" ] && skipped[$PREPART$TESTPART]="$skip" && continue 2
        done
    done

    if [ "$PRECOMPILE" == "yes" ]; then
        [ "${skipped[$testid]+x}" ] && continue

        TARGET_DIR="./build/${dirname[$testid]}/cache"
        TARGET_JSON="${TARGET_DIR}/${p4files[$testid]}.${ext_p4[$testid]}.json.cached"
        mkdir -p ${TARGET_DIR}

        [ -f ${TARGET_JSON} ] && [ ${TARGET_JSON} -nt "${src_p4[$testid]}" ] && echo -n "|" && continue

        [ "${ext_p4[$testid]}" == "p4_14" ] && P4VSN=14
        [ "${ext_p4[$testid]}" == "p4"    ] && P4VSN=16

        model_compile_argument=""
        [ "${models[$testid]}" == "v1model" ] && model_compile_argument="-D__TARGET_V1__"
        [ "${models[$testid]}" == "psa" ] && model_compile_argument="-D__TARGET_PSA__"

        # $P4C/build/p4test ${P4TESTS_EXTRA_OPTS} "${src_p4[$testid]}" --toJSON ${TARGET_JSON} --p4v $P4VSN --Wdisable=unused --ndebug $model_compile_argument -I $BASE_DIR/examples/include && \
        $P4C/build/p4test "${src_p4[$testid]}" --toJSON ${TARGET_JSON} --p4v $P4VSN --Wdisable=unused --ndebug $model_compile_argument -I $BASE_DIR/examples/include 2>&1 | tee /tmp/t4p4s_precompile_output_$testid && \
            gzip ${TARGET_JSON} && \
            mv ${TARGET_JSON}.gz ${TARGET_JSON} && \
            echo -n "|" &
    fi

done

wait

total_count=0
for TESTCASE in ${testcases[@]}; do
    [ "${skipped[$TESTCASE]+x}" ] && continue
    ((++total_count))
done

echo Precompiled ${total_count} P4 files to JSON

echo

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
REPORT_OUTPUT_FILE=${LOG_DIR}/$(date +"%Y%m%d_%H%M%S")_output
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

if [ ${HTML_REPORT} == "yes" ]; then
  COLLECTOR_PATH="examples/test_scripts/data_collector/data_collector.py"
  actual_commit_hash=`git rev-parse HEAD`
  echo "python3 ${COLLECTOR_PATH} new $REPORT_OUTPUT_FILE commitHash=$actual_commit_hash"
  echo ""
  python3 ${COLLECTOR_PATH} new $REPORT_OUTPUT_FILE commitHash=$actual_commit_hash
fi

INTERRUPT_COUNT=0
for TESTCASE in ${sorted_testcases[@]}; do
    [ "${skipped[$TESTCASE]+x}" ] && continue

    all_arguments="$TESTCASE $*"
    echo
    echo
    echo Running test case ${current_idx}/${total_count}: ./t4p4s.sh $all_arguments

    if [ ${HTML_REPORT} == "yes" ]; then
        tmpFilename="/tmp/t4p4s_run_result"
        set -o pipefail

        for i in $(seq $RUN_COUNT); do
          python3 examples/test_scripts/timeoutee/timeoutee.py $TIMEOUT $FORCE_TIMEOUT ${tmpFilename}_pure_output ./t4p4s.sh $all_arguments 2>&1
          #./t4p4s.sh $all_arguments|tee "${tmpFilename}_pure_output"
          exitcode["$TESTCASE"]="$?"
          [ ${exitcode["$TESTCASE"]} -ne 0 ] && break
        done
        set +o pipefail

        echo ${current_idx} > $tmpFilename
        echo $TESTCASE >> $tmpFilename
        echo ${exitcode["$TESTCASE"]} >> $tmpFilename
        echo "$TESTCASE" >> $tmpFilename
        if test -f "/tmp/t4p4s_precompile_output_$TESTCASE"; then
            echo " ------- Precompile output ------- "  >> $tmpFilename
            cat "/tmp/t4p4s_precompile_output_$TESTCASE"  >> $tmpFilename
            echo "\n\n ------- T4P4S output ------- "  >> $tmpFilename
        fi
        cat "${tmpFilename}_pure_output" >> $tmpFilename

        echo ""
        echo "python3 ${COLLECTOR_PATH} add $REPORT_OUTPUT_FILE $tmpFilename"
        python3 ${COLLECTOR_PATH} add $REPORT_OUTPUT_FILE $tmpFilename
    else
        for i in $(seq $RUN_COUNT); do
          ./t4p4s.sh $all_arguments
          exitcode["$TESTCASE"]="$?"
          [ ${exitcode["$TESTCASE"]} -ne 0 ] && break
        done
    fi
    ((++current_idx))

    [ ${exitcode["$TESTCASE"]} -eq 0 ] && ((++success_count))
    [ ${exitcode["$TESTCASE"]} -ne 0 ] && ((++failure_count))

    # if there is an interrupt, finish executing test cases
    if [ ${exitcode["$TESTCASE"]} -eq 254 ]; then
        INTERRUPT_COUNT=$((++INTERRUPT_COUNT))
        [ $INTERRUPT_COUNT -eq $MAX_INTERRUPTS ] && break
    fi
done

if [ ${HTML_REPORT} == "yes" ]; then
  echo "python3 ${COLLECTOR_PATH} end $REPORT_OUTPUT_FILE"
  python3 ${COLLECTOR_PATH} end $REPORT_OUTPUT_FILE
fi

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
    fail_codes[2]="C compilation failed (meson)"
    fail_codes[3]="C compilation failed"
    fail_codes[4]="Too many iterations, possible infinite loop"
    fail_codes[5]="Packets were unexpectedly dropped/sent"
    fail_codes[6]="Execution finished with wrong output"
    fail_codes[7]="Control flow requirements not met"
    fail_codes[8]="Not enough memory"
    fail_codes[9]="Compiler tool not found"
    fail_codes[10]="Could not determine model (e.g. v1model or psa)"
    fail_codes[11]="Egress port was not set"
    fail_codes[124]="Timeout error"
    fail_codes[134]="Stack smashing"
    fail_codes[139]="C code execution: Segmentation fault"
    fail_codes[247]="Timeout error (sigkill was needed instead of sigterm)"
    fail_codes[254]="Execution interrupted"
    fail_codes[255]="Switch execution error"

    for fc in `seq 139 -1 1` `seq 200 255`; do
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
