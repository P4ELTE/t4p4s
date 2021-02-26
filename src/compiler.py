#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# SPDX-License-Identifier: Apache-2.0
# Copyright 2016-2020 Eotvos Lorand University, Budapest, Hungary

import argparse
from hlir16.hlir import *
from compiler_log_warnings_errors import *
import compiler_log_warnings_errors

from compiler_load_p4 import load_from_p4

from compiler_exception_handling import *
import compiler_common

import re
import os
import sys
import pkgutil


generate_code_files = True

# Inside the compiler, these variables are considered singleton.
args = []
hlir = None


def replace_insert2(insert):
    simple = re.split(r'^\$([a-zA-Z_][a-zA-Z_0-9]*)$', insert)
    if len(simple) == 3:
        return ("{}", simple[1])

    # replace $$[light][text1]{expr}{text2} inserts, where all parts except {expr} are optional
    m = re.match(r'(?P<type>\$\$?)(\[(?P<light>[^\]]+)\])?(\[(?P<text1>[^\]]+)\])?{\s*(?P<expr>[^}]*)\s*}({(?P<text2>[^}]+)})?', insert)

    light = m.group("light")
    txt1  = m.group('text1') or ''
    expr  = m.group('expr')
    txt2  = m.group('text2') or ''

    # no highlighting
    if m.group("type") == '$':
        fmt = f'{escape_brace(txt1)}{{}}{escape_brace(txt2)}'
    else:
        light_param = f",{light}" if light not in (None, "") else ""
        fmt = f'" T4LIT({escape_brace(txt1)}{{}}{escape_brace(txt2)}{light_param}) "'

    return (fmt, expr)


def replace_insert(insert):
    simple = re.split(r'^\$([a-zA-Z_][a-zA-Z_0-9]*)$', insert)
    if len(simple) == 3:
        yield (simple[1],)
        return

    # replace $$[light][text1]{expr}{text2} inserts, where all parts except {expr} are optional
    m = re.match(r'(?P<type>\$\$?)(\[(?P<light>[^\]]+)\])?(\[(?P<text1>[^\]]+)\])?{\s*(?P<expr>[^}]*)\s*}({(?P<text2>[^}]+)})?', insert)

    light = m.group("light")
    txt1  = m.group('text1') or ''
    expr  = m.group('expr')
    txt2  = m.group('text2') or ''

    # no highlighting
    if m.group("type") == '$':
        yield escape_brace(txt1)
        yield (fmt,)
        yield escape_brace(txt2)
    else:
        light_param = f",{light}" if light not in (None, "") else ""
        yield '" T4LIT("'
        yield escape_brace(txt1)
        yield (fmt,)
        yield escape_brace(txt2)
        if light:
            yield f",{light}"
        yield ') "'


def adjust_indentation(indenter, line_idx, indent, file):
    indent_levels = {
        "[":  0,
        "{": +1,
        "}": -1,
    }

    compiler_common.file_indentation_level += indent_levels[indenter]

    # #{ starts a new indentation level from the next line
    # also, #} unindents starting this line
    if indenter == '{' and compiler_common.file_indentation_level == 0:
        addError("Compiler", f"Too much unindent in {file}:{line_idx}")


def escape_slash(s):
    return re.sub(r'(\\|")', r'\\\1', s)

def escape_brace(s):
    return re.sub(r'(\{|\})', r'\1\1', s)

def split_and_translate(content, extra_content="", no_quote_allowed=False):
    parts = re.split(r'(\$+(?:(?:\[[^\]]*\])*(?:\{[^\}]*\})+|[a-zA-Z_][a-zA-Z_0-9]*))', content)
    return translate_line_main_content2(parts, extra_content, no_quote_allowed)


