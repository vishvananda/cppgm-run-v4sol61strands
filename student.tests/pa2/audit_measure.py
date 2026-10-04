#!/usr/bin/env python3
"""Pinned slowest-fixture diagnostic, including expected host rejection evidence."""
import hashlib
import json
import os
import pathlib
import statistics
import subprocess
import argparse
import shutil

ROOT = pathlib.Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('artifact', type=pathlib.Path)
p.add_argument('--label', default='slowest')
p.add_argument('--tool-label', default='audit')
a = p.parse_args()
A = a.artifact.resolve(); A.mkdir(parents=True, exist_ok=True)
P = A.parent/'pa2-perf'
source = ROOT/'pa2/tests/700-hard-string-concat.t'
cpu = min(os.sched_getaffinity(0))
commands = {'cursor': [str(P/('cursor-'+a.tool_label))],
            'posttoken': [str(P/('posttoken-'+a.tool_label))],
            'reference': [str(ROOT/'reference-binaries/posttoken')],
            'gcc-E': ['g++', '-std=c++11', '-trigraphs', '-E', '-P', '-x', 'c++', '-'],
            'clang-E': ['clang++', '-std=c++11', '-trigraphs', '-E', '-P', '-x', 'c++', '-']}
env = {**os.environ, 'CPPGM_POST_METRICS': '1'}
manifest = {'input': str(source), 'sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
            'binaries': {key: hashlib.sha256(pathlib.Path(shutil.which(cmd[0])).read_bytes()).hexdigest()
                         for key, cmd in commands.items()},
            'cpu_model': next(line.strip() for line in pathlib.Path('/proc/cpuinfo').read_text().splitlines()
                              if line.startswith('model name')),
            'hosts': {host: subprocess.check_output([host, '--version'], text=True).splitlines()[0]
                      for host in ('g++', 'clang++')},
            'commands': commands, 'cpu': cpu, 'warmups': 1, 'runs': 3,
            'environment': {'CPPGM_POST_METRICS': '1'},
            'note': 'GCC/Clang preprocessing only; fixture intentionally includes invalid tokens. Clang rejects some numeric suffixes in lexing: timings are diagnostic, not equivalent supported work.'}
(A/(a.label+'-manifest.json')).write_text(json.dumps(manifest, indent=2))
observations = []
for variant, cmd in commands.items():
    with source.open('rb') as inp, (A/(a.label+'-'+variant+'-warm.stderr')).open('wb') as err:
        r = subprocess.run(['taskset', '-c', str(cpu), *cmd], stdin=inp, stdout=subprocess.DEVNULL, stderr=err, env=env)
        assert r.returncode == (1 if variant == 'clang-E' else 0), (variant, r.returncode)
for n in range(3):
    for variant, cmd in commands.items():
        stem = A/f'{a.label}-{variant}-{n}'
        with source.open('rb') as inp, pathlib.Path(str(stem)+'.telemetry').open('wb') as err:
            r = subprocess.run(['perf', 'stat', '-x', ';', '-o', str(stem)+'.perf',
                                '-e', '{instructions:u,cycles:u}', '--', 'taskset', '-c', str(cpu),
                                '/usr/bin/time', '-f', '%e %U %S %M', '-o', str(stem)+'.time', *cmd],
                               stdin=inp, stdout=subprocess.DEVNULL, stderr=err, env=env)
            assert r.returncode == (1 if variant == 'clang-E' else 0)
        counts = {}
        for line in pathlib.Path(str(stem)+'.perf').read_text().splitlines():
            f = line.split(';')
            if len(f) > 4 and f[2] in ('instructions:u', 'cycles:u'):
                assert float(f[4]) >= 99.0, line
                counts[f[2]] = float(f[0])
        wall, user, system, rss = map(float, pathlib.Path(str(stem)+'.time').read_text().splitlines()[-1].split())
        observations.append({'variant': variant, 'run': n, 'exit': r.returncode,
                             'instructions': counts['instructions:u'], 'cycles': counts['cycles:u'],
                             'ipc': counts['instructions:u']/counts['cycles:u'],
                             'wall': wall, 'user': user, 'system': system, 'rss_kb': rss})
(A/(a.label+'-observations.json')).write_text(json.dumps(observations, indent=2))
summary = {}
for v in commands:
    rows = [r for r in observations if r['variant'] == v]
    summary[v] = {key: {'median': statistics.median(r[key] for r in rows),
                        'min': min(r[key] for r in rows), 'max': max(r[key] for r in rows)}
                  for key in ('instructions', 'cycles', 'ipc', 'wall', 'user', 'system', 'rss_kb')}
(A/(a.label+'-summary.json')).write_text(json.dumps(summary, indent=2))
print(json.dumps(summary, indent=2))
# Repeat independent executions for enough attributed samples, not measurements.
cmd = ['bash', '-c', 'for ((i=0;i<30;i++)); do "$1" < "$2" > /dev/null || exit; done',
       'profile', commands['posttoken'][0], str(source)]
with (A/(a.label+'-record.log')).open('w') as err:
    subprocess.run(['perf', 'record', '-e', 'cycles:u', '-F', '99', '--call-graph', 'dwarf,8192',
                    '-o', A/(a.label+'.data'), '--', 'taskset', '-c', str(cpu), *cmd], check=True, stderr=err)
with (A/(a.label+'-profile.txt')).open('w') as out:
    subprocess.run(['perf', 'report', '--stdio', '--no-children', '-i', A/(a.label+'.data')], check=True, stdout=out, stderr=out)
