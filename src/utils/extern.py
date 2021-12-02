# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

def get_smem_name(smem_inst, prefix=[]):
    parts = prefix + smem_inst.name_parts.vec
    return f'SMEM{len(parts)}({",".join(parts)})'


def extern_has_unresolved_typepars(extern):
    """This apparently means that the extern is not used."""
    tparams = extern.typeParameters.parameters
    return extern.name == 'Register'
    # return any(tpar.urtype.node_type in ('Type_Var', 'Type_Name') for tpar in tparams if tpar is not None if tpar.urtype is not None)


def extern_is_unused(extern, repr):
    return repr is None or repr.node_type in ('Type_Name') or extern_has_unresolved_typepars(extern)


def get_extern_repr(extern):
    ctor_params = extern.constructors[0].urtype.parameters.parameters
    if len(ctor_params) == 0:
        return None
    repr = ctor_params[0].urtype

    if extern_is_unused(extern, repr):
        return None

    return repr


def extern_has_tuple_params(mname, parinfos):
    return any(partype.urtype.node_type == 'Type_Struct' for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if argvalue != None)
