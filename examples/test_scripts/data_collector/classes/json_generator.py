import dataclasses
import json
from typing import List

from .data import Data
from .report_generator import ReportGenerator


class JSONGenerator(ReportGenerator):
    report_file_extension = 'json'

    def new(self):
        with open(self.report_file_path, 'w') as report_file:
            report_file.write('[]')

    def _add_data_to_report(self, new_data: Data):
        with open(self.report_file_path) as report_file:
            report_data: List[dict] = json.load(report_file)

        report_data.append(dataclasses.asdict(new_data))

        with open(self.report_file_path, 'w') as report_file:
            json.dump(report_data, report_file)
