import argparse
import sys

from classes import HTMLGenerator
from classes import JSONGenerator

report_type_mapping = {
    'html': HTMLGenerator,
    'json': JSONGenerator,
}

if __name__ == '__main__':
    report_file_prefix_parameter = 'report_file_prefix'
    report_types_parameter = 'report_types'

    argument_parser = argparse.ArgumentParser()
    subparsers = argument_parser.add_subparsers(dest='action')

    parser_for_new = subparsers.add_parser('new')
    parser_for_new.add_argument(report_file_prefix_parameter)
    parser_for_new.add_argument(report_types_parameter)

    parser_for_add = subparsers.add_parser('add')
    parser_for_add.add_argument(report_file_prefix_parameter)
    parser_for_add.add_argument(report_types_parameter)
    parser_for_add.add_argument('data_file_path')

    parser_for_end = subparsers.add_parser('end')
    parser_for_end.add_argument(report_file_prefix_parameter)
    parser_for_end.add_argument(report_types_parameter)

    arguments = vars(argument_parser.parse_known_args()[0])
    if (action_name := arguments.pop('action', None)) is None:
        argument_parser.print_usage()
        sys.exit()

    report_file_prefix = arguments.pop(report_file_prefix_parameter)
    report_types = arguments.pop(report_types_parameter).split(',')

    generator_classes = []
    for report_type in report_types:
        if (generator_class := report_type_mapping.get(report_type)) is None:
            raise NotImplementedError(f'Unknown report type {report_type!r}')
        generator_classes.append(generator_class)

    for generator_class in generator_classes:
        report_generator = generator_class(report_file_prefix)
        if callable(action := getattr(report_generator, action_name, None)):
            action(**arguments)
        else:
            raise NotImplementedError(f'Unknown action {action_name!r} in {generator_class.__name__}')
