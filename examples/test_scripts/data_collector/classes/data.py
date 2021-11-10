from dataclasses import dataclass


@dataclass
class Data:
    index: int
    name: str
    exit_code: int
    output: str
