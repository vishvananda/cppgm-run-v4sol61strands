#!/usr/bin/env python3
"""Frozen same-source PA2 benchmark; raw measurements/profiles live in artifacts."""
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
p.add_argument('--prepare', action='store_true')
p.add_argument('--against', help='Include frozen cursor/tool binaries from a prior label')
a = p.parse_args()
A = a.artifact.resolve(); A.mkdir(parents=True, exist_ok=True)
cpu = min(os.sched_getaffinity(0))

def run(cmd, **kwargs):
    return subprocess.run([str(x) for x in cmd], check=True, **kwargs)

def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''): digest.update(block)
    return digest.hexdigest()

workloads = ('declarations', 'raw', 'hosted', 'literals', 'concat')
if a.prepare:
    with (A/'declarations.cpp').open('w') as out:
        for n in range(80000):
            out.write(f'namespace n{n} {{ int value = {n}; double f(double x) {{ return x * 1.25e+2 + value; }} }}\n')
    with (A/'raw.cpp').open('w') as out:
        for n in range(40000):
            out.write(f'const char* s{n} = u8R"1234567890123456(' + ')123456789012345X' * 8
                      + ' ??= \\u03c0 \\n)1234567890123456";\n')
    original = A/'hosted-original.cpp'
    original.write_text('#include <vector>\n#include <string>\n#include <tuple>\n#include <algorithm>\n'
                        'std::vector<std::tuple<std::string,int>> values;\n'
                        + ''.join(f'int hosted{n}(const std::vector<int>& v) {{ return v.size() + {n}; }}\n'
                                  for n in range(80000)))
    hosted = subprocess.check_output(['g++', '-std=c++11', '-E', '-P', str(original)], text=True)
    # Freeze GCC-expanded hosted source; PA2 explicitly excludes phase-4 work.
    (A/'hosted.cpp').write_text(''.join(line for line in hosted.splitlines(keepends=True)
                                      if not line.lstrip().startswith('#')))
    with (A/'literals.cpp').open('w') as out:
        for n in range(40000):
            out.write(f'namespace n{n} {{ const char16_t s[] = "\\x3c0" u"😀"; unsigned long long x = 0xffffffffffffffffULL; double d = 1.25e-2; }}\n')
    with (A/'concat.cpp').open('w') as out:
        for n in range(1000):
            out.write(f'const char16_t s{n}[] = ' + ' '.join(['"\\x3c0"']*500) + ' u"😀";\n')

flags = ['-std=c++11', '-O3', '-g', '-I'+str(ROOT/'dev/src')]
sources = list((ROOT/'dev/src/preprocess/lex').glob('*.cpp'))
sources += list((ROOT/'dev/src/preprocess/post').glob('*.cpp'))
for variant, main in [('cursor', ROOT/'student.tests/pa2/bench.cpp'), ('posttoken', ROOT/'dev/posttoken.cpp')]:
    run(['g++', *flags, main, *sources, '-o', A/(variant+'-'+a.label)])
variants = {
    'cursor': [A/('cursor-'+a.label)], 'posttoken': [A/('posttoken-'+a.label)],
    'gcc': ['g++', '-std=c++11', '-trigraphs', '-fsyntax-only', '-x', 'c++', '-'],
    'clang': ['clang++', '-std=c++11', '-trigraphs', '-fsyntax-only', '-x', 'c++', '-'],
    'gcc-lex': ['g++', '-std=c++11', '-trigraphs', '-E', '-P', '-x', 'c++', '-'],
    'clang-lex': ['clang++', '-std=c++11', '-trigraphs', '-E', '-P', '-x', 'c++', '-'],
    'reference': [ROOT/'reference-binaries/posttoken'],
}
if a.against:
    for variant in ('cursor', 'posttoken'):
        variants[variant+'-baseline'] = [A/(variant+'-'+a.against)]
manifest = {'cpu': cpu, 'flags': flags, 'warmups': 1, 'runs': 3,
            'cpu_model': next(line.strip() for line in pathlib.Path('/proc/cpuinfo').read_text().splitlines()
                              if line.startswith('model name')),
            'environment': {'CPPGM_POST_METRICS': '1'},
            'commands': {key: [str(x) for x in cmd] for key, cmd in variants.items()},
            'inputs': {name: {'bytes': (A/(name+'.cpp')).stat().st_size, 'sha256': sha(A/(name+'.cpp'))} for name in workloads},
            'binaries': {key: sha(pathlib.Path(shutil.which(str(cmd[0])))) for key, cmd in variants.items()},
            'hosts': {host: subprocess.check_output([host, '--version'], text=True).splitlines()[0]
                      for host in ('g++', 'clang++')}}
