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

import sys

def bswap16(x):
    return (((x & 0xff00) >> 8) |
            ((x & 0x00ff) << 8))

def bswap32(x):
    return (((x & 0xff000000) >> 24) |
            ((x & 0x00ff0000) >>  8) |
            ((x & 0x0000ff00) <<  8) |
            ((x & 0x000000ff) << 24))
    
def bswap64(x):
    return (((x & 0xff00000000000000) >> 56) |
            ((x & 0x00ff000000000000) >> 40) |
            ((x & 0x0000ff0000000000) >> 24) |
            ((x & 0x000000ff00000000) >>  8) |
            ((x & 0x00000000ff000000) <<  8) |
            ((x & 0x0000000000ff0000) << 24) |
            ((x & 0x000000000000ff00) << 40) |
            ((x & 0x00000000000000ff) << 56))
    
def cpu_to_be_16(x):
    if sys.byteorder == "little":
        return bswap16(x)
    else:
        return x
    
def cpu_to_be_32(x):
    if sys.byteorder == "little":
        return bswap32(x)
    else:
        return x
    
def cpu_to_be_64(x):
    if sys.byteorder == "little":
        return bswap64(x)
    else:
        return x
    
def cpu_to_le_16(x):
    if sys.byteorder == "big":
        return bswap16(x)
    else:
        return x
    
def cpu_to_le_32(x):
    if sys.byteorder == "big":
        return bswap32(x)
    else:
        return x
    
def cpu_to_le_64(x):
    if sys.byteorder == "big":
        return bswap64(x)
    else:
        return x
    
def be_to_cpu_16(x):
    if sys.byteorder == "little":
        return bswap16(x)
    else:
        return x
    
def be_to_cpu_32(x):
    if sys.byteorder == "little":
        return bswap32(x)
    else:
        return x
    
def be_to_cpu_64(x):
    if sys.byteorder == "little":
        return bswap64(x)
    else:
        return x
    
def le_to_cpu_16(x):
    if sys.byteorder == "big":
        return bswap16(x)
    else:
        return x
    
def le_to_cpu_32(x):
    if sys.byteorder == "big":
        return bswap32(x)
    else:
        return x
    
def le_to_cpu_64(x):
    if sys.byteorder == "big":
        return bswap64(x)
    else:
        return x
        
def cpu_to_be(x, bits):
    if bits <= 16:
        return cpu_to_be_16(x)
    elif bits <= 32:
        return cpu_to_be_32(x)
    else:
        return cpu_to_be_64(x)
        
def cpu_to_le(x, bits):
    if bits <= 16:
        return cpu_to_le_16(x)
    elif bits <= 32:
        return cpu_to_le_32(x)
    else:
        return cpu_to_le_64(x)
        
def be_to_cpu(x, bits):
    if bits <= 16:
        return be_to_cpu_16(x)
    elif bits <= 32:
        return be_to_cpu_32(x)
    else:
        return be_to_cpu_64(x)
        
def le_to_cpu(x, bits):
    if bits <= 16:
        return le_to_cpu_16(x)
    elif bits <= 32:
        return le_to_cpu_32(x)
    else:
        return le_to_cpu_64(x)