def translate_line_main_content(parts, extra_content, no_quote_allowed):
    replaceds = [repl for part in parts for repl in replace_insert(part)]
    raws = [part[0] if type(part) is tuple else part for part in replaceds]

    apostrophe_count = len("'" in raw for raw in raws)
    quote_count = len('"' in raw for raw in raws)

    if quote_count == 0:
        content = "".join(f'{{{part[0]}}}' if type(part[0]) is tuple else part for part in replaceds)
        return False, f'{content}'

    if apostrophe_count == 0:
        content = "".join(f'{{{part[0]}}}' if type(part[0]) is tuple else part for part in replaceds)
        return False, f"f'{content}'"

    return translate_line_main_content2(parts, extra_content, no_quote_allowed)


def translate_line_main_content2(parts, extra_content, no_quote_allowed):
    if len(parts) == 1:
        if no_quote_allowed and '\\' not in parts[0] and '"' not in parts[0]:
            return False, parts[0]
        return True, f'"{escape_slash(parts[0])}"'

    match_with_rests = [(replace_insert2(parts[1+2*i]), parts[2+2*i]) for i in range((len(parts)-1)//2)]

    all_fmt = "".join(((re.sub(r'\{\}', '', fmt) if expr == "" else fmt) + escape_brace(txt) for (fmt, expr), txt in match_with_rests))
    all_fmt = escape_slash(f'{escape_brace(parts[0])}{all_fmt}') + extra_content
    if "'" not in all_fmt:
        quote = "'"
        all_fmt = re.sub(r'\\"', '"', all_fmt)
    else:
        quote = '"'

    all_escapes_txt = ", ".join((escape_brace(expr) or '""' for (fmt, expr), txt in match_with_rests if expr != ""))

    if all_escapes_txt == "":
        if no_quote_allowed:
            return False, f'{all_fmt}'.strip()
        return True, f'{quote}{all_fmt}{quote}'
    else:
        return True, f'{quote}{all_fmt}{quote}.format({all_escapes_txt})'


def translate_line_with_insert(file, genfile, line_idx, line, indent_str):
    """Gets a line that contains an insert
       and transforms it to a Python code section."""
    _empty, indent, maybe_pre, indenter, content, _empty2 = re.split(r'^([ \t]*)#(pre|aft)?([\[\{\}])(.*)$', line)

    adjust_indentation(indenter, line_idx, indent, file)
    prepend_append_funname = "prepend" if maybe_pre == "pre" else "append" if maybe_pre == "aft" else ""
    prepend_append_txt = f"[{maybe_pre}]" if maybe_pre != "" else ""
    no_hint = "nohint" in args['hint']
    extra_content = f" // {prepend_append_txt} {print_file(file, genfile)}:{line_idx}\\n" if not no_hint and maybe_pre else ""

    _is_escaped, line = split_and_translate(content, extra_content)

    if maybe_pre:
        return f'{indent}{prepend_append_funname}_statement({line})'

    if line_idx is None:
        return f'{indent}generated_code += add_code({line})'
    return f'{indent}generated_code += add_code({line}, {line_idx})'


def increase(idx):
    if idx is None:
        return None
    return idx + 1


def add_empty_lines(code_lines):
    """Returns an enumerated list of the lines.
    When an empty line separates follows an escaped code part,
    an empty line is inserted into the generated list with None as line number."""
    new_lines = []
    is_block_with_sequence = False
    last_indent = 0
    already_added = False
    for idx, line in code_lines:
        if "#[" in line:
            is_block_with_sequence = True

        if not line.strip() and last_indent == 0 and not already_added:
            new_lines.append((idx, line))
            new_lines.append((None, "#["))
            last_indent = 0
            already_added = True
        else:
            if not line.strip():
                continue
            new_lines.append((increase(idx), line))
            last_indent = len(line) - len(line.lstrip())
            already_added = False

    return new_lines


def add_gen_in_def(code_lines, orig_file):
    """If a function's name starts with 'gen_' in a generated file,
    that function produces code.
    This is a helper function that initialises and returns the appropriate variable.
    Also, if "return" is encountered on a single line,
    the requisite return value is inserted."""
    new_lines = []
    is_inside_gen = False
    for idx, line in code_lines:
        if is_inside_gen:
            if re.match(r'^[ \t]*return[ \t]*$', line):
                line = re.sub(r'^([ \t]*)return[ \t]*$', r'\1return generated_code', line)

            is_separator_line  = re.match(r'^#[ \t]*([^ \t])\1\1*', line)
            is_method_line     = re.sub(r'[ \t]*#.*', '', line).strip() != "" and line.lstrip() == line
            is_unindented_line = re.match(r'^[^ \t]', line)
            if is_separator_line or is_method_line or is_unindented_line:
                new_lines.append((None, '    return generated_code'))
                new_lines.append((None, ''))
                is_inside_gen = False

        if line.startswith('def gen_'):
            new_lines.append((idx, line))
            new_lines.append((None, '    generated_code = ""'))
            is_inside_gen = True
            continue

        new_lines.append((idx, line))

    if is_inside_gen:
        new_lines.append((None, '    return generated_code'))
        new_lines.append((None, ''))

    return new_lines


def print_file(file, genfile):
    nopath = os.path.basename(file)
    noext = re.sub(r'[.].*$', '', nopath)

    if "nofile" in args['hint']: return ''
    if "noext"  in args['hint']: return noext
    if "nopath" in args['hint']: return nopath
    return file


def translate_file_contents(file, genfile, code, prefix_lines="", add_lines=True, relpath=None):
    """Returns the code transformed into runnable Python code.
       Translated are #[generated_code, #=generator_expression and ${var} constructs."""
    global indentation_level
    indentation_level = 0

    no_hint = "nohint" in args['hint']

    new_lines = prefix_lines.splitlines()
    new_lines += """
# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Eotvos Lorand University, Budapest, Hungary

# Autogenerated file (from {0}), do not modify directly.
# Generator: T4P4S (https://github.com/P4ELTE/t4p4s/)

import compiler_common
import re

def add_code(line, lineno = None, file = "{0}"):
    global generated_code

    line_ends = {{
        "line_comment": "\\n",
        "inline_comment": "",
        "no_comment": "",
    }}

    sugar_style = compiler_common.file_sugar_style[-1]

    stripped_line = line.strip()
    no_sugar_on_line = stripped_line.startswith('//') or stripped_line.startswith('# ') or stripped_line == ""

    indent = '{2}' * compiler_common.file_indentation_level
    return indent + line + sugar(no_sugar_on_line, file, lineno, sugar_style) + line_ends[sugar_style]


def sugar(no_sugar_on_line, file, lineno, sugar_style):
    if {1} or no_sugar_on_line or file is None or lineno is None:
        return ""

    if sugar_style == 'line_comment':
        return f" // {{file}}:{{lineno}}"
    if sugar_style == 'inline_comment':
        return f" /* {{file}}:{{lineno}} */"
    return ""


generated_code += "// Autogenerated file (from {0} via {3}), do not modify directly.\\n"
generated_code += "// Generator: T4P4S (https://github.com/P4ELTE/t4p4s/)\\n"
generated_code += "\\n"

""".format(print_file(file, genfile), no_hint, compiler_common.file_indent_str, os.path.relpath(file, relpath) if relpath is not None else file).splitlines()

    code_lines = enumerate(code.splitlines())
    code_lines = add_gen_in_def(code_lines, file)

    if add_lines:
        code_lines = add_empty_lines(code_lines)

    has_translatable_comment = re.compile(r'^([ \t]*)#(pre|aft)?([\[\{\}])(.*)$')

    for idx, code_line in code_lines:
        new_line = code_line
        if has_translatable_comment.match(code_line):
            new_line = translate_line_with_insert(file, genfile, idx, code_line, compiler_common.file_indent_str)
        elif re.match(r'^[ \t]*#= .*$', code_line):
            line_regex = r'^([ \t]*)#=[ \t]*(.*)$'
            with compiler_common.SugarStyle('no_comment'):
                line_indent, line_content = re.match(line_regex, code_line).groups()
                is_escaped, code_part = split_and_translate(line_content, no_quote_allowed=True)
                if is_escaped:
                    code_part = f'eval({code_part})'

            new_indent = re.sub(line_regex, r'\1', code_line)
            new_line = f'{line_indent}generated_code += {code_part}'

            if args['desugar_info'] == "comment":
                # sugar_filename = os.path.basename(file)
                # sugar_filename = re.sub("([.]sugar)?[.]py", "", sugar_filename)
                sugar_filename = file
                new_line += f" ## {os.path.relpath(sugar_filename, '.')}:{idx}"

        stripped = new_line.strip()

        # won't mark empty lines and continued lines
        if stripped != "" and new_line.strip()[-1] != '\\' and idx is not None and not stripped.startswith('generated_code +='):
            # TODO idx is sometimes off by one?
            new_line += f" ## {os.path.relpath(file, '.')}:{int(idx) + 1}"

        new_lines.append(new_line)

    if indentation_level != 0:
        addError("Compiler", f"Non-zero indentation level ({indentation_level}) at end of file: {file}")

    return '\n'.join(new_lines) + "\n"


def generate_code(file, genfile, localvars={}):
    """The file contains Python code with #[ inserts.
       The comments (which have to be indented properly)
       contain code to be output,
       their contents are collected in the variable generated_code.
       Inside the comments, refer to Python variables as ${variable_name}."""
    with open(file, "r") as orig_file:
        code = orig_file.read()
        code = translate_file_contents(file, genfile, code, relpath="src/")

        if generate_code_files:
            write_file(genfile, code)

        localvars['generated_code'] = ""
        module_name = genfile

        localvars['t4p4sdir'] = os.path.relpath(os.path.join(args['generated_dir'], '..', "gen"))
        exec(compile(code, module_name, 'exec'), localvars, localvars)

        return re.sub(r'\n{3,}', '\n\n', localvars['generated_code'])


def generate_desugared_py():
    """Some Python source files also use the sugared syntax.
    The desugared files are generated here."""
    import glob
    for fromfile in glob.glob("src/utils/*.sugar.py"):
        tofile = re.sub("[.]sugar[.]py$", ".py", fromfile)
        compiler_common.current_compilation = { 'from': fromfile, 'to': tofile, 'use_real_random': args['use_real_random'] }
        with open(fromfile, "r") as orig_file:
            code = orig_file.read()
            prefix_lines = "generated_code = \"\"\n"
            code = translate_file_contents(fromfile, tofile, code, prefix_lines=prefix_lines, add_lines=False, relpath="src/")

            write_file(tofile, code)

    compiler_common.current_compilation = None


def generate_desugared_c(filename, filepath):
    global hlir

    genfile = os.path.join(args['desugared_path'], re.sub(r'\.([ch])\.py$', r'.\1.gen.py', filename))
    outfile = os.path.join(args['generated_dir'], re.sub(r'\.([ch])\.py$', r'.\1', filename))

    compiler_common.current_compilation = { 'orig': filename, 'from': genfile, 'to': outfile, 'use_real_random': args['use_real_random'] }

    compiler_log_warnings_errors.filename = filename
    compiler_log_warnings_errors.filepath = filepath
    compiler_log_warnings_errors.genfile = genfile
    compiler_log_warnings_errors.outfile = outfile

    code = generate_code(filepath, genfile, {'hlir': hlir})
    write_file(outfile, code)

    compiler_common.current_compilation = None


def make_dirs(cache_dir_name):
    """Makes directories if they do not exist"""
    if not os.path.isdir(args['compiler_files_dir']):
        print("Compiler files path is missing", file=sys.stderr)
        sys.exit(1)

    if not os.path.isdir(args['desugared_path']):
        os.makedirs(args['desugared_path'])
        args['verbose'] and print(" GEN {0} (desugared compiler files)".format(args['desugared_path']))

    if not os.path.isdir(args['generated_dir']):
        os.makedirs(args['generated_dir'])
        args['verbose'] and print(" GEN {0} (generated files)".format(args['generated_dir']))

    if cache_dir_name and not os.path.isdir(cache_dir_name):
        os.mkdir(cache_dir_name)


def file_contains_exact_text(filename, text):
    """Returns True iff the file exists and it already contains the given text."""
    if not os.path.isfile(filename):
        return False

    with open(filename, "r") as infile:
        intext = infile.read()
        return text == intext

    return False


def write_file(filename, text):
    """Writes the given text to the given file."""

    if file_contains_exact_text(filename, text):
        return

    if filename.endswith(".gen.py"):
        args['verbose'] and print("  P4", os.path.basename(filename))

    with open(filename, "w") as genfile:
        genfile.write(text)


def init_args():
    """Parses the command line arguments and loads them
    into the global variable args."""
    parser = argparse.ArgumentParser(description='T4P4S compiler')
    parser.add_argument('p4_file', help='The source file')
    parser.add_argument('-v', '--p4v', help='Use P4-14 (default is P4-16)', required=False, choices=[16, 14], type=int, default=16)
    parser.add_argument('-p', '--p4c_path', help='P4C path', required=False)
    parser.add_argument('-c', '--compiler_files_dir', help='Source directory of the compiler\'s files', required=False, default=os.path.join("src", "hardware_indep"))
    parser.add_argument('-g', '--generated_dir', help='Output directory for hardware independent files', required=True)
    parser.add_argument('-desugared_path', help='Output directory for the compiler\'s files', required=False, default=argparse.SUPPRESS)
    parser.add_argument('-desugar_info', help='Markings in the generated source code', required=False, choices=["comment", "pragma", "none"], default="comment")
    parser.add_argument('-verbose', help='Verbosity', required=False, default=False, action='store_const', const=True)
    parser.add_argument('-hint', help='Filename hint style in generated code comments', required=False, default=[], action='append')
    parser.add_argument('-recompile', help='Force recompilation: ignore cache files', required=False, default=False, action='store_const', const=True)
    parser.add_argument('-beautify', help='Beautification', required=False, default=False, action='store_const', const=True)
    parser.add_argument('-use_real_random', help='Use random values in unspecified cases', required=False, default=False, action='store_const', const=True)
    parser.add_argument('--p4dbg', help='Debugging', required=False, default=False, action='store_const', const=True)
    parser.add_argument('--p4opt', help='Debug option passed to P4-to-JSON compiler', required=False, default=[], action='append')

    args = vars(parser.parse_args())

    if 'desugared_path' not in args:
        args['desugared_path'] = os.path.relpath(os.path.join(args['generated_dir'], '..', "gen"))

    cache_dir_name = os.path.relpath(os.path.join(args['generated_dir'], '..', "cache"))

    if args['hint'] == []:
        args['hint'] = ['nohint']

    return args, cache_dir_name


def generate_files():
    base = args['compiler_files_dir']
    exts = [".c.py", ".h.py"]

    generate_desugared_py()
    for filename in (f for f in os.listdir(base) if os.path.isfile(os.path.join(base, f)) for ext in exts if f.endswith(ext)):
        generate_desugared_c(filename, os.path.join(base, filename))


def main():
    try:
        global args
        args, cache_dir_name = init_args()
        make_dirs(cache_dir_name)

        global hlir
        hlir = load_from_p4(args, cache_dir_name)
        generate_files()

        showErrors()
        showWarnings()
    except T4P4SHandledException:
        sys.exit(1)
    except:
        cuco = compiler_common.current_compilation
        if cuco:
            stagetxt = f"{cuco['stage']['name']}: " if 'stage' in cuco else ""
            print(f"{stagetxt}Error during the compilation of {cuco['from']} to {cuco['to']}")
        print_with_backtrace(sys.exc_info(), cuco['from'] if cuco else "(no compiler_common file)", args['p4dbg'])
        sys.exit(1)

    global errors
    if len(errors) > 0:
        sys.exit(1)



if __name__ == '__main__':
    main()
