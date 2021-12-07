import os
import json
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Optional

from .data import Data


class ReportGenerator(ABC):
    report_file_extension: Optional[str] = None

    def __init__(self, report_file_prefix: str):
        self.report_file_prefix = report_file_prefix
        self.report_file_path = report_file_prefix + (
            '' if self.report_file_extension is None else f'.{self.report_file_extension}'
        )

    def get_previous_commit(self):
        commit_hash_prev = None
        file_path = os.path.dirname(self.report_file_prefix) + '/result_collection.json'
        if os.path.exists(file_path) and os.stat(file_path).st_size != 0:
            with open(file_path, 'r') as report_file:
                json_data = json.load(report_file)
                commit_hash_prev = json_data['data'][0]['commit_hash'] if 'commit_hash' in json_data['data'][0] else None

        return commit_hash_prev

    def new(self, **kwargs):
        Path(self.report_file_path).touch()

    def add(self, data_file_path: str):
        self._add_data_to_report(self._get_data_from_file(data_file_path))

    @staticmethod
    def _get_data_from_file(data_file_path: str):
        with open(data_file_path) as data_file:
            index = int(data_file.readline().strip())
            name = data_file.readline().strip()
            exit_code = int(data_file.readline().strip())
            output = ''.join(data_file.readlines())

        return Data(index, name, exit_code, output)

    @abstractmethod
    def _add_data_to_report(self, new_data: Data):
        return

    def end(self):
        pass
