"""Full CPU-header correctness and paired screening, no GPU/official-score claim."""
import json
import os
from pathlib import Path
import re
import statistics
import subprocess
import sys

root=Path.cwd()
common=['g++','-std=c++17','-O3','-Wno-deprecated-declarations','-DQSB_CPU_THREADS=1','-DQSB_CPU_DEVBENCH=6','host_driver.cpp','-lcrypto','-pthread']
for N,kind in [(12,'exact'),(24,'bench')]:
    for arm in ['base','tree']:
        cmd=common+['-DQSB_ZEROS_N='+str(N),'-o',f'{arm}_{kind}']
        if arm=='base':cmd+=['-DBASELINE=1']
        subprocess.run(cmd,check=True)

def execute(arm,kind,label,cap,limit=None):
    work=root/'round3-results'/label;work.mkdir(parents=True,exist_ok=True)
    env=dict(os.environ,QSB_CPU_TABLE_MB=str(cap),QSB_CPU_MODE='2')
    if limit:env['QSB_CPU_DEVCAND']=str(limit)
    run=subprocess.run([str(root/f'{arm}_{kind}'),str(root/'subset.bin')],cwd=work,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90)
    (work/'stdout.log').write_text(run.stdout)
    print('FULL',label,run.returncode,flush=True)
    print(run.stdout,flush=True)
    assert run.returncode==0
    return work,run.stdout

sys.path.insert(0,str(root/'generated/harness'))
import gpu_wrap
import verify
prob=json.loads((root/'subset.json').read_text())
os.environ['QSB_VERIFY_WORKERS']='2'
sets=[]
for arm in ['base','tree']:
    work,out=execute(arm,'exact',arm+'-exact',128,262144)
    assert 'DEVCAND 262144 candidates' in out
    hits=gpu_wrap.collect_hits('subset',work/'results')
    artifact={'bench':'subset','zeros_n':12,'candidates':262144,'hits':hits}
    verified,failures,warn=verify.verify_artifact(artifact,prob=prob,bench='subset',N=12)
    assert verified==len(hits)>0 and not failures,(verified,failures)
    sets.append({(tuple(h['skip']),h['recid']) for h in hits})
    (work/'artifact.json').write_text(json.dumps(artifact))
    print('EXACT',arm,'verified',verified,'failures',len(failures),'warning',warn,flush=True)
assert sets[0]==sets[1]
print('PASS FULL CPU HIT SETS identical',len(sets[0]),flush=True)

rates=[]
for block in range(2):
    for phase,arm in enumerate(['base','tree','tree','base']):
        _,out=execute(arm,'bench',f'b{block}-p{phase}-{arm}',1024)
        m=re.search(r'DEVBENCH cpu ([0-9.]+) M/s',out);assert m
        rates.append({'block':block,'phase':phase,'arm':arm,'Mps':float(m.group(1))})
baseline=[r['Mps'] for r in rates if r['arm']=='base'];variant=[r['Mps'] for r in rates if r['arm']=='tree']
ratios=[]
for b in range(2):
    a=[r['Mps'] for r in rates if r['block']==b and r['arm']=='base'];v=[r['Mps'] for r in rates if r['block']==b and r['arm']=='tree']
    ratios.append(statistics.mean(v)/statistics.mean(a))
summary={'hit_set_count':len(sets[0]),'CPU_only_worker_rates':rates,'baseline_mean_Mps':statistics.mean(baseline),'tree_mean_Mps':statistics.mean(variant),'ratio_of_means':statistics.mean(variant)/statistics.mean(baseline),'block_ratios':ratios,'limitations':'Free Linux x86 CPU-only screening, one thread, fixed 1GiB budget/mode2. No GPU, no ranked-host thermal evidence, no formal score.'}
(root/'round3-results/summary.json').write_text(json.dumps(summary,indent=2))
print('SUMMARY',json.dumps(summary),flush=True)
