import json
import os

from pathlib import Path
from typing import List, Dict, Any
from datetime import datetime

from error_codes import error_codes
from helper.diff_calculator import compare_current_with_prev_result
from .json_generator import JSONGenerator
from .html_generator import HTMLGenerator
from .data import Data


class ReportCollectionGenerator(JSONGenerator, HTMLGenerator):
    def __init__(self, report_file_prefix: str):
        self.error_code_types = []
        super().__init__(report_file_prefix)

    @staticmethod
    def _count_error_types(report: List[dict]):
        error_types = {}
        for data in report:
            if 'exit_code' in data:
                exit_code_string = str(data['exit_code'])
                error_types[exit_code_string] = error_types.get(exit_code_string, 0) + 1

        return error_types

    @staticmethod
    def _get_previous_results(json_collection_path):
        results = {'data': []}
        if os.stat(json_collection_path).st_size != 0:
            with open(json_collection_path) as json_file:
                results: Dict[str, Any] = json.load(json_file)

        return results['data']

    @staticmethod
    def _create_json_collection_file(json_collection_path):
        with open(json_collection_path, 'w') as json_file:
            json.dump({'data': []}, json_file)

    @staticmethod
    def _generate_json_collection(results, json_collection_path):
        with open(json_collection_path, 'w') as json_file:
            json.dump({'data': results}, json_file)

    def _get_current_result(self, current_result_path):
        with open(current_result_path) as report_file:
            json_data: Dict[str, Any] = json.load(report_file)

        date = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        errors = self._count_error_types(json_data['data'])

        return {
            'date': date,
            'filename': os.path.basename(self.report_file_prefix + '.html'),
            'commit_hash': json_data['commitHash'] if 'commitHash' in json_data else None,
            'code_list': [{'code': int(error_code), 'count': errors[error_code]} for error_code in errors],
            'result': [{'name': data['name'], 'code': data['exit_code']} for data in json_data['data']]
        }

    def _save_error_code_types(self, results):
        all_types = {}
        for error_dict in results:
            for error in error_dict['code_list']:
                code = str(error['code'])
                all_types[code] = all_types.get(code, error['code'])

        self.error_code_types = sorted(all_types.values())

    def _merge_results(self, current_result_path, json_collection_path):
        current_result = self._get_current_result(current_result_path)
        results = self._get_previous_results(json_collection_path)
        results.insert(0, current_result)

        return results

    def _create_or_truncate_result_collection_html(self):
        Path(self.report_file_path).touch()
        file = open(self.report_file_path, "r+")
        file.truncate()

    def _add_codes_to_table_header(self):
        for code in self.error_code_types:
            self._render_to_report_file('table_head_code', {'code': code})

    def _add_code_column_to_row(self, code_type, code_list):
        code_dict = {'code': '', 'count': ''}
        for code_element in code_list:
            if code_element['code'] == code_type:
                code_dict = code_element
                break

        self._render_to_report_file('table_row_code', code_dict)

    def _fill_up_table_body(self, results):
        for index, report in enumerate(results):
            report['commit_compare_hidden_class'] = 'hidden' if index == len(results) - 1 else ''
            report['commit_hash_prev'] = results[index + 1]['commit_hash'] if index != len(results) - 1 else ''

            if report['commit_hash_prev'] == report['commit_hash'] \
                    or report['commit_hash_prev'] == '' or report['commit_hash'] == '':
                report['commit_compare_hidden_class'] = 'hidden'

            self._render_to_report_file('table_row_start', report)
            for code in self.error_code_types:
                self._add_code_column_to_row(code, report['code_list'])

            self._render_to_report_file('table_row_end', report)

    def _generate_html_collection(self, results):
        self._create_or_truncate_result_collection_html()
        self.template_directory = os.path.realpath(
            os.path.dirname(os.path.realpath(__file__)) + '/../templates/report_collection')

        self._render_to_report_file('start', {'error_codes': error_codes, 'results': results})
        self._add_codes_to_table_header()
        self._render_to_report_file('table_head_close')
        self._fill_up_table_body(results)
        self._render_to_report_file('end')

    def _get_all_previous_results(self, results, report_file_prefix):
        new_targets = []
        length = len(results)
        for index, report in enumerate(results):
            if 'result' not in report:
                new_targets.append(index)
                json_file = report_file_prefix + '/' + os.path.splitext(report['filename'])[0] + '.json'

                if os.path.exists(json_file):
                    results[index]['result'] = self._get_current_result(json_file)['result']
                else:
                    results[index]['result'] = []

        if len(new_targets) > 0:
            for index in new_targets:
                next_index = index + 1 if index < (length - 1) else index
                results[index]['diff'] = compare_current_with_prev_result(results, index, next_index)

        return results

    def _add_data_to_report(self, new_data: Data):
        pass

    def new(self, **kwargs):
        pass

    def end(self):
        current_result_path = self.report_file_path
        report_file_prefix = os.path.dirname(self.report_file_prefix)
        json_collection_path = report_file_prefix + '/result_collection.json'
        self.report_file_path = report_file_prefix + '/result_collection.html'

        if not os.path.exists(json_collection_path):
            self._create_json_collection_file(json_collection_path)

        results = self._merge_results(current_result_path, json_collection_path)
        results = self._get_all_previous_results(results, report_file_prefix)
        results[0]['diff'] = compare_current_with_prev_result(results)

        self._save_error_code_types(results)
        self._generate_json_collection(results, json_collection_path)
        self._generate_html_collection(results)
