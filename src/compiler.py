# Copyright 2016 Eotvos Lorand University, Budapest, Hungary
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#!/usr/bin/env python

from __future__ import print_function

import argparse
from hlir16.hlir16 import *
from utils.misc import *
import utils.misc
from transform_hlir16 import *

from subprocess import call

import re
import os
import sys
import traceback
import pkgutil
from os.path import isfile, join


generate_code_files = True
show_code = False
cache_dir_name = "build/.cache"

# Inside the compiler, these variables are considered singleton.
args = []
hlir = None

indentation_level = 0


def verbose_print(*txts):
    if args['verbose']:
        print(*txts)


def translate_line_with_insert(file, line_idx, line, indent_str):
    """Gets a line that contains an insert
       and transforms it to a Python code section."""
    # since Python code is generated, indentation has to be respected
    indentation = re.sub(r'^([ \t]*)#[\[\{\}].*$', r'\1', line)

    global indentation_level
    pre_indentation_mod = ""
    post_indentation_mod = ""
    content_postfix = ""
    # #{ unindents starting this line
    if '#}' in line:
        if indentation_level == 0:
            addError("Compiler", "Too much unindent in {}:{}".format(file, line_idx))
        indentation_level -= 1
        pre_indentation_mod = indentation + "file_indentation_level -= 1\n"
        content_postfix = "\\n"

    # #{ starts a new indentation level from the next line
    if '#{' in line:
        indentation_level += 1
        post_indentation_mod = "\n" + indentation + "file_indentation_level += 1"

    # get the #[ (or #{, or #}) part
    content = re.sub(r'^[ \t]*#[\[\{\}]([ \t]*[^\n]*)[ \t]*', r'\1', line)
    # escape sequences like \n may appear in #[ parts
    content = re.sub(r'\\', r'\\\\', content)
    # quotes may appear in #[ parts
    content = re.sub(r'"', r'\"', content)

    def replacer(m):
        light = m.group("light")
        txt1  = m.group('text1') or ''
        expr  = m.group('expr')
        txt2  = m.group('text2') or ''

        # no highlighting
        if m.group("type") == '$':
            return '{}" + str({}) + "{}'.format(txt1, expr, txt2)

        light_param = "," + light if light not in (None, "") else ""
        return '\\" T4LIT({}" + str({}) + "{}{}) \\"'.format(txt1, expr, txt2, light_param)

    # replace $$[light][text1]{expr}{text2} inserts, where all parts except {expr} are optional
    content = re.sub(r'(?P<type>\$\$?)(\[(?P<light>[^\]]+)\])?(\[(?P<text1>[^\]]+)\])?{\s*(?P<expr>[^}]*)\s*}({(?P<text2>[^}]+)})?',
                     replacer, content)

    # replace $var inserts
    content = re.sub(r'\$([a-zA-Z0-9_]*)', r'" + str(\1) + "', content)
    # trim the line
    content = content.strip()

    # add a comment that shows where the line is generated at
    is_nonempty_line = bool(content.strip())
    if is_nonempty_line and line_idx is not None:
        if args['desugar_info'] == "comment":
            content += '" + sugar("{}", {}) + "'.format(os.path.basename(file), line_idx)
        elif args['desugar_info'] == "pragma":
            content = '#line %d \\"%s\\"\\n%s' % (line_idx, "../../" + file, content)
        else:
            content = '{}\\n'.format(content)

    return '{}{}generated_code += indent() + "{}{}"{}'.format(pre_indentation_mod, indentation, content, content_postfix, post_indentation_mod)


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


def translate_file_contents(file, genfile, code, indent_str="    ", prefix_lines="", add_lines=True):
    """Returns the code transformed into runnable Python code.
       Translated are #[generated_code, #=generator_expression and ${var} constructs."""
    has_translatable_comment = re.compile(r'^[ \t]*#[\[\{\}][ \t]*.*$')

    global indentation_level
    indentation_level = 0

    new_lines = prefix_lines.splitlines()
    new_lines += """
# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Eotvos Lorand University, Budapest, Hungary

# Autogenerated file (from {0}), do not modify directly.
# Generator: T4P4S (https://github.com/P4ELTE/t4p4s/)

global file_indentation_level
file_indentation_level = 0

# The last element is the innermost (current) style.
file_sugar_style = ['line_comment']


def indent():
    global file_indentation_level
    return '{1}' * file_indentation_level


class SugarStyle():
    def __init__(self, sugar):
        global file_sugar_style
        file_sugar_style.append(sugar)

    def __enter__(self):
        global file_sugar_style
        return file_sugar_style[-1]

    def __exit__(self, type, value, traceback):
        global file_sugar_style
        file_sugar_style.pop()


def sugar(file, line):
    import re
    global file_sugar_style
    sugar_file = re.sub("[.].*$", "", file)
    if file_sugar_style[-1] == 'line_comment':
        if file == "{2}":
            return ' // @' + str(line) + '\\n'
        return ' // ' + sugar_file + '@' + str(line) + '\\n'
    if file_sugar_style[-1] == 'inline_comment':
        if file == "{2}":
            return '\\n/*@' + str(line) + '*/'
        return '\\n/*' + sugar_file + '@' + str(line) + '*/'
    return ''


""".format(file, indent_str, os.path.basename(genfile)).splitlines()

    code_lines = enumerate(code.splitlines())
    code_lines = add_gen_in_def(code_lines, file)
    if add_lines:
        code_lines = add_empty_lines(code_lines)

    for idx, code_line in code_lines:
        new_line = code_line
        if has_translatable_comment.match(code_line):
            new_line = translate_line_with_insert(file, idx, code_line, indent_str)
        elif re.match(r'^[ \t]*#= .*$', code_line):
            new_line = re.sub(r'^([ \t]*)#=(.*)$', r'\1generated_code += str(\2)', code_line)

            if args['desugar_info'] == "comment":
                sugar_filename = os.path.basename(file)
                sugar_filename = re.sub("([.]sugar)?[.]py", "", sugar_filename)
                new_line += " # {}@{}".format(sugar_filename, idx)

        # won't mark empty lines and continued lines
        if new_line.strip() != "" and new_line.strip()[-1] != '\\':
            new_line += " ## " + file + " " + str(idx)

        new_lines.append(new_line)

    if indentation_level != 0:
        addError("Compiler", "Non-zero indentation level ({}) at end of file: {}".format(indentation_level, file))

    return '\n'.join(new_lines) + "\n"


