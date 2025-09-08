import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os

from common import *

# 실험 파라미터
workloads = [
    'DWTL', 'MRPL', 'MRPM', 'MRPH',
    'MRDL', 'MRDM', 'MWCL', 'MWCM',
    'MWUL', 'MWUM', 'MWRL', 'MWRM'
]
titles = [
    'DWTL', 'MRPL', 'MRPM', 'MRPH',
    'MRDL', 'MRDM', 'MWCL', 'MWCM',
    'MWUL', 'MWUM', 'MWRL', 'MWRM'
]

# 파일 시스템 및 스타일 정의
fs_list = ['ext4', 'pmfs', 'nova', 'winefs', 'odinfs', 'splitfs', 'sufs', 'sufs-fix-fix']


selected_x = set([1, 2, 4, 6, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48])

plt_config(plt)

linestyles = {
    'ext4': None,
    'pmfs': None,
    'nova': None,
    'winefs': None,
    'odinfs': None,
    'sufs': 'dashed',
    'sufs-fix': None
}
colors = {
    'ext4': '#E69F00', 'pmfs': '#009E73', 'nova': '#0072B2', 'winefs': '#CC79A7', 
    'extr': 'x-', 'odinfs': '#F0E442', 'sufs': '#D55E00', 'sufs-fix': '#D55E00'
}

# 경로 패턴
data_paths = {
    'ext4': '../data/fxmark/pmem-local:{fs}:{wl}:bufferedio.dat',
    'pmfs': '../data/fxmark/pmem-local:{fs}:{wl}:bufferedio.dat',
    'nova': '../data/fxmark/pmem-local:{fs}:{wl}:bufferedio.dat',
    'winefs': '../data/fxmark/pmem-local:{fs}:{wl}:bufferedio.dat',
    'odinfs': '../data/fxmark/pm-array:{fs}:{wl}:bufferedio.dat',
    'splitfs': '../data/fxmark/pmem-local:{fs}:{wl}:bufferedio.dat',
    'sufs' : '../data/fxmark/pm-char-array:sufs:{wl}:bufferedio.dat',
    'sufs-fix-fix': '../data/fxmark/plus/pm-char-array:sufs:{wl}:bufferedio.dat',
}

# Figure 생성
fig, axes = plt.subplots(3, 4, figsize=(14, 6), sharex=False)
axes = axes.flatten()

A = {}
B = {}

for idx, (wl, title) in reversed(list(enumerate(zip(workloads, titles)))):
    ax = axes[idx]

    ax_config(ax)

    for fs in fs_list:
        path_template = data_paths[fs]
        filepath = path_template.format(fs=fs, wl=wl)
        
        if not os.path.exists(filepath):
            print(f"No data for {fs} {title}")
            #if idx == 0:
            # ax.plot([], [], **fs_style[fs])
            #    ax.axis('off')
            # ax_label(ax, "center", [], 2)
            continue
        try:
            df = pd.read_csv(filepath, delim_whitespace=True, header=None, comment="#")

            x = df[0].astype(int)
            y = df[1].astype(float) / 1_000_000  # Convert to ops/μs
            
            x, y = zip(*[(x, y) for x, y in zip(x, y) if x in selected_x])

            if fs == 'sufs-fix-fix':
                A[wl] = y[-1]
            if fs == 'sufs':
                B[wl] = y[-1]

            ax.plot(x, y, **fs_style[fs])
            #ax.plot(x, y, color=colors[fs], linestyle=linestyles[fs], label=fs)
        except Exception as e:
            print(f"Error loading {filepath}: {e}")

    # if idx != 0:
    ax_post(ax, title)
        # ax.set_title(title, pad=5)

    if idx == 0:
        ax_label_m(ax, "upper left", ["ext4", "pmfs"], 1)

    if idx == 1:
        ax_label_m(ax, "upper left", ["nova", "winefs"], 1)
    
    if idx == 2:
        ax_label_m(ax, "upper left", ["splitfs", "odinfs"], 1)
    
    if idx == 3:
        ax_label_m(ax, "upper left", ["sufs", "sufs-fix-fix"], 1)

    if idx % 4 == 0 or idx == 1:
        ax.set_ylabel(r'Mops/s')
    if idx > 7:
        ax.set_xlabel('# threads')
plt.tight_layout()

plt.savefig('fig4.pdf')
plt.show()

print(A)
print(B)

for wl in workloads:
    # if wl == 'DWTL':
        # continue
    print(wl, round((A[wl] / B[wl]) * 100, 2))