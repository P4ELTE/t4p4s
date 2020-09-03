#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Eotvos Lorand University, Budapest, Hungary

import hlir16.hlir
from hlir16.hlir_attrs import set_additional_attrs
from utils.misc import *
from transform_hlir import *

from subprocess import call

import os
import os.path
import sys
import pkgutil
import importlib

import bz2
import pickle

import compiler_common


# TODO also reload if (the relevant part of) the HLIR generator code has changed
def is_file_fresh(filename):
    global p4time
    filetime = os.path.getmtime(filename)
    return p4time < filetime


def is_cache_file_loadable(path):
    return path is not None and os.path.isfile(path) and is_file_fresh(path)


def import_modules(required_modules):
    for modname in required_modules:
        if not pkgutil.find_loader(modname):
            print(f"missing {modname}")
            return None

    return [importlib.import_module(modname) for modname in required_modules]


def load_cache(filename, is_compressed, required_modules, loader):
    if not is_cache_file_loadable(filename):
        return None

    if import_modules(required_modules) is None:
        return None

    if is_compressed:
        with open(filename, 'rb') as cache_file:
            return loader(cache_file, None)


def write_cache(cache, required_modules, saver, data):
    if import_modules(required_modules) is None:
        return

    with open(cache, 'wb') as cache_file:
        cache_file.write(saver(data))


def p4_to_json(files, arg):
    p4_filename, json_filename = arg
    return hlir16.hlir.p4_to_json(p4_filename, json_filename, args['p4v'], args['p4c_path'], args['p4opt'])


def load_simdjson(file, data):
    import simdjson
    if file is not None:
        return simdjson.load(file)

    with open(data, 'r') as f:
        return simdjson.load(f)


def load_ujson(file, data):
    import ujson
    if file is not None:
        return ujson.load(file)

    with open(data, 'r') as f:
        return ujson.load(f)


def load_json(file, data):
    if file is not None:
        return json.load(file)

    with open(data, 'r') as f:
        return json.load(f)


class RecursionLimit():
    """Temporarily increase the standard recursion limit."""
    def __init__(self, limit):
        self.limit = limit
    def __enter__(self):
        self.old_limit = sys.getrecursionlimit()
        sys.setrecursionlimit(self.limit)
    def __exit__(self, type, value, traceback):
        sys.setrecursionlimit(self.old_limit)


def restrict_stages(stages):
    for stage in stages:
        if 'dependency' not in stage:
            yield stage
            continue

        if not os.path.isfile(stage['filename']):
            return

        filetime = os.path.getctime(stage['filename'])
        for dep in stage['dependency']:
            if filetime < os.path.getctime(dep):
                args['verbose'] and print(f"Dependency file {dep} is newer than {stage['filename']}, stage {stage['name']} has to be redone")
                return

        yield stage


def load_latest_stage_from_cache(stages):
    for stage_idx, stage in reversed(list(enumerate(restrict_stages(stages)))):
        for attrname, attrdefault in [('filename', None), ('is_valid', lambda x: True), ('msgfmt', ""), ('saver', None), ('is_compressed', stage_idx != 0)]:
            if attrname not in stage:
                stage[attrname] = attrdefault

        for required_modules, loader in stage['loaders']:
            loaded = load_cache(stage['filename'], stage['is_compressed'], required_modules, loader)
            if loaded and stage['is_valid'](loaded):
                args['verbose'] and print(stage['msgfmt'].format(stage['filename']))
                return stage_idx + 1, loaded

    return 0, None


def continue_stages(stages, stage_idx, data):
    for curr_stage_idx, stage in list(enumerate(stages))[stage_idx:]:
        compiler_common.current_compilation = { 'from': f"(cached) {stage['filename']}", 'to': "(generated content)", 'stage': stage }

        new_data = None
        for required_modules, loader in stage['loaders']:
            if import_modules(required_modules) is None:
                continue

            new_data = loader(None, data)

            if new_data is not None:
                break

        if new_data is None:
            return None

        data = new_data

        if stage['saver'] is not None:
            required_modules, save_data = stage['saver']
            write_cache(stage['filename'], required_modules, save_data, data)
    return data

    compiler_common.current_compilation = None

