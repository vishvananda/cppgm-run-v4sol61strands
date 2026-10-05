#!/usr/bin/env python3
"""Whole-stage slow/error/scale audit; frozen paired PMU runs and attribution."""
import hashlib
import json
import os
from pathlib import Path
import re
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[2]
BASE = Path(os.environ['RALPH_ARTIFACT_DIR'])
LABEL = os.environ.get('PA3_AUDIT_LABEL', 'audit-accepted')
A = BASE/'pa3-audit'/('focused-'+LABEL)
P = BASE/'pa3-perf'
A.mkdir(parents=True, exist_ok=True)
CPU = min(os.sched_getaffinity(0))
BEFORE = P/'ppexpr-audit-before'
AFTER = P/('ppexpr-'+LABEL)


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


# Exclude course character promotion, mock defined and UB from same-semantics
# host comparison, not from required triple/reference/student validation.
rows = []
for expr, answer in zip((ROOT/'pa3/tests/300-triple.t').read_text().splitlines(),
                        (ROOT/'pa3/tests/300-triple.ref').read_text().splitlines()):
    if 'defined' in expr or "u'" in expr or answer == 'error':
        continue
    if any(op in expr for op in ('<<', '>>')):
        continue
    # Triple intermediates remain within signed 64-bit for these terminals;
    # original test only uses modest positive/negative signed values.
    rows.append((expr, answer))
(A/'portable-triple.expr').write_text(''.join(e+'\n' for e, _ in rows))
(A/'portable-triple.cpp').write_text(''.join(
    f'#if ({e}) != {v[:-1]+"ull" if v.endswith("u") else v+"ll"}\n#error mismatch\n#endif\n{v}\n'
    for e, v in rows)+'eof\n')
(A/'portable-triple.expected').write_text(''.join(v+'\n' for _, v in rows)+'eof\n')
(A/'invalid.expr').write_text('1 +\n'*150000)
(A/'invalid.cpp').write_text('#if 1 +\n#endif\n'*150000)
(A/'invalid.expected').write_text('error\n'*150000+'eof\n')
for scale in (1, 3):
    (A/f'chain-{scale}.expr').write_text('+'.join(['1']*(300000*scale))+'\n')
    (A/f'chain-{scale}.cpp').write_text(f'#if ({(A/f"chain-{scale}.expr").read_text().strip()}) != {300000*scale}\n#error mismatch\n#endif\n{300000*scale}\neof\n')
    (A/f'chain-{scale}.expected').write_text(f'{300000*scale}\neof\n')

variants = {'before': [str(BEFORE)], 'after': [str(AFTER)],
            'gcc': ['g++', '-std=c++11', '-E', '-P', '-x', 'c++', '-'],
            'clang': ['clang++', '-std=c++11', '-E', '-P', '-x', 'c++', '-'],
            'reference': [str(ROOT/'pa3/ppexpr-ref')]}