def generate_code(file, genfile, localvars={}):
    """The file contains Python code with #[ inserts.
       The comments (which have to be indented properly)
       contain code to be output,
       their contents are collected in the variable generated_code.
       Inside the comments, refer to Python variables as ${variable_name}."""
    with open(file, "r") as orig_file:
        code = orig_file.read()
        code = translate_file_contents(file, genfile, code)

        if generate_code_files:
            write_file(genfile, code)

        if show_code:
            print(file + " -------------------------------------------------")
            print(code)
            print(file + " *************************************************")

        localvars['generated_code'] = ""

        try:
            exec(code, localvars, localvars)
        except Exception as exc:
            exc_type, exc, tb = sys.exc_info()
            if hasattr(exc, 'lineno'):
                addError("{}:{}:{}".format(genfile, exc.lineno, exc.offset), exc.msg)
            else:
                # TODO better error output
                print("Error: cannot compile file {} (compiled from {})".format(genfile, file), file=sys.stderr)
                if not pkgutil.find_loader('backtrace'):
                    print("Exception: {}".format(str(exc)), file=sys.stderr)
                    traceback.print_exc(file=sys.stderr)

                raise

        return re.sub(r'\n{3,}', '\n\n', localvars['generated_code'])


def generate_desugared_py():
    """Some Python source files also use the sugared syntax.
    The desugared files are generated here."""
    import glob
    for fromfile in glob.glob("src/utils/*.sugar.py"):
        tofile = re.sub("[.]sugar[.]py$", ".py", fromfile)
        with open(fromfile, "r") as orig_file:
            code = orig_file.read()
            prefix_lines = "generated_code = \"\"\n"
            code = translate_file_contents(fromfile, tofile, code, prefix_lines=prefix_lines, add_lines=False)

            write_file(tofile, code)


def get_hlir():
    global hlir
    if hlir is not None:
        return hlir
    hlir = load_hlir()
    return hlir


def generate_desugared_c(filename, filepath):
    hlir = get_hlir()

    genfile = join(args['desugared_path'], re.sub(r'\.([ch])\.py$', r'.\1.gen.py', filename))
    outfile = join(args['generated_dir'], re.sub(r'\.([ch])\.py$', r'.\1', filename))

    utils.misc.filename = filename
    utils.misc.filepath = filepath
    utils.misc.genfile = genfile
    utils.misc.outfile = outfile

    code = generate_code(filepath, genfile, {'hlir16': hlir})
    write_file(outfile, code)


def make_dirs():
    """Makes directories if they do not exist"""
    if not os.path.isdir(args['compiler_files_dir']):
        print("Compiler files path is missing", file=sys.stderr)
        sys.exit(1)

    if not os.path.isdir(args['desugared_path']):
        os.makedirs(args['desugared_path'])
        verbose_print(" GEN {0} (desugared compiler files)".format(args['desugared_path']))

    if not os.path.isdir(args['generated_dir']):
        os.makedirs(args['generated_dir'])
        verbose_print(" GEN {0} (generated files)".format(args['generated_dir']))

    if cache_dir_name and not os.path.isdir(cache_dir_name):
        os.mkdir(cache_dir_name)


def file_contains_exact_text(filename, text):
    """Returns True iff the file exists and it already contains the given text."""
    if not os.path.isfile(filename):
        return

    with open(filename, "r") as infile:
        intext = infile.read()
        return text == intext

    return False


def write_file(filename, text):
    """Writes the given text to the given file."""

    if file_contains_exact_text(filename, text):
        return

    with open(filename, "w") as genfile:
        genfile.write(text)


