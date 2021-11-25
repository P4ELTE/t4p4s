import json
import os

from pathlib import Path
from typing import List, Dict, Any
from datetime import datetime

from error_codes import error_codes
from .json_generator import JSONGenerator
from .html_generator import HTMLGenerator


class ReportCollectionGenerator(JSONGenerator, HTMLGenerator):
    actual_result_path, json_collection_path, data = None, None, None
    error_code_types = []

    @staticmethod
    def _rearrange_error_codes(errors):
        error_list = []
        for error_code in errors:
            error_list.append({
                'code': int(error_code),
                'count': errors[error_code]
            })

        return error_list

    @staticmethod
    def _count_error_types(report: List[dict]):
        error_types = {}
        for data in report:
            if 'exit_code' in data:
                exit_code_string = str(data['exit_code'])
                error_types[exit_code_string] = error_types.get(exit_code_string, 0) + 1

        return error_types

    def _get_previous_results(self):
        data = []

        if os.path.exists(self.json_collection_path):
            with open(self.json_collection_path) as json_file:
                reports: Dict[str, Any] = json.load(json_file)
                data = reports['data']
        else:
            with open(self.json_collection_path, 'w') as json_file:
                json.dump({}, json_file)

        return data

    def _get_current_result(self):
        with open(self.actual_result_path) as report_file:
            json_data: Dict[str, Any] = json.load(report_file)

        date = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        errors = self._count_error_types(json_data['data'])

        return {
            'date': date,
            'filename': os.path.basename(self.report_file_prefix + '.html'),
            'commit_hash': json_data['commitHash'] if 'commitHash' in json_data else None,
            'code_list': self._rearrange_error_codes(errors)
        }

    def _save_error_code_types(self, results):
        all_types = {}
        for error_dict in results:
            for error in error_dict['code_list']:
                code = str(error['code'])
                all_types[code] = all_types.get(code, error['code'])

        self.error_code_types = sorted(all_types.values())

    def _add_new_report_to_collection(self):
        current_result = self._get_current_result()
        results = self._get_previous_results()
        results.insert(0, current_result)
        json_collection = {'data': results}

        self.data = results
        self._save_error_code_types(results)

        with open(self.json_collection_path, 'w') as json_file:
            json.dump(json_collection, json_file)

    def _create_and_empty_result_collection_html(self):
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

    def _fill_up_table_body(self):
        for report in self.data:
            self._render_to_report_file('table_row_start', report)
            for code in self.error_code_types:
                self._add_code_column_to_row(code, report['code_list'])

            self._render_to_report_file('table_row_end', report)

    def _generate_html_collection(self):
        self._create_and_empty_result_collection_html()
        self.template_directory = os.path.realpath(os.path.dirname(os.path.realpath(__file__))
                                                   + '/../templates/report_collection')

        self._render_to_report_file('start', {'error_codes': error_codes})
        self._add_codes_to_table_header()
        self._render_to_report_file('table_head_close')
        self._fill_up_table_body()
        self._render_to_report_file('end')

    def end(self):
        self.actual_result_path = self.report_file_path
        self.report_file_path = os.path.dirname(self.report_file_prefix) + '/result_collection.html'
        self.json_collection_path = os.path.dirname(self.report_file_prefix) + '/result_collection.json'
        self._add_new_report_to_collection()
        self._generate_html_collection()
