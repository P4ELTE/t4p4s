# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

import sys
import os
import re
import pkgutil
import itertools
import compiler_common

errors   = []
warnings = []

# TODO these should be replaced by compiler_common.current_compilation
global filename
global filepath
global genfile
global outfile
filename = "?"
filepath = "?"
genfile = "?"
outfile = "?"

def line_from_file(file, lineno):
    try:
        with open(file, 'r') as lines:
            from itertools import islice
            [line] = islice(lines, lineno-1, lineno)
            return line
    except:
        raise T4P4SHandledException()

def insert_line_srcfile(code):
    split = code.split(" ")
    if len(split) <= 2 or split[-2] != "##":
        return None, None, None

    split2 = split[-1].split(":")
    file   = split2[0]

    lineno = int(split2[1])
    line = line_from_file(file, lineno)
    return file, line, lineno

def add_referred_tb_lines(tb, show_original=False):
    for filename, lineno, module, code in tb:
        codefile, codeline, codelineno = insert_line_srcfile(code)
        if codefile:
            yield codefile, codelineno, module, codeline.rstrip()
        if show_original:
            yield os.path.relpath(filename), lineno, module, code

# TODO unify this with functionality in compiler_exception_handling
def get_simplified_traceback():
    rootcwd = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

    import traceback
    tb  = traceback.extract_stack()
    tb = list(itertools.dropwhile(lambda module: 'src/compiler.py' in module[0], tb))
    # don't show the traceback entries for the current module and addError/addWarning
    tb = tb[:-3]
    lineno = tb[0][1]
    tb = [(genfile, lineno, tb[0][2], tb[0][3])] + tb[1:-1]

    tb = add_referred_tb_lines(tb)

    # try:
    #     with open(genfile) as f:
    #         lines = f.readlines()
    #         origlineno = int(lines[lineno+1].split(" ")[-1])

    #         with open(filepath) as f2:
    #             origlines = f2.readlines()
    #             tb = [(filepath, origlineno, "...", origlines[origlineno].strip())] + tb
    # except:
    #     pass

    return [(f".{path[len(rootcwd):]}" if path.startswith(rootcwd) else path, line, module, errmsg) for (path, line, module, errmsg) in tb]


colorama_formatter = None
pygments_lexer = None

def prepare_colour():
    from pygments.lexers import PythonLexer
    from pygments.formatters import TerminalTrueColorFormatter

    global colorama_formatter
    if colorama_formatter is None:
        colorama_formatter = TerminalTrueColorFormatter(style="paraiso-dark")

    global pygments_lexer
    if pygments_lexer is None:
        pygments_lexer = PythonLexer()


def coloured_line(lineno, space1, filename, space2, module, space3, code, has_colour):
    from colorama import Fore, Style

    if has_colour:
        from pygments import highlight

        code = highlight(code, pygments_lexer, colorama_formatter)
        # cut the line end character that pygments adds on
        code = code[:-1]
    return f'{Style.BRIGHT}{Fore.YELLOW}{filename}{Fore.RESET}:{Fore.GREEN}{lineno}{space1}{space2}{Fore.GREEN}{module}{space3}{Style.RESET_ALL}-->{code}'


def reformat_lines(txt, use_colorama=True):
# def reformat_lines(txt, use_colorama=False):
    has_colour = use_colorama and pkgutil.find_loader('pygments') is not None and pkgutil.find_loader('colorama') is not None

    if has_colour:
        prepare_colour()

    lines = txt.splitlines()

    for line in lines[2:]:
        if not has_colour or '-->' not in line:
            yield line
            continue

        pre, code = line.split('-->')
        lineno, space1, filename, space2, module, space3 = re.match(r'([^ ]*)([ ]*)([^ ]*)([ ]*)([^ ]*)([ ]*)', pre).groups()
        yield coloured_line(lineno, space1, filename, space2, module, space3, code, has_colour)


def get_backtrace_lines(tb):
    """As backtrace.hook prints to stderr, its output has to be captured to get it as a text."""

    import backtrace
    import io
    import contextlib
    with io.StringIO() as buf:
        with contextlib.redirect_stderr(buf):
            backtrace.hook(
                tpe='ignored-tpe',
                value='ignored-value',
                tb=tb,
                reverse=True,
                align=True,
                strip_path=False,
                enable_on_envvar_only=False,
                on_tty=False,
                conservative=False)

            return reformat_lines(buf.getvalue())


def get_traceback_txts():
    tb = get_simplified_traceback()

    if pkgutil.find_loader('backtrace'):
        # uses the backtrace module to prettify output
        return [f"    {line}" for line in get_backtrace_lines(tb)]
    else:
        import traceback
        return traceback.format_list(tb)

def make_msg(where, msg_prefix, msg, use_traceback, show_details):
    msg = f"{msg_prefix}: while {where}: {msg}"
    if show_details:
        msg = f'{msg} {compiler_common.current_compilation}'
    tb_txt = get_traceback_txts() if use_traceback else []
    return [msg] + tb_txt


def addError(where, msg, use_traceback=True, show_details=False):
    global errors
    errors += make_msg(where, "Error", msg, use_traceback, show_details)


# def addWarning(where, msg, use_traceback=True, show_details=False):
def addWarning(where, msg, use_traceback=False, show_details=False):
    global warnings
    warnings += make_msg(where, "Warning", msg, use_traceback, show_details)


def showErrors():
    global errors
    for e in errors:
        print(e, file=sys.stderr)


def showWarnings():
    global warnings
    for w in warnings:
        print(w, file=sys.stderr)
