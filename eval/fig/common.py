DEFAULT_LINE_WIDTH = 1.1

def plt_config(plt):
    plt.rcParams['pdf.fonttype'] = 42
    plt.rcParams['ps.fonttype'] = 42
    plt.rcParams['font.family'] = ['serif']
    plt.rc('axes', titlesize=14, labelsize=14)  # 축 타이틀 & 축 레이블
    plt.rc('xtick', labelsize=14)  # x축 틱 라벨
    plt.rc('ytick', labelsize=14)  # y축 틱 라벨
    plt.rc('axes', titlesize=14)  # 전체 Figure 제목

    
    plt.rcParams.update({
    # 데이터 선
    "lines.linewidth": DEFAULT_LINE_WIDTH,

    # 축 테두리 (spines)
    "axes.linewidth": DEFAULT_LINE_WIDTH,

    # 눈금 (ticks)
    "xtick.major.width": DEFAULT_LINE_WIDTH,
    "ytick.major.width": DEFAULT_LINE_WIDTH,
    "xtick.minor.width": DEFAULT_LINE_WIDTH,
    "ytick.minor.width": DEFAULT_LINE_WIDTH,

    })


fs_style = {
    'ext4': {
        'label': 'ext4',
        'linestyle': None,
        'color': '#E69F00',
        'marker': '|',
        'linewidth': DEFAULT_LINE_WIDTH,
    },
    'pmfs': {
        'label': 'PMFS',
        'linestyle': ':',
        'color': '#009E73',
        'marker': 's',
        'linewidth': DEFAULT_LINE_WIDTH,
    },
    'nova': {
        'label': 'NOVA',
        'linestyle': None,
        'color': '#0072B2',
        'marker': 'D',
        'markersize': 4,
        'linewidth': DEFAULT_LINE_WIDTH,
    },
    'winefs': {
        'label': 'WineFS',
        'linestyle': 'dashed',
        'color': '#A055D4',
        'marker': 'x',
        'linewidth': DEFAULT_LINE_WIDTH,
    },
    'odinfs': {
        'label': 'OdinFS',
        'linestyle': 'dashed',
        'color': '#F0E442',
        'marker': 'o',
        'linewidth': DEFAULT_LINE_WIDTH,
    },
    'splitfs': {
        'label': 'SplitFS',
        'linewidth': DEFAULT_LINE_WIDTH,
        'color': '#59168C',
        'marker': 'x'
    },
    'sufs': {
        'label': 'ArckFS',
        'linestyle': 'dashed',
        'color': '#c25804',
        'marker': '^',
        'markerfacecolor': 'none',
        'linewidth': DEFAULT_LINE_WIDTH,
    },
    'sufs-fix': {
        'label': 'ArckFS+',
        'linestyle': 'dashed',
        'color': '#D55E00',
        'marker': '^'
    },
    'sufs-fix-fix': {
        'label': 'ArckFS+',
        'linestyle': None,
        'color': '#D55E00',
        'marker': 'D',
        'linewidth': 2,
        'markersize': 4
    },
    'sufs-nd': {
        'label': 'ArckFS-No-Dele',
        'linestyle': 'dashed',
        'color': '#d95f65',
        'marker': '^',
        'markerfacecolor': 'none',
        'linewidth': DEFAULT_LINE_WIDTH,
    },
    'sufs-nd-fix': {
        'label': 'ArckFixFS-No-Dele',
        'linestyle': None,
        'color': '#d95f65',
        'marker': '^',
        'linewidth': DEFAULT_LINE_WIDTH,
    },
    'strata': {
        'label': 'Strata',
        'linestyle': None,
        'color': '#d45eb8',
        'linewidth': DEFAULT_LINE_WIDTH,
        'marker': '^',

    },
}

linestyles = {
    'ext4': '\\',
    'splitfs': '\\\\\\',
    'strata': '\\\\\\',
    'nova': 'xx',
    'winefs': '////',
    'odinfs': '//',
    'sufs': 'xxx',
    'sufs-fix': '/',
    'sufs-fix-fix': '/',
    'sufs-nd': 'xxx',
    'sufs-nd-fix': '/',
    'pmfs': '\\\\\\'
}

legend_font_size = 10

def plt_post(plt):

    plt.subplots_adjust(top=0.6, bottom=0.4, hspace=1)
    plt.tight_layout()

def ax_config(ax):
    ax.tick_params(direction='in')
    ax.grid(True, linestyle=':')
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

def ax_label(ax, loc, show_labels, ncol = 1):
    if len(show_labels) == 0:
        ax.legend(loc=loc, prop={'family': 'monospace', 'size': 10}, frameon=False, ncol=ncol)
        return
    


    handles, labels = ax.get_legend_handles_labels()
    filtered = [(h, l) for h, l in zip(handles, labels) if l in show_labels]

    if filtered:
        handles, labels = zip(*filtered)
        ax.legend(handles, labels, loc=loc, prop={'family': 'monospace', 'size': 10}, frameon=False, ncol=ncol)

def ax_label_upper(ax, loc, show_labels, y=-0.1, ncol = 1):
    if len(show_labels) == 0:
        ax.legend(loc=loc, prop={'family': 'monospace', 'size': 10}, frameon=False, bbox_to_anchor=(0.5, y), ncol=ncol)
        return

    handles, labels = ax.get_legend_handles_labels()
    filtered = [(h, l) for h, l in zip(handles, labels) if l in show_labels]

    if filtered:
        handles, labels = zip(*filtered)
        ax.legend(handles, labels, loc=loc, prop={'family': 'monospace', 'size': 10}, frameon=False, bbox_to_anchor=(0.5, y), ncol = ncol)

def ax_label_m(ax, loc, labels, ncol = 1):
    labels = list(map(lambda x:fs_style[x]["label"], labels))
    ax_label(ax, loc, labels, ncol)

def ax_post(ax, title):
    ax.set_title(title, pad=5)
    ax.set_ylim(bottom=0)
    ax.set_xlim(left=0)


colors = {
    'ext4': '#E69F00', 'strata': 'skyblue', 'nova': 'pink', 'winefs': '#CC79A7', 
    'extr': 'x-', 'odinfs': '#E69F00', 'sufs': '#D55E00', 'sufs-fix': '#D55E00',
    'sufs-nd': '#d95f65', 'sufs-nd-fix': '#d95f65',
    'splitfs': 'blue', 'strata': 'skyblue', 'nova': 'pink', 'winefs': '#CC79A7', 
    'extr': 'x-', 'sufs': '#D55E00', 'sufs-fix-fix': '#D55E00', 'odinfs': 'green',
    'pmfs': fs_style['pmfs']['color']
}