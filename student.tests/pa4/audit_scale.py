#!/usr/bin/env python3
"""Freeze supplemental PA4 work-model controls; common host/reference inputs."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

A=Path(sys.argv[1]).resolve()
A.mkdir(parents=True,exist_ok=True)
inputs={}
for count in (21000, 85000, 340000):
    name=f'conditional{count}'
    text='#define FLAG 7\n'+('#if defined(FLAG) && FLAG==7 && (1 || 1/0)\ndone\n#else\n#error fail\n#endif\n'*count)
    (A/(name+'.cc')).write_text(text)
    inputs[name]={'path':str(A/(name+'.cc')), 'expected':count}
for count in (1000, 4000, 16000):
    name=f'replacement{count}'
    text='#define MANY '+'done '*count+'\nMANY\n'*20
    (A/(name+'.cc')).write_text(text)
    inputs[name]={'path':str(A/(name+'.cc')), 'expected':count*20}
for count in (2000, 8000):
    name=f'headers{count}'
    directory=A/name
    directory.mkdir(exist_ok=True)
    for i in range(count):
        # Distinct content avoids GCC's identical-file pragma-once coalescing.
        (directory/f'h{i}.hh').write_text(f'// unique header {i}\n#pragma once\n#define VAL done\nVAL\n')
    text=''.join(f'#include "{name}/h{i}.hh"\n' for i in range(count))
    text+=''.join(f'#include "{name}/h{i}.hh"\n' for i in range(count))
    (A/(name+'.cc')).write_text(text)
    inputs[name]={'path':str(A/(name+'.cc')), 'expected':count}
for v in inputs.values():
    p=Path(v['path']);v['bytes']=p.stat().st_size;v['sha256']=hashlib.sha256(p.read_bytes()).hexdigest()
(A/'inputs.json').write_text(json.dumps(inputs,indent=2))
print(A/'inputs.json')
