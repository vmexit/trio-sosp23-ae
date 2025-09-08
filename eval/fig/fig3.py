import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os

from common import *

plt_config(plt)

# 실험 파라미터
workloads = [
    'seq-read-4K',
    'seq-write-4K',
    'seq-read-2M',
    'seq-write-2M',
    'MRPL',
    'MWCL',
    'MWUL'
]

titles = [
    '4KB data read',
    '4KB data write',
    '2MB data read',
    '2MB data write',
    'open',
    'create',
    'delete'
]

# 파일 시스템 및 스타일 정의
fs_list = ['nova', 'splitfs', 'odinfs', 'strata', 'sufs', 'sufs-fix-fix',]

# 경로 패턴
data_paths = {
    'strata': '../data/sg_meta/pm-char:{fs}:{wl}:bufferedio.dat',
    'nova': '../data/sg_meta/pmem-local:{fs}:{wl}:bufferedio.dat',
    'sufs' : '../data/sg_meta/pm-char-array:sufs:{wl}:bufferedio.dat',
    'sufs-fix-fix': '../data/sg_meta/plus/pm-char-array:sufs:{wl}:bufferedio.dat',
}

# Figure 생성
fig, axes = plt.subplots(1, 2, figsize=(7, 2.5), sharex=False)
axes = axes.flatten()

width = 6

for i in range(2, 4):
    ax = axes[i-2]
    ax.tick_params(axis='x', labelsize=14)
    ax.tick_params(axis='y', labelsize=14)


    a = 6
    if i==3:
        ax.set_xlim(width/2+1.5 - a, width/2+1.5 +a)
    elif i != 2:
        ax.set_xlim(width/2+2 - a, width/2+2 +a)
    else:
        ax.set_xlim(1.5-a, 1.5+a)
    ax.tick_params(direction='in')
    ax.grid(True, linestyle=':', zorder=-1000)
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

# axes[0]

ax = axes[0]

def get_config(fs, i):
    if i == 0:
        return {
            'edgecolor': colors[fs],
            'color': "white" if fs!="sufs-fix-fix" else colors[fs],
            'hatch': linestyles[fs],
            'label': fs_style[fs]['label'],
            'zorder': 50
        }
    else:
        return {
            'edgecolor': colors[fs],
            'color': "white" if fs!="sufs-fix-fix" else colors[fs],
            'hatch': linestyles[fs],
            'zorder': 50
        }


ax = axes[0]

meta_fs_list = ['nova', 'strata', 'sufs', 'sufs-fix-fix']

for fs in meta_fs_list:
    for (i, (wl, tick)) in enumerate(zip(['MRPL'], ['open'])):
        path_template = data_paths[fs]
        filepath = path_template.format(fs=fs, wl=wl)

        if not os.path.exists(filepath):
            print(f"No data for {fs} {wl}")
            continue
        df = pd.read_csv(filepath, delim_whitespace=True, header=None, comment="#")

        x = df[0].astype(int)
        y = df[1].astype(float) / 1_000_000  # Convert to Mops/s
        print(fs, y)
            
        ax.bar([meta_fs_list.index(fs) + i * width], y, **get_config(fs, i))


ax_label(ax, "upper left", ['NOVA', 'Strata', 'ArckFS', 'ArckFS+'])

ax.set_title('(a) Read metadata', pad=5)
ax.set_ylabel('Throughput (Mops/s)')
ax.set_xticks([1.5], ['open'])

ax = axes[1]

for fs in meta_fs_list:
    for (i, (wl, tick)) in enumerate(zip(['MWCL', 'MWUL'], ['create', 'delete'])):
        path_template = data_paths[fs]
        filepath = path_template.format(fs=fs, wl=wl)

        if not os.path.exists(filepath):
            print(f"No data for {fs} {wl}")
            continue
        try:
            df = pd.read_csv(filepath, delim_whitespace=True, header=None, comment="#")

            x = df[0].astype(int)
            y = df[1].astype(float) / 1_000_000  # Convert to Mops/s
            print(fs, y)
            
            ax.bar([meta_fs_list.index(fs) + i * width], y, **get_config(fs, i))

        except Exception as e:
            print(f"Error loading {filepath}: {e}")

ax.set_title('(b) Write metadata', pad=5)
# ax.set_ylabel(Throughput (GiB/s)')
ax.set_xticks([1.5, 1.5 + width], ['create', 'delete'])



'''
for idx, (wl, title) in reversed(list(enumerate(zip(workloads, titles)))):
    ax = axes[idx]
    for fs in fs_list:
        path_template = data_paths[fs]
        filepath = path_template.format(fs=fs, wl=wl)

        if not os.path.exists(filepath):
            print(f"No data for {fs} {title}")
            continue
        try:
            df = pd.read_csv(filepath, delim_whitespace=True, header=None, comment="#")

            x = df[0].astype(int)
            y = df[1].astype(float) / 1_000_000  # Convert to ops/μs
            
            ax.bar([fs_list.index(fs)], y, edgecolor=colors[fs], color="white" if fs!="sufs" else colors[fs], hatch=linestyles[fs], label=fs)

        except Exception as e:
            print(f"Error loading {filepath}: {e}")

    ax.set_title(title, pad=5)
    if idx % 4 == 0:
        # ax.legend(loc="upper left", fontsize=8)
        pass
    if idx >= 4:
        ax.set_ylabel('Throughput (Mops/s)')
    else:
        ax.set_ylabel('Throughput (GiB/s)')
    #if idx >= 6:
    ax.set_xticks([]) 
    # ax.set_xlabel('# Threads')
handles, labels = [], []
for ax in axes:
    h, l = ax.get_legend_handles_labels()
    handles += h
    labels += l

# dict로 중복 제거
by_label = dict(zip(labels, handles))

# global legend 설정
fig.legend(by_label.values(), by_label.keys(), 
           loc='upper center', bbox_to_anchor=(0.5, 0.95), ncol=6)'''
'''plt.subplots_adjust(top=0.85)
plt.subplots_adjust(wspace=0.4)'''
# 레이아웃 조정 및 저장
plt.tight_layout()

plt.savefig('fig3.pdf')
plt.show()