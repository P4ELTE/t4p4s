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
# Miscellaneous utility functions (not using HLIR)

from __future__ import print_function

import sys
import os

errors = []

warnings = []

def addError(where, msg):
    import traceback
    import itertools
    sb  = traceback.extract_stack()
    res = list(itertools.dropwhile(lambda (mod, line, fun, code): mod == 'src/compiler.py', sb))

    global errors
    msg = "Error while {}: {}\n".format(where, msg)
    errors += [msg] + traceback.format_list(res)


def addWarning(where, msg):
    global warnings
    warnings += ["WARNING: " + msg + " (While " + where + ").\n"]


def showErrors():
    global errors
    for e in errors:
        print(e, file=sys.stderr)


def showWarnings():
    global warnings
    for w in warnings:
        print(w, file=sys.stderr)


disable_hlir_messages = False


def build_hlir(hlir):
    """Builds the P4 internal representation, optionally disabling its output messages.
    Returns True if the compilation was successful."""
    if disable_hlir_messages:
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        sys.stdout = open(os.devnull, 'w')
        sys.stderr = open(os.devnull, 'w')

    success = hlir.build()

    if disable_hlir_messages:
        sys.stdout = old_stdout
        sys.stderr = old_stderr

    return success