manifest['prepare_command'] = ['g++', '-std=c++11', '-E', '-P', str(A/'hosted-original.cpp')]
cc1 = pathlib.Path(subprocess.check_output(['g++', '-print-prog-name=cc1plus'], text=True).strip())
manifest['cc1plus_sha256'] = sha(cc1)
(A/(a.label+'-manifest.json')).write_text(json.dumps(manifest, indent=2))
observations = []
parity = {}
for workload in workloads:
    source = A/(workload+'.cpp')
    # Check complete outputs first, then erase large derived renderings; hashes
    # and raw diagnostics retain parity evidence without using fixture answers.
    outputs = []
    for variant in ('posttoken', 'reference'):
        path = A/(workload+'-'+variant+'.out'); outputs.append(path)
        with source.open('rb') as inp, path.open('wb') as out:
            run(variants[variant], stdin=inp, stdout=out)
    equal = sha(outputs[0]) == sha(outputs[1])
    parity[workload] = {'student_sha256': sha(outputs[0]), 'reference_sha256': sha(outputs[1]),
                        'bytes': outputs[0].stat().st_size, 'exact_match': equal}
    if not equal:
        # GCC 15 system headers contain extensions beyond PA2's C++11 grammar.
        # Keep every difference, not a normalized oracle or claimed equivalence.
        assert workload == 'hosted', workload
        student = outputs[0].read_text().splitlines()
        reference = outputs[1].read_text().splitlines()
        assert len(student) == len(reference)
        differences = [{'line': i+1, 'student': x, 'reference': y}
                       for i, (x, y) in enumerate(zip(student, reference)) if x != y]
        allowed = {('identifier __decltype', 'simple __decltype KW_DECLTYPE'),
                   ('identifier __typeof__', 'simple __typeof__ KW_DECLTYPE'),
                   ('identifier __typeof', 'simple __typeof KW_DECLTYPE'),
                   ('invalid 0.0bf16', 'literal 0.0bf16 _Float16 00000000')}
        assert all((d['student'], d['reference']) in allowed for d in differences), differences[:10]
        parity[workload]['extension_differences'] = differences
    else:
        assert equal
    (A/(a.label+'-parity.json')).write_text(json.dumps(parity, indent=2))
    for path in outputs: path.unlink()
    active = {key: value for key, value in variants.items() if not (workload == 'hosted' and key == 'clang')}
    for variant, command in active.items():
        with source.open('rb') as inp, (A/(a.label+'-'+workload+'-'+variant+'-warm.stderr')).open('wb') as err:
            run(['taskset', '-c', cpu, *command], stdin=inp, stdout=subprocess.DEVNULL, stderr=err)
    for repeat in range(3):
        for variant, command in active.items():
            stem = A/f'{a.label}-{workload}-{variant}-{repeat}'
            with source.open('rb') as inp, pathlib.Path(str(stem)+'.telemetry').open('wb') as err:
                run(['perf', 'stat', '-x', ';', '-o', str(stem)+'.perf', '-e', '{instructions:u,cycles:u}',
                     '--', 'taskset', '-c', cpu, '/usr/bin/time', '-f', '%e;%U;%S;%M', '-o', str(stem)+'.time',
                     *command], stdin=inp, stdout=subprocess.DEVNULL, stderr=err,
                    env={**os.environ, 'CPPGM_POST_METRICS': '1'})
            wall, user, system, rss = pathlib.Path(str(stem)+'.time').read_text().strip().split(';')
            counts = {}
            for line in pathlib.Path(str(stem)+'.perf').read_text().splitlines():
                f = line.split(';')
                if len(f) > 2 and f[2] in ('instructions:u', 'cycles:u'):
                    assert f[0].strip().isdigit() and float(f[4]) >= 95, line
                    counts[f[2]] = int(f[0])
            assert len(counts) == 2, stem
            observations.append({'workload': workload, 'variant': variant, 'repeat': repeat,
                'instructions': counts['instructions:u'], 'cycles': counts['cycles:u'],
                'ipc': counts['instructions:u']/counts['cycles:u'], 'wall': float(wall), 'user': float(user),
                'system': float(system), 'rss_kb': int(rss)})
            (A/(a.label+'-observations.json')).write_text(json.dumps(observations, indent=2))
# Typed payload checksums must remain stable across repetitions/build variants.
for workload in workloads:
    checksums = set()
    for variant in ('cursor', 'cursor-baseline') if a.against else ('cursor',):
        for repeat in range(3):
            telemetry = (A/f'{a.label}-{workload}-{variant}-{repeat}.telemetry').read_text()
            checksums.add(re.search(r'checksum=(\d+)', telemetry).group(1))
    assert len(checksums) == 1, (workload, checksums)
summary = {}
for workload in workloads:
    summary[workload] = {}
    for variant in variants:
        rows = [row for row in observations if row['workload'] == workload and row['variant'] == variant]
        if not rows:
            summary[workload][variant] = {'inconclusive': 'GCC-preprocessed hosted extensions rejected by Clang; see capability diagnostic'}
            continue
        summary[workload][variant] = {key: {'median': statistics.median(row[key] for row in rows),
            'min': min(row[key] for row in rows), 'max': max(row[key] for row in rows)}
            for key in ('instructions', 'cycles', 'ipc', 'wall', 'user', 'system', 'rss_kb')}
(A/(a.label+'-parity.json')).write_text(json.dumps(parity, indent=2))
(A/(a.label+'-summary.json')).write_text(json.dumps(summary, indent=2))
print(json.dumps(summary, indent=2))
