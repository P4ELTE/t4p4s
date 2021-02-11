# SPDX-License-Identifier: Apache-2.0
# Copyright 2020 Eotvos Lorand University, Budapest, Hungary

# this allows proper filename display in error messages
current_compilation = None
type_env = {}

################################################################################

file_indent_str = "    "
file_indentation_level = 0

# The last element is the innermost (current) style.
file_sugar_style = ['line_comment']

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


################################################################################

class types:
    def __init__(self, new_type_env):
        global type_env
        self.env_vars = set()
        for v in new_type_env:
            if v in type_env:
                addWarning('adding a type environment', f'variable {v} is already bound to type {type_env[v]}')
            else:
                self.env_vars.add(v)
                type_env[v] = new_type_env[v]

    def __enter__(self):
        global type_env
        return type_env

    def __exit__(self, type, value, traceback):
        global type_env
        for v in self.env_vars:
            del type_env[v]

    def env():
        global type_env
        return type_env

################################################################################

def with_base(number, base):
    if base == 16:
        return f"0x{number:x}"
    if base == 2:
        return f"0b{number:b}"

    return f"{number}"

def split_join_text(text, n, prefix, joiner):
    """Splits the text into chunks that are n characters long, then joins them up again."""
    return joiner.join(f"{prefix}{text[i:i+n]}" for i in range(0, len(text), n))

def make_const(e):
    byte_width = (e.type.size+7)//8
    const_str = f'{{:0{2 * byte_width}x}}'.format(e.value)

    hex_content = split_join_text(const_str, 2, "0x", ", ")
    name = split_join_text(const_str, 4, "", "_")

    return name, hex_content

################################################################################

def pp_type_16(t):
    """Pretty print P4_16 type"""
    if t.node_type == 'Type_Boolean':
        return 'bool'
    elif t.node_type == 'Type_Bits':
        return f'int<{t.size}>' if t.isSigned else f'bit<{t.size}>'
    else:
        return f'{t}'


################################################################################

def resolve_reference(e):
    if 'field_ref' in e:
        hdr = e.expr.header_ref
        fld = e.field_ref
        return (hdr, fld)
    else:
        return e

def is_subsequent(range1, range2):
    h1, f1 = range1
    h2, f2 = range2
    fs = h1.urtype.fields.vec
    return h1 == h2 and fs.index(f1) + 1 == fs.index(f2)

def groupby(xs, fun):
    """Groups the elements of a list.
    The upcoming element will be grouped if
    fun(last element of the group, upcoming) evaluates to true."""
    if not xs:
        yield []
        return

    elems = []
    for x in xs:
        if elems == []:
            elems = [x]
        elif not fun(elems[-1], x):
            yield elems
            elems = [x]
        else:
            elems.append(x)

    if elems != []:
        yield elems

def group_references(refs):
    for xs in groupby(refs, lambda x1, x2: isinstance(x1, tuple) and isinstance(x2, tuple) and is_subsequent(x1, x2)):
        if xs == [None]:
            # TODO investigate this case further
            continue

        yield (xs[0][0], [fld for hdr, fld in xs])

def fldid(h, f):
    inst_type_name = 'all_metadatas' if h.urtype.is_metadata else h.ref.name if h.node_type == 'PathExpression' else h.name
    return f'field_{inst_type_name}_{f.name}'
def fldid2(h, f):
    return f'{h.id},{f.id}'

# ################################################################################

def unique_everseen(items):
    """Returns only the first occurrence of the items in a list.
    Equivalent to unique_everseen from the package more-itertools."""
    from collections import OrderedDict
    return list(OrderedDict.fromkeys(items))

################################################################################

enclosing_control = None

pre_statement_buffer = ""
post_statement_buffer = ""

def prepend_statement(s):
    global pre_statement_buffer
    pre_statement_buffer += f"\n{s}"

def append_statement(s):
    global post_statement_buffer
    post_statement_buffer += f"{s}\n"

def statement_buffer_value():
    global pre_statement_buffer
    global post_statement_buffer
    ret = (pre_statement_buffer, post_statement_buffer)
    pre_statement_buffer = ""
    post_statement_buffer = ""
    return ret


################################################################################

def is_control_local_var(var_name):
    global enclosing_control

    def get_locals(node):
        if node.node_type == 'P4Parser':  return node.parserLocals
        if node.node_type == 'P4Control': return node.controlLocals
        return []

    return enclosing_control is not None and [] != [cl for cl in get_locals(enclosing_control) if cl.name == var_name]

################################################################################

var_name_counter = 0
generated_var_names = set()

def generate_var_name(var_name_part, var_id = None):
    global var_name_counter
    global generated_var_names

    var_name_counter += 1

    var_name = f"{var_name_part}_{var_name_counter:04d}"
    if var_id is not None:
        simpler = f"{var_name_part}_{var_id}"
        var_name = simpler
        if simpler in generated_var_names:
            var_name = f"{simpler}_{var_name_counter}"

    generated_var_names.add(var_name)

    return var_name

# ################################################################################

def dlog(num, base=2):
    """Returns the discrete logarithm of num.
    For the standard base 2, this is the number of bits required to store the range 0..num."""
    return [n for n in range(32) if num < base**n][0]
