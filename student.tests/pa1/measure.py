#!/usr/bin/env python3
"""Pinned warm-cache PA1 measurements; raw observations remain outside checkout."""
import argparse
import hashlib
import json
import os
import pathlib
import statistics
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('artifact', type=pathlib.Path)
parser.add_argument('--label', default='current')
parser.add_argument('--prepare', action='store_true')
parser.add_argument('--runs', type=int, default=3)
args = parser.parse_args()
args.artifact.mkdir(parents=True, exist_ok=True)
A = args.artifact.resolve()
cpu = min(os.sched_getaffinity(0))

def run(cmd, **kw):
    return subprocess.run([str(x) for x in cmd], check=True, **kw)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

if args.prepare:
    # Each workload is phases-1-3 supported. No macros or include expansion in
    # the measured input. Hosted case freezes a real GCC-expanded standard TU.
    with (A / 'declarations.cpp').open('w') as out:
        for n in range(80000):
            out.write(f'namespace n{n} {{ int value = {n}; double f(double x) {{ return x * 1.25e+2 + value; }} }}\n')
    with (A / 'raw.cpp').open('w') as out:
        for n in range(40000):
            out.write(f'const char* s{n} = u8R"1234567890123456(' + ')123456789012345X' * 8
                      + ' ??= \\u03c0 \\n)1234567890123456";\n')
    hosted = A / 'hosted-original.cpp'
    hosted.write_text('#include <vector>\n#include <string>\n#include <tuple>\n#include <algorithm>\n'
                      'std::vector<std::tuple<std::string,int>> values;\n')
    with (A / 'hosted.cpp').open('wb') as out:
        run(['g++', '-std=c++11', '-E', '-P', hosted], stdout=out)

# Freeze optimized symbols for profiling, plus the exact tool wrapper being used.
flags = ['-std=c++11', '-O3', '-g', '-I' + str(ROOT / 'dev/src')]
sources = [ROOT / 'dev/src/preprocess/lex/lexer.cpp', ROOT / 'dev/src/preprocess/lex/unicode.cpp']
cursor = A / ('cursor-' + args.label)
tool = A / ('pptoken-' + args.label)
run(['g++', *flags, ROOT / 'student.tests/pa1/lex_bench.cpp', *sources, '-o', cursor])
run(['g++', *flags, ROOT / 'dev/pptoken.cpp', *sources, '-o', tool])
reference = ROOT / 'reference-binaries/pptoken'
variants = {
    'cursor': [cursor],
    'pptoken': [tool],
    'gcc': ['g++', '-std=c++11', '-trigraphs', '-E', '-P', '-x', 'c++', '-'],
    'clang': ['clang++', '-std=c++11', '-trigraphs', '-E', '-P', '-x', 'c++', '-'],
    'reference': [reference],
}
manifest = {'cpu': cpu, 'flags': flags, 'warmup': 1, 'runs': args.runs,
            'cpu_model': next(line.strip() for line in pathlib.Path('/proc/cpuinfo').read_text().splitlines()
                              if line.startswith('model name')),
            'variants': variants, 'hashes': {p.name: sha(p) for p in [cursor, tool, reference]},
            'inputs': {name: sha(A / (name + '.cpp')) for name in ('declarations', 'raw', 'hosted')}}
manifest['variants'] = {k: [str(x) for x in v] for k, v in variants.items()}
(A / (args.label + '-manifest.json')).write_text(json.dumps(manifest, indent=2))
observations = []
for workload in ('declarations', 'raw', 'hosted'):
    source = A / (workload + '.cpp')
    # Check reference parity before timing; debug output contains multiline raw
    # strings, so compare bytes rather than treating lines as token records.
    for variant in ('pptoken', 'reference'):
        with source.open('rb') as inp, (A / (workload + '-' + variant + '.out')).open('wb') as out:
            run(variants[variant], stdin=inp, stdout=out)
    assert sha(A / (workload + '-pptoken.out')) == sha(A / (workload + '-reference.out')), workload
    for variant, command in variants.items():
        with source.open('rb') as inp:
            run(['taskset', '-c', cpu, *command], stdin=inp, stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL)
    # Interleave variants, retaining every PMU and /usr/bin/time observation.
    for repeat in range(args.runs):
        for variant, command in variants.items():
            stem = A / f'{args.label}-{workload}-{variant}-{repeat}'
            with source.open('rb') as inp, pathlib.Path(str(stem) + '.telemetry').open('wb') as err:
                run(['perf', 'stat', '-x', ';', '-o', str(stem) + '.perf', '-e',
                     '{instructions:u,cycles:u}', '--', 'taskset', '-c', cpu,
                     '/usr/bin/time', '-f', '%e;%U;%S;%M', '-o', str(stem) + '.time',
                     *command], stdin=inp, stdout=subprocess.DEVNULL, stderr=err,
                    env={**os.environ, 'CPPGM_LEX_METRICS': '1'})
            wall, user, system, rss = pathlib.Path(str(stem) + '.time').read_text().strip().split(';')
            counts = {}
            for line in pathlib.Path(str(stem) + '.perf').read_text().splitlines():
                fields = line.split(';')
                if len(fields) > 2 and fields[2] in ('instructions:u', 'cycles:u'):
                    assert fields[0].strip().isdigit(), line
                    assert float(fields[4]) >= 95, ('multiplexed', line)
                    counts[fields[2]] = int(fields[0])
            assert len(counts) == 2, stem
            observations.append({'workload': workload, 'variant': variant, 'repeat': repeat,
                'instructions': counts['instructions:u'], 'cycles': counts['cycles:u'],
                'ipc': counts['instructions:u'] / counts['cycles:u'],
                'wall': float(wall), 'user': float(user), 'system': float(system), 'rss_kb': int(rss)})
(A / (args.label + '-observations.json')).write_text(json.dumps(observations, indent=2))
lines = ['workload variant instructions median[range] cycles median[range] IPC median wall median[range] RSS median[range]']
for workload in ('declarations', 'raw', 'hosted'):
    for variant in variants:
        group = [o for o in observations if o['workload'] == workload and o['variant'] == variant]
        def stats(key):
            numbers = [o[key] for o in group]
            return f'{statistics.median(numbers):.3g}[{min(numbers):.3g},{max(numbers):.3g}]'
        lines.append(f'{workload} {variant} {stats("instructions")} {stats("cycles")} '
                     f'{statistics.median(o["ipc"] for o in group):.3f} {stats("wall")} {stats("rss_kb")}')
report = '\n'.join(lines) + '\n'
(A / (args.label + '-summary.txt')).write_text(report)
print(report)
