def are_equal(arr1, arr2):
    n = len(arr1)
    m = len(arr2)

    if n != m:
        return False

    arr1.sort()
    arr2.sort()

    for i in range(0, n - 1):
        if arr1[i] != arr2[i]:
            return False

    return True


def get_results_by_code_and_name(type_comparison, report, report_type):
    for result in report:
        code_string = str(result['code'])
        if code_string not in type_comparison:
            type_comparison[code_string] = {}

            if 'prev' not in type_comparison[code_string]:
                type_comparison[code_string]['prev'] = []
                type_comparison[code_string]['actual'] = []

        if result['name'] not in type_comparison[code_string][report_type]:
            type_comparison[code_string][report_type].append(result['name'])

    return type_comparison


def calculate_diff(type_comparison):
    diff = {
        "is_changed": 0,
        "change": {}
    }

    for code_string in type_comparison:
        if code_string != '0' \
                and not are_equal(type_comparison[code_string]['prev'], type_comparison[code_string]['actual']):
            diff['is_changed'] = 1
            diff['change'][code_string] = {
                'new': [],
                'fixed': []
            }

            for name in type_comparison[code_string]['prev']:
                if name not in type_comparison[code_string]['actual']:
                    diff['change'][code_string]['fixed'].append(name)

            for name in type_comparison[code_string]['actual']:
                if name not in type_comparison[code_string]['prev']:
                    diff['change'][code_string]['new'].append(name)

    return diff


def compare_current_with_prev_result(reports, act_index=None, prev_index=None):
    if len(reports) > 1:
        actual_report = reports[act_index] if act_index else reports[0]
        prev_report = reports[prev_index] if prev_index else reports[1]

        prev_results = get_results_by_code_and_name({}, prev_report['result'], 'prev')
        merged_results = get_results_by_code_and_name(prev_results, actual_report['result'], 'actual')

        return calculate_diff(merged_results)
    else:
        return {
            "is_changed": 0,
            "change": {}
        }
