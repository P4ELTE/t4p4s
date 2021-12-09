import argparse
import sys

from classes import HTMLGenerator
from classes import JSONGenerator
from classes import ReportCollectionGenerator

generator_classes = [HTMLGenerator, JSONGenerator, ReportCollectionGenerator]

if __name__ == '__main__':
    report_file_prefix_parameter = 'report_file_prefix'

    argument_parser = argparse.ArgumentParser()
    subparsers = argument_parser.add_subparsers(dest='action')

    parser_for_new = subparsers.add_parser('new')
    parser_for_new.add_argument(report_file_prefix_parameter)

    parser_for_add = subparsers.add_parser('add')
    parser_for_add.add_argument(report_file_prefix_parameter)
    parser_for_add.add_argument('data_file_path')

    parser_for_end = subparsers.add_parser('end')
    parser_for_end.add_argument(report_file_prefix_parameter)

    known_arguments, unknown_arguments = argument_parser.parse_known_args()

    known_arguments = vars(known_arguments)
    if (action_name := known_arguments.pop('action', None)) is None:
        argument_parser.print_usage()
        sys.exit()

    report_file_prefix = known_arguments.pop(report_file_prefix_parameter)

    additional_parameters = dict()
    for argument in unknown_arguments:
        argument_parts = argument.split('=')
        if len(argument_parts) == 2:
            additional_parameters[argument_parts[0]] = argument_parts[1]
        else:
            raise ValueError(f"Invalid additional argument: {argument!r}. Must be a key-value pair separated by '='")

    for generator_class in generator_classes:
        report_generator = generator_class(report_file_prefix)
        if callable(action := getattr(report_generator, action_name, None)):
            action(**known_arguments, **additional_parameters)
        else:
            raise NotImplementedError(f'Unknown action {action_name!r} in {generator_class.__name__}')
