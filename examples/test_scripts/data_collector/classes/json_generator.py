import dataclasses
import json
from typing import List, Dict, Any

from .data import Data
from .report_generator import ReportGenerator


class JSONGenerator(ReportGenerator):
    report_file_extension = 'json'

    def new(self, **kwargs):
        with open(self.report_file_path, 'w') as report_file:
            json.dump({**kwargs, 'data': []}, report_file)

    def _add_data_to_report(self, new_data: Data):
        with open(self.report_file_path) as report_file:
            report: Dict[str, Any] = json.load(report_file)

        existing_data: List[dict] = report['data']
        existing_data.append(dataclasses.asdict(new_data))

        with open(self.report_file_path, 'w') as report_file:
            json.dump(report, report_file)
