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
from p4_hlir.frontend.ast import *

################################################################################

def Integer(value): return P4Integer('', 42, value)
def FieldRefExpression(headerref, fieldname): return P4FieldRefExpression('', 42, headerref, str(fieldname))
def RefExpression(name): return P4RefExpression('', 42, str(name))
def ParserImmediateReturn(next_state): return P4ParserImmediateReturn('', 42, next_state)
def ParserSelectReturn(select, cases): return P4ParserSelectReturn('', 42, select, cases)
def ParserFunction(name, ops, ret): return P4ParserFunction('', 42, str(name), ops, ret)
def ParserSelectDefaultCase(next_state): return P4ParserSelectDefaultCase('', 42, next_state)
def ParserSelectCase(case, next_state): return P4ParserSelectCase('', 42, case, next_state)
def Table(name, action_spec, action_prof, reads, min_size, max_size, size, timeout): return P4Table('', 42, str(name), action_spec, action_prof, reads, min_size, max_size, size, timeout)
def ParserExtract(header): return P4ParserExtract('', 42, header)
def TableFieldMatch(field, typ): return P4TableFieldMatch('', 42, field, typ)
def ControlFunction(name, body): return P4ControlFunction('', 42, str(name), body)
def HeaderType(name, layout, length, max_length): return P4HeaderType('', 42, str(name), layout, length, max_length)
def HeaderInstanceRegular(header_type, name): return P4HeaderInstanceRegular('', 42, header_type, str(name))
def HeaderInstanceMetadata(header_type, name): return P4HeaderInstanceMetadata('', 42, header_type, str(name))
def ActionCall(action): return P4ActionCall('', 42, action)
def ActionCallWP(action, parameters): return P4ActionCall('', 42, action, parameters)
def ActionFunction(name, params, body): return P4ActionFunction('', 42,  str(name), params, body)
def BinaryExpression(op, left, right): return P4BinaryExpression('', 42, str(op), left, right)
def ControlFunction(name, body): return P4ControlFunction('', 42, name, body)
def ControlFunctionApply(name): return P4ControlFunctionApply('', 42, name)
def ControlFunctionApplyAndSelect(name, cases): return P4ControlFunctionApplyAndSelect('', 42, name, cases)
def ControlFunctionApplyActionCase(case, next): return P4ControlFunctionApplyActionCase('', 42, case, next)