def load_hlir(filename, cache_dir_name):
    p4cache = os.path.join(cache_dir_name, os.path.basename(filename))

    stages = [
        stage_p4_to_json_file(filename, p4cache),
        stage_load_json(filename, p4cache),
        stage_json_to_hlir(filename, p4cache),
        stage_hlir_add_attributes(filename, p4cache),
        stage_hlir_transform(filename, p4cache),
        ]

    stage_idx, data = load_latest_stage_from_cache(stages)
    if stage_idx == 0:
        args['verbose'] and print(stages[0]['msgfmt'].format(filename))
        data = (filename, f"{p4cache}.json.cached")
    return continue_stages(stages, stage_idx, data)


def cache_loader(no_cache_loader):
    return ([], lambda file, data: pickle.load(file) if file is not None else no_cache_loader(data) if data is not None else None)

def cache_saver():
    return ([], lambda data: pickle.dumps(data, protocol=pickle.HIGHEST_PROTOCOL))

def stage_p4_to_json_file(filename, p4cache):
    return {
        'name': 'stage_p4_to_json_file',
        'filename': filename,
        'msgfmt': "HLIR (uncached) {}",
        'dependency': [filename],
        # The P4C compiler creates the JSON file while "loading".
        'loaders': [([], p4_to_json)],
    }

def stage_load_json(filename, p4cache):
    return {
        'name': 'stage_load_json',
        'msgfmt': "HLIR (cached: JSON) {}",
        'filename': f"{p4cache}.json.cached",
        'loaders': [(['simdjson'], load_simdjson), (['ujson'], load_ujson), ([], load_json)],
        # This detects if the loaded JSON does not contain "main".
        'is_valid': lambda json_root: json_root['Node_ID'] is not None,
    }

def stage_json_to_hlir(filename, p4cache):
    return {
        'name': 'stage_json_to_hlir',
        'msgfmt': "HLIR (cached: stage json_to_hlir) {}",
        'filename': f"{p4cache}.hlir.cached",
        'loaders': [cache_loader(lambda json_root: hlir16.hlir.walk_json_from_top(json_root))],
        'dependency': ["src/compiler.py", "src/compiler_load_p4.py", "src/compiler_exception_handling.py", "src/hlir16/hlir.py", "src/hlir16/hlir_attrs.py", "src/hlir16/p4node.py"],
        'saver': cache_saver(),
    }

def stage_hlir_add_attributes(filename, p4cache):
    return {
        'name': 'stage_hlir_add_attributes',
        'msgfmt': "HLIR (cached: stage hlir_add_attributes) {}",
        'filename': f"{p4cache}.hlir.attributed.cached",
        'loaders': [cache_loader(lambda hlir: set_additional_attrs(hlir, args['p4v']))],
        'saver': cache_saver(),
    }

def stage_hlir_transform(filename, p4cache):
    return {
        'name': 'stage_hlir_transform',
        'msgfmt': "HLIR (cached) {}",
        'filename': f"{p4cache}.hlir.transformed.cached",
        'loaders': [cache_loader(transform_hlir)],
        'saver': cache_saver(),
    }


def check_file_exists(filename):
    if os.path.isfile(filename) is False:
        print("FILE NOT FOUND: %s" % filename, file=sys.stderr)
        sys.exit(1)

def check_file_extension(filename):
    _, ext = os.path.splitext(filename)
    if ext not in {'.p4', '.p4_14'}:
        print("EXTENSION NOT SUPPORTED: %s" % ext, file=sys.stderr)
        sys.exit(1)


def load_from_p4(compiler_args, cache_dir_name):
    global args
    args = compiler_args

    filename = args['p4_file']

    global p4time
    p4time = os.path.getmtime(filename)

    check_file_exists(filename)
    check_file_extension(filename)

    with RecursionLimit(10000) as recursion_limit:
        hlir = load_hlir(filename, cache_dir_name)

        if hlir is None:
            print(f"P4 compilation failed for file {os.path.basename(__file__)}", file=sys.stderr)
            sys.exit(1)

        return hlir
