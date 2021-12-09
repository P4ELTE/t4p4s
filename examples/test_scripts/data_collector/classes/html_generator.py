import dataclasses
import os
from os import path
from string import Template
from typing import Optional

from .data import Data
from .report_generator import ReportGenerator
from error_codes import error_codes


class HTMLGenerator(ReportGenerator):
    report_file_extension = 'html'
    template_directory = path.realpath(path.dirname(path.realpath(__file__)) + '/../templates/report')

    def __init__(self, report_file_prefix: str):
        super().__init__(report_file_prefix)

    def new(self, **kwargs):
        kwargs['error_codes'] = error_codes
        kwargs['commit_hash_prev'] = self.get_previous_commit()
        self._render_to_report_file('new', template_values=kwargs, mode='w')

    def _add_data_to_report(self, new_data: Data):
        self._render_to_report_file('add', template_values=dataclasses.asdict(new_data))

    def end(self):
        self._render_to_report_file('end')

    def _render_to_report_file(self, template_name: str, template_values: dict = None, mode: str = 'a'):
        with open(self.report_file_path, mode) as report_file:
            report_file.write(self._render_template(template_name, self.prefix_keys(template_values)))

    def _render_template(self, name: str, values: dict = None):
        with open(os.sep.join([self.template_directory, f'{name}.html'])) as template_file:
            return Template(template_file.read()).safe_substitute(**({} if values is None else values))

    @staticmethod
    def prefix_keys(key_value_pairs: Optional[dict]):
        return None if key_value_pairs is None else {f'__{key}': value for key, value in key_value_pairs.items()}
