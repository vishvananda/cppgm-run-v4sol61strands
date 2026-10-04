#!/usr/bin/env python3
"""Frozen stage-supported expressions; paired warm pinned 3-run PMU measurements.

Hosts receive #if envelopes around exactly the same expressions and validate
all expected values. They do additional directive work; this is not a claim
that PA3 consumes a hosted translation unit or compiles native executables.
"""
import argparse
import hashlib
import json
import os
import pathlib
import re
import shutil
import statistics
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('artifact', type=pathlib.Path)
p.add_argument('--label', default='final')
p.add_argument('--against', help='Include frozen ppexpr/cursor binaries from this label')
a = p.parse_args()
A = a.artifact.resolve(); A.mkdir(parents=True, exist_ok=True)
cpu = min(os.sched_getaffinity(0))

def sha(path):
    h = hashlib.sha256()
    with pathlib.Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1024*1024), b''): h.update(block)
    return h.hexdigest()

def run(cmd, **kw):
    return subprocess.run([str(x) for x in cmd], check=True, **kw)

workloads = ('arithmetic', 'lazy', 'chain', 'nested', 'identifiers')
for name in workloads:
    with (A/(name+'.expr')).open('w') as expr, (A/(name+'.cpp')).open('w') as host, (A/(name+'.expected')).open('w') as expected:
        if name == 'arithmetic':
            rows = ((f'({i%997+1}*731+42-17)/3 ^ 15', str(((i%997+1)*731+25)//3 ^ 15)) for i in range(150000))
        elif name == 'lazy':
            rows = ((f'(0 && (9/0)) || (1 ? ({i%991}ull * 13ull + 7ull) : (1ull << 64))', '1') for i in range(120000))
        elif name == 'chain': rows = [('+'.join(['1']*300000), '300000')]
        elif name == 'nested': rows = [('('*128+'1 ? -7 : 9ull'+')'*128, '18446744073709551609u')]*15000
        else: rows = ((f'unknown_{i} + (unknown_{i} || unknown_{i+1})', '0') for i in range(120000))
        for e, result in rows:
            expr.write(e+'\n'); expected.write(result+'\n')
            constant = result[:-1]+'ull' if result.endswith('u') else result+'ll'
            host.write(f'#if ({e}) != {constant}\n#error value_mismatch\n#endif\n{result}\n')
        expected.write('eof\n'); host.write('eof\n')

flags = ['-std=c++11', '-O3', '-g', '-I'+str(ROOT/'dev/src')]
sources = list((ROOT/'dev/src/preprocess/lex').glob('*.cpp')) + list((ROOT/'dev/src/preprocess/post').glob('*.cpp'))
sources += list((ROOT/'dev/src/preprocess/expr').glob('*.cpp'))
for variant, entry in [('cursor', ROOT/'student.tests/pa3/bench.cpp'), ('ppexpr', ROOT/'dev/ppexpr.cpp')]:
    run(['g++', *flags, entry, *sources, '-o', A/(variant+'-'+a.label)])
variants = {'cursor': [A/('cursor-'+a.label)], 'ppexpr': [A/('ppexpr-'+a.label)],
            'gcc': ['g++', '-std=c++11', '-E', '-P', '-x', 'c++', '-'],
            'clang': ['clang++', '-std=c++11', '-E', '-P', '-x', 'c++', '-'],
            'reference': [ROOT/'reference-binaries/ppexpr']}
if a.against:
    for variant in ('cursor', 'ppexpr'):
        variants[variant+'-baseline'] = [A/(variant+'-'+a.against)]
manifest = {'cpu': cpu, 'flags': flags, 'warmups': 1, 'repeats': 3,
            'cpu_model': next(s for s in pathlib.Path('/proc/cpuinfo').read_text().splitlines() if s.startswith('model name')),
            'environment': {'CPPGM_EXPR_METRICS': '1'},
            'commands': {k: list(map(str,v)) for k,v in variants.items()},
            'inputs': {n: {x: {'bytes': (A/(n+'.'+x)).stat().st_size, 'sha256': sha(A/(n+'.'+x))}
                           for x in ('expr', 'cpp', 'expected')} for n in workloads},
            'binary_hashes': {k: sha(shutil.which(str(v[0]))) for k,v in variants.items()},
            'hosts': {c: subprocess.check_output([c, '--version'], text=True).splitlines()[0] for c in ('g++', 'clang++')},
            'comparison': 'Identical expressions; host #if envelopes verify each value and emit matching output. Cursor does not render. No generated executable.'}
manifest['cc1plus_hash'] = sha(subprocess.check_output(['g++', '-print-prog-name=cc1plus'], text=True).strip())
(A/(a.label+'-manifest.json')).write_text(json.dumps(manifest, indent=2))
observations = []; parity = {}
for name in workloads:
    parity[name] = {}
    checksum = 0
    for result in (A/(name+'.expected')).read_text().splitlines()[:-1]:
        u = result.endswith('u')
        bits = int(result[:-1] if u else result) % (1<<64)
        checksum = ((checksum ^ bits ^ u) * 1099511628211) % (1<<64)
    for variant, cmd in variants.items():
        input_path = A/(name+('.cpp' if variant in ('gcc', 'clang') else '.expr'))
        output = A/f'{a.label}-{name}-{variant}.output'
        with input_path.open('rb') as inp, output.open('wb') as out:
            result = run(cmd, stdin=inp, stdout=out, stderr=subprocess.PIPE)
        if variant.startswith('cursor'):
            assert int(re.search(rb'checksum=(\d+)', result.stderr).group(1)) == checksum, (name, variant)
        if not variant.startswith('cursor'):
            assert sha(output) == sha(A/(name+'.expected')), (name, variant)
        parity[name][variant] = {'sha256': sha(output), 'expected_match': not variant.startswith('cursor'), 'checksum': checksum if variant.startswith('cursor') else None}
        output.unlink()
        with input_path.open('rb') as inp:
            run(['taskset', '-c', cpu, *cmd], stdin=inp, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for repeat in range(3):
        for variant, cmd in variants.items():
            stem = A/f'{a.label}-{name}-{variant}-{repeat}'
            input_path = A/(name+('.cpp' if variant in ('gcc', 'clang') else '.expr'))
            with input_path.open('rb') as inp, pathlib.Path(str(stem)+'.telemetry').open('wb') as err:
                run(['perf', 'stat', '-x', ';', '-o', str(stem)+'.perf', '-e', '{instructions:u,cycles:u}',
                     '--', 'taskset', '-c', cpu, '/usr/bin/time', '-f', '%e;%U;%S;%M', '-o', str(stem)+'.time',
                     *cmd], stdin=inp, stdout=subprocess.DEVNULL, stderr=err,
                    env={**os.environ, 'CPPGM_EXPR_METRICS': '1'})
            if variant.startswith('cursor'):
                assert int(re.search(r'checksum=(\d+)', pathlib.Path(str(stem)+'.telemetry').read_text()).group(1)) == checksum
            counts = {}
            for line in pathlib.Path(str(stem)+'.perf').read_text().splitlines():
                f = line.split(';')
                if len(f)>4 and f[2] in ('instructions:u', 'cycles:u'):
                    assert f[0].strip().isdigit() and float(f[4]) >= 95, line
                    counts[f[2]] = int(f[0])
            assert len(counts) == 2, stem
            wall,user,system,rss = pathlib.Path(str(stem)+'.time').read_text().strip().split(';')
            observations.append({'workload': name, 'variant': variant, 'repeat': repeat,
                'instructions': counts['instructions:u'], 'cycles': counts['cycles:u'],
                'ipc': counts['instructions:u']/counts['cycles:u'], 'wall': float(wall),
                'user': float(user), 'system': float(system), 'rss_kb': int(rss)})
            (A/(a.label+'-observations.json')).write_text(json.dumps(observations, indent=2))
summary = {}
for name in workloads:
    summary[name] = {}
    for variant in variants:
        rows = [r for r in observations if r['workload']==name and r['variant']==variant]
        summary[name][variant] = {k: {'median': statistics.median(r[k] for r in rows),
                                     'min': min(r[k] for r in rows), 'max': max(r[k] for r in rows)}
                                 for k in ('instructions', 'cycles', 'ipc', 'wall', 'user', 'system', 'rss_kb')}
(A/(a.label+'-parity.json')).write_text(json.dumps(parity, indent=2))
(A/(a.label+'-summary.json')).write_text(json.dumps(summary, indent=2))
print(json.dumps(summary, indent=2))
