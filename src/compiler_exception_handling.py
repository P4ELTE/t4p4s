#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Eotvos Lorand University, Budapest, Hungary

import os
import sys
import traceback
import pkgutil
import itertools


class T4P4SHandledException(Exception):
    """This "exception" type indicates that the exception has already been handled."""
    def __init__(self):
        pass


def line_from_file(file, lineno):
    try:
        with open(file, 'r') as lines:
            from itertools import islice
            [line] = islice(lines, lineno-1, lineno)
            return line
    except:
        raise T4P4SHandledException()

def replace_genfile_in_tb(tb, genfile):
    is_not_genfile = lambda tbelem: (tbelem[0], tbelem[2], tbelem[3]) != ('<string>', '<module>', None)
    part1 = list(itertools.takewhile(is_not_genfile, tb))
    part2 = list(itertools.dropwhile(is_not_genfile, tb))

    if part2:
        lineno = part2[0][1]
        line   = line_from_file(genfile, lineno)
        if line is not None:
            new_line = line.strip()
        else:
            new_line = f"{genfile}:{lineno}"

        part2 = [(genfile, lineno, "...", new_line)] + part2[1:]

    return part1 + part2


def line_info(origfile, line):
    if line is None:
        return None

    split = line.split(' ')
    if line.strip().startswith("generated_code +="):
        import re
        file   = os.path.join("src", "hardware_indep", re.sub("[.]gen[.]py", ".py", os.path.basename(origfile)))
        lineno = int(split[-1].split(')')[0])
    elif len(split) > 2 and split[-2] == '##':
        split2 = split[-1].split(":")
        file   = split2[0]
        lineno = int(split2[1])
    else:
        return None

    return (file, lineno)



def add_generator_file_to_tb(idx, tbelem):
    (t1, t2, t3, t4) = tbelem


    info = line_info(t1, t4)
    if not info:
        return [tbelem]

    (file, lineno) = info

    line = line_from_file(file, lineno)
    if line is None:
        return [tbelem]

    return [(file, lineno, '...', line.strip()), tbelem]


def simplify_traceback(orig_tb):
    return [(rel if not rel.startswith('..') else t1, t2, t3, t4) for (t1, t2, t3, t4) in orig_tb for rel in [os.path.relpath(t1)]]


def print_with_backtrace(tb_arg, file, is_tb_extracted = False, post_mortem = False):
    if not pkgutil.find_loader('backtrace'):
        raise

    import backtrace

    exc_type, exc, tb = tb_arg

    if post_mortem:
        if pkgutil.find_loader('ipdb'):
            import ipdb
            ipdb.post_mortem(tb)
        else:
            import pdb
            pdb.post_mortem(tb)

    if not is_tb_extracted:
        tb = traceback.extract_tb(tb)

    try:
        orig_tb = simplify_traceback(tb)
        tb = replace_genfile_in_tb(tb, file)
        tb = [new_elem for idx, tbelem in enumerate(tb) for new_elem in add_generator_file_to_tb(idx, tbelem)]
    except:
        tb = orig_tb

    tb = simplify_traceback(tb)

    backtrace.hook(
        tpe=exc_type,
        value=exc,
        tb=tb,
        reverse=True,
        align=True,
        strip_path=False,
        enable_on_envvar_only=False,
        on_tty=False,
        conservative=False,
        styles={})


def errmsg_srcfile(genline, genlineno):
    split = genline.split(" ")
    if len(split) <= 2 or split[-2] != "##":
        raise T4P4SHandledException()

    split2 = split[-1].split(":")
    file   = split2[0]

    lineno = int(split2[1])
    line = line_from_file(file, lineno)
    return line, lineno

def errmsg_genfile(tb, genfile):
    extracted_tb = traceback.extract_tb(tb)
    extracted_tb_last = extracted_tb[-1]
    if extracted_tb_last[0] != '<string>':
        genfile = extracted_tb_last[0]

    genlineno = extracted_tb_last[1]
    genline = line_from_file(genfile, genlineno)
    return genline, genlineno


def _detailed_error_message(genfile, file, sys_exc_info):
    (exc_type, exc, tb) = sys_exc_info

    genline, genlineno = errmsg_genfile(tb, genfile)
    line, lineno = errmsg_srcfile(genline, genlineno)

    new_tb = [
        (genfile, genlineno, '...', genline.strip()),
        (file, lineno, '...', line.strip())
    ]

    print_with_backtrace((exc_type, exc, new_tb), file, True)


def detailed_error_message(genfile, file, sys_exc_info):
    try:
        _detailed_error_message(genfile, file, sys_exc_info)
        raise T4P4SHandledException()
    except:
        print_with_backtrace(sys_exc_info, genfile)
        raise