def init_args():
    """Parses the command line arguments and loads them
    into the global variable args."""
    parser = argparse.ArgumentParser(description='T4P4S compiler')
    parser.add_argument('p4_file', help='The source file')
    parser.add_argument('-v', '--p4v', help='Use P4-14 (default is P4-16)', required=False, choices=[16, 14], type=int, default=16)
    parser.add_argument('-p', '--p4c_path', help='P4C path', required=False)
    parser.add_argument('-c', '--compiler_files_dir', help='Source directory of the compiler\'s files', required=False, default=join("src", "hardware_indep"))
    parser.add_argument('-g', '--generated_dir', help='Output directory for hardware independent files', required=True)
    parser.add_argument('-desugared_path', help='Output directory for the compiler\'s files', required=False, default=join("build", "util", "gen"))
    parser.add_argument('-desugar_info', help='Markings in the generated source code', required=False, choices=["comment", "pragma", "none"], default="comment")
    parser.add_argument('-verbose', help='Verbosity', required=False, default=False, action='store_const', const=True)
    parser.add_argument('-beautify', help='Beautification', required=False, default=False, action='store_const', const=True)

    global args
    args = vars(parser.parse_args())


# TODO also reload if HLIR has changed
def is_file_fresh(filename):
    global p4time
    filetime = os.path.getmtime(filename)
    return p4time < filetime


def load_json_from_cache(base_p4_file):
    if not cache_dir_name:
        return None

    json_filename = base_p4_file + ".json"
    json_filepath = os.path.join(cache_dir_name, json_filename)

    if not os.path.isfile(json_filepath):
        return None
    if not is_file_fresh(json_filepath):
        return None

    verbose_print("JSON %s (cached)" % json_filename)
    return json_filepath


def get_pickled_hlir_file(base_p4_file):
    if not cache_dir_name:
        return None

    if not pkgutil.find_loader('dill'):
        return None

    pickle_filepath = os.path.join(cache_dir_name, base_p4_file + ".pickled")
    if not os.path.isfile(pickle_filepath):
        return None
    if not is_file_fresh(pickle_filepath):
        return None

    return pickle_filepath


def load_pickled_hlir(pickle_filepath):
    if pickle_filepath is None:
        return None

    if not pkgutil.find_loader('dill'):
        return None

    import dill
    import pickle

    # the standard recursion limit of 1000 can be too restrictive in more complex cases
    sys.setrecursionlimit(10000)

    with open(pickle_filepath, 'r') as inf:
        verbose_print("Found serialized HLIR in %s..." % pickle_filepath)
        return pickle.load(inf)


def save_pickled_hlir(hlir, base_p4_file):
    if not cache_dir_name:
        return None

    if not pkgutil.find_loader('dill'):
        return None

    import dill
    import pickle

    # the standard recursion limit of 1000 can be too restrictive in more complex cases
    sys.setrecursionlimit(10000)

    with open(os.path.join(cache_dir_name, base_p4_file + ".pickled"), 'w') as outf:
        pickled_hlir = pickle.dumps(hlir)
        outf.write(pickled_hlir)


def load_p4_file(filename):
    global hlir

    base_p4_file = os.path.basename(args['p4_file'])

    pickle_filepath = get_pickled_hlir_file(base_p4_file)
    hlir = load_pickled_hlir(pickle_filepath)
    if hlir is not None:
        return True

    to_load = load_json_from_cache(base_p4_file) or args['p4_file']

    hlir = load_p4(to_load, args['p4v'], args['p4c_path'], cache_dir_name)
    success = type(hlir) is not int

    if not success:
        return False

    verbose_print("HLIR " + filename)
    transform_hlir16(hlir)

    save_pickled_hlir(hlir, base_p4_file)

    return True


def check_file_exists(filename):
    if os.path.isfile(filename) is False:
        print("FILE NOT FOUND: %s" % filename, file=sys.stderr)
        sys.exit(1)

def check_file_extension(filename):
    _, ext = os.path.splitext(filename)
    if ext not in {'.p4', '.p4_14'}:
        print("EXTENSION NOT SUPPORTED: %s" % ext, file=sys.stderr)
        sys.exit(1)


def setup_backtrace():
    """If the backtrace module is installed, use it to print better tracebacks."""
    if not pkgutil.find_loader('backtrace'):
        return

    import backtrace

    backtrace.hook(
        reverse=True,
        align=True,
        strip_path=True,
        enable_on_envvar_only=False,
        on_tty=False,
        conservative=False,
        styles={})


def main():
    setup_backtrace()
    init_args()

    filename = args['p4_file']

    global p4time
    p4time = os.path.getmtime(filename)

    make_dirs()

    check_file_exists(filename)
    check_file_extension(filename)

    success = load_p4_file(filename)

    if not success:
        print("P4 compilation failed for file %s" % (os.path.basename(__file__)), file=sys.stderr)
        sys.exit(1)

    base = args['compiler_files_dir']
    exts = [".c.py", ".h.py"]

    for filename in (f for f in os.listdir(base) if isfile(join(base, f)) for ext in exts if f.endswith(ext)):
        verbose_print("  P4", filename)
        generate_desugared_py()
        generate_desugared_c(filename, join(base, filename))

    showErrors()
    showWarnings()

    global errors
    if len(errors) > 0:
        sys.exit(1)


if __name__ == '__main__':
    main()
