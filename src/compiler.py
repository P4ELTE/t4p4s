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
from utils.misc import *

from subprocess import call

import collections

from p4_hlir.main import HLIR
from p4_hlir.hlir import p4_parser
from p4_hlir.hlir import p4_tables

import re
import os
import sys
from os.path import isfile, join


# Possible values are "pragma", "comment" and anything else
generate_orig_lines = "comment"
generate_code_files = True
show_code = False
# The name of the beautifier program, or None.
c_beautifier = "clang-format-3.6"
c_beautifier_opts = "-style=llvm"


def translate_line_with_insert(file, line_idx, line):
    """Gets a line that contains an insert
       and transforms it to a Python code section."""
    # since Python code is generated, indentation has to be respected
    indentation = re.sub(r'^([ \t]*)#\[.*$', r'\1', line)

    # get the #[ part
    content = re.sub(r'^[ \t]*#\[([ \t]*[^\n]*)[ \t]*', r'\1', line)
    # escape sequences like \n may appear in #[ parts
    content = re.sub(r'\\', r'\\\\', content)
    # quotes may appear in #[ parts
    content = re.sub(r'"', r'\"', content)
    # replace ${var} and ${call()} inserts
    content = re.sub(r'\${([ \t\f\v]*)([^}]+)([ \t\f\v]*)}', r'" + str(\2) + "', content)

    # add a comment that shows where the line is generated at
    is_nonempty_line = bool(content.strip())
    if is_nonempty_line:
        if generate_orig_lines == "comment":
            content += "// sugar@%d" % (line_idx)
        if generate_orig_lines == "pragma":
            content = '#line %d \\"%s\\"\\n%s' % (line_idx, "../../" + file, content)

    return indentation + "generated_code += \"" + content + "\\n\""


def translate_file_contents(file, code):
    """Returns the code transformed into runnable Python code.
       Translated are #[generated_code and ${var} constructs."""
    has_translateable_comment = re.compile(r'^[ \t]*#\[[ \t]*.*$')

    new_lines = []
    code_lines = code.splitlines()
    for line_idx, code_line in enumerate(code_lines):
        new_line = code_line
        if has_translateable_comment.match(code_line):
            new_line = translate_line_with_insert(file, line_idx+1, code_line)

        new_lines.append(new_line)
    return '\n'.join(new_lines)

def generate_code(file, genfile, localvars={}):
    """The file contains Python code with #[ inserts.
       The comments (which have to be indented properly)
       contain code to be output,
       their contents are collected in the variable generated_code.
       Inside the comments, refer to Python variables as ${variable_name}."""
    with open(file, "r") as orig_file:
        code = orig_file.read()
        code = translate_file_contents(file, code)

        if generate_code_files:
            write_file(genfile, code)

        if show_code:
            print(file + " -------------------------------------------------")
            print(code)
            print(file + " *************************************************")

        localvars['generated_code'] = ""

        print "Desugaring %s..." % file

        exec(code, localvars, localvars)

        return localvars['generated_code']


def generate_all_in_dir(dir, gendir, outdir, hlir):

    for file in os.listdir(dir):
        full_file = join(dir, file)

        if not isfile(full_file):
            continue

        if not full_file.endswith(".c.py") and not full_file.endswith(".h.py"):
            continue

        genfile = join(gendir, re.sub(r'\.([ch])\.py$', r'.\1.desugared.py', file))
        code = generate_code(full_file, genfile, {'hlir': hlir})

        outfile = join(outdir, re.sub(r'\.([ch])\.py$', r'.\1', file))

        write_file(outfile, code)


def make_dirs(compiler_files_path, desugared_path, generated_path):
    """Makes directories if they do not exist"""
    if not os.path.isdir(compiler_files_path):
        print("Compiler files path is missing")
        sys.exit(1)

    if not os.path.isdir(desugared_path):
        os.makedirs(desugared_path)
        print("Generating path for desugared compiler files: {0}".format(desugared_path))

    if not os.path.isdir(generated_path):
        os.makedirs(generated_path)
        print("Generating path for generated files: {0}".format(generated_path))


def setup_paths():
    """Gets paths from the command line arguments (or defaults)
       and makes sure that they exist in the file system."""
    argidx_p4, argidx_genpath, argidx_srcpath = 1, 2, 3

    p4_path = sys.argv[argidx_p4]
    compiler_files_path = sys.argv[argidx_srcpath] if len(sys.argv) > argidx_srcpath else join("src", "hardware_indep")
    desugared_path = join("build", "util", "desugared_compiler")
    generated_path = sys.argv[argidx_genpath] if len(sys.argv) > argidx_genpath else join("build", "src_hardware_indep")

    make_dirs(compiler_files_path, desugared_path, generated_path)

    return p4_path, compiler_files_path, desugared_path, generated_path


def is_beautifiable(filename):
    return filename.endswith(".c") or filename.endswith(".h")


def write_file(filename, text):
    """Writes the given text to the given file with optional beautification."""
    with open(filename, "w") as genfile:
        genfile.write(text)

    if c_beautifier is not None and is_beautifiable(filename):
        try:
            call([c_beautifier, filename, "-i", c_beautifier_opts])
        except OSError:
            pass

def main():
    if len(sys.argv) <= 1:
        print("Usage: %s p4_file [compiler_files_dir] [generated_dir]" % (os.path.basename(__file__)))
        sys.exit(1)

    p4_path, compiler_files_path, desugared_path, generated_path = setup_paths()

    if os.path.isfile(p4_path) is False:
        print("FILE NOT FOUND: %s" % p4_path)
        sys.exit(1)

    hlir = HLIR(p4_path)
    build_hlir(hlir)

    generate_all_in_dir(compiler_files_path, desugared_path, generated_path, hlir)
   
    showErrors()
    showWarnings()

main()