workloads = ['portable-triple', 'invalid', 'chain-1', 'chain-3', 'full-triple']
inputs = {}
observations = []
for name in workloads:
    inputs[name] = {}
    selected = variants if name != 'full-triple' else {v: variants[v] for v in ('before', 'after', 'reference')}
    for v, cmd in selected.items():
        inp = ROOT/'pa3/tests/300-triple.t' if name == 'full-triple' else A/(name+('.cpp' if v in ('gcc', 'clang') else '.expr'))
        expected = ROOT/'pa3/tests/300-triple.ref' if name == 'full-triple' else A/(name+'.expected')
        inputs[name][v] = {'path': str(inp), 'sha256': sha(inp), 'cmd': cmd}
        with inp.open('rb') as f:
            parity = subprocess.run(cmd, stdin=f, capture_output=True)
        (A/f'{name}-{v}-parity.stderr').write_bytes(parity.stderr)
        if name == 'invalid' and v in ('gcc', 'clang'):
            assert parity.returncode != 0
        else:
            assert parity.returncode == 0, parity.stderr[:1000]
            assert parity.stdout == expected.read_bytes(), (name, v)
        with inp.open('rb') as f:
            subprocess.run(['taskset', '-c', str(CPU), *cmd], stdin=f,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for repeat in range(1, 4):
        for v, cmd in selected.items():
            inp = Path(inputs[name][v]['path'])
            prefix = A/f'{name}-{v}-{repeat}'
            with inp.open('rb') as f, Path(str(prefix)+'.stderr').open('wb') as err:
                r = subprocess.run(['perf', 'stat', '-x', ';', '-o', str(prefix)+'.perf',
                    '-e', '{instructions:u,cycles:u}', '--', '/usr/bin/time',
                    '-f', '%e;%U;%S;%M', '-o', str(prefix)+'.time',
                    'taskset', '-c', str(CPU), *cmd], stdin=f,
                    stdout=subprocess.DEVNULL, stderr=err)
            assert r.returncode == (1 if name == 'invalid' and v in ('gcc', 'clang') else 0), (name, v, r.returncode)
            counts = {}
            running = []
            for line in Path(str(prefix)+'.perf').read_text().splitlines():
                fields = line.split(';')
                if len(fields) > 4 and fields[2] in ('instructions:u', 'cycles:u'):
                    counts[fields[2].split(':')[0]] = int(fields[0].replace(',', ''))
                    running.append(float(fields[4]))
            assert len(counts) == 2 and min(running) >= 95, (name, v, counts, running)
            timing = Path(str(prefix)+'.time').read_text().splitlines()[-1].split(';')
            row = dict(workload=name, variant=v, repeat=repeat, **counts,
                       ipc=counts['instructions']/counts['cycles'], running=running,
                       wall=float(timing[0]), user=float(timing[1]), system=float(timing[2]), rss_kb=int(timing[3]))
            observations.append(row)
(A/'focused-observations.json').write_text(json.dumps(observations, indent=2))
summary = {}
for row in observations:
    name, v = row['workload'], row['variant']
    group = [r for r in observations if (r['workload'], r['variant']) == (name, v)]
    summary.setdefault(name, {})[v] = {key: dict(median=statistics.median(r[key] for r in group),
        min=min(r[key] for r in group), max=max(r[key] for r in group))
        for key in ('instructions', 'cycles', 'ipc', 'wall', 'user', 'system', 'rss_kb')}
(A/'focused-summary.json').write_text(json.dumps(summary, indent=2))
manifest = dict(cpu=CPU, cpu_model=subprocess.check_output(['lscpu'], text=True),
    warmup=1, repeats=3, inputs=inputs, before_sha256=sha(BEFORE), after_sha256=sha(AFTER),
    host_versions={c: subprocess.check_output([c, '--version'], text=True).splitlines()[0] for c in ('g++', 'clang++')},
    host_hashes={c: sha(subprocess.check_output(['which', c], text=True).strip()) for c in ('g++', 'clang++')},
    cc1plus_hash=sha(subprocess.check_output(['g++', '-print-prog-name=cc1plus'], text=True).strip()),
    comparison='Identical expressions in #if envelopes; invalid host diagnostic recovery not output-equivalent; full course triple no equivalent host semantics.')
(A/'focused-manifest.json').write_text(json.dumps(manifest, indent=2))
# Profile triggers at sufficient duration. Keep sampling outside all stat runs.
for name in ('chain-3', 'full-triple', 'invalid'):
    inp = Path(inputs[name]['after']['path'])
    for v in ('before', 'after'):
        data = A/f'{name}-{v}.data'
        with inp.open('rb') as f, (A/f'{name}-{v}-record.log').open('wb') as err:
            subprocess.run(['perf', 'record', '-e', 'cycles:u', '-F', '99',
                '--call-graph', 'dwarf,8192', '-o', str(data), '--', 'taskset', '-c', str(CPU), *variants[v]],
                stdin=f, stdout=subprocess.DEVNULL, stderr=err, check=True)
        report = subprocess.check_output(['perf', 'report', '--stdio', '--no-children',
            '--call-graph', 'none', '-i', str(data), '--sort', 'symbol'], text=True)
        (A/f'{name}-{v}-profile.txt').write_text(report)
for name in ('nested', 'lazy'):
    for v in ('before', 'after'):
        with (P/(name+'.expr')).open('rb') as f, (A/f'{name}-{v}-record.log').open('wb') as err:
            subprocess.run(['perf', 'record', '-e', 'cycles:u', '-F', '99', '--call-graph', 'dwarf,8192',
                '-o', str(A/f'{name}-{v}.data'), '--', 'taskset', '-c', str(CPU), *variants[v]],
                stdin=f, stdout=subprocess.DEVNULL, stderr=err, check=True)
        (A/f'{name}-{v}-profile.txt').write_text(subprocess.check_output(['perf', 'report', '--stdio',
            '--no-children', '--call-graph', 'none', '-i', str(A/f'{name}-{v}.data'), '--sort', 'symbol'], text=True))
# Branch/cache investigations in separate passes, retaining time-running fields.
for events, label in (('{branches:u,branch-misses:u}', 'branch'), ('{cache-references:u,cache-misses:u}', 'cache')):
    for v in ('before', 'after'):
        for repeat in range(1, 4):
            with (A/'chain-3.expr').open('rb') as f:
                subprocess.run(['perf', 'stat', '-x', ';', '-o', str(A/f'chain-3-{v}-{label}-{repeat}.perf'),
                    '-e', events, '--', 'taskset', '-c', str(CPU), *variants[v]], stdin=f,
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
print('Focused audit measurements PASS: parity, paired 3-run PMU, profiles, branch/cache passes')
