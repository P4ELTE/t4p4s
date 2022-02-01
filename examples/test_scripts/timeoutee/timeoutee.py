import logging
import subprocess
import sys
from typing import List

def set_up_logging(output_file_path: str):
    log_level = logging.INFO

    logging.basicConfig(level=log_level, format='%(message)s', filename=output_file_path, filemode='w')
    console = logging.StreamHandler()
    console.setLevel(log_level)
    logging.getLogger().addHandler(console)


class CommandOutputGenerator:
    def __init__(self, command_line: List[str], timeout: float = None, force_timout : float = None):
        self.command_line = command_line
        self.timeout = timeout

        self.exit_code = None

    def __iter__(self):
        prefix_command = ['timeout', str(self.timeout)] if self.timeout is not None else []
        process = subprocess.Popen(
            args=prefix_command + self.command_line,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True
        )
        yield from iter(process.stdout.readline, '')
        process.stdout.close()

        self.exit_code = process.wait()


if __name__ == '__main__':
    if len(sys.argv) < 4:
        print('Parameters needed: timeout force_timout outputfile command')
        sys.exit(1)

    timeout_length = sys.argv[1]
    force_timout = sys.argv[2]
    outputfile = sys.argv[3]
    command = sys.argv[4:]

    set_up_logging(outputfile)

    for line in (generator := CommandOutputGenerator(command, timeout=timeout_length, force_timout=force_timout)):
        logging.info(line.rstrip())
    sys.exit(generator.exit_code)