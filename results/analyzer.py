import pandas as pd
import numpy as np
import os
import matplotlib.pyplot as plt

if __name__ == '__main__':

    # append all the results into a single pandas DataFrame
    df = pd.DataFrame()
    for entry in os.scandir('.'):
        if entry.path.endswith('.csv'):
            data = pd.read_csv(entry.path)
            df = df.append(data)

    # PLOTTING RESULTS OF ALL GRAPHS

    # filter DataFrame by graph_name starting with rgg
    fig1 = plt.figure(figsize=(15, 8))
    ax1 = fig1.add_subplot(111)
    fig2 = plt.figure(figsize=(15, 8))
    ax2 = fig2.add_subplot(111)

    # export dataframe to markdown table
    # df_gb = df.groupby(['coloring_method', 'graph_name'])['coloring_time'].mean().reset_index().round(decimals=5)
    # df_gb = df_gb.pivot("graph_name","coloring_method","coloring_time")
    # cols = df_gb.columns
    # df_sep = pd.DataFrame([['---',]*len(cols)], columns=cols )
    # df_md = pd.concat([df_sep, df_gb])
    # df_md.to_csv("table.md", sep="|",index_label='graph_name')

    # for each different coloring method
    for coloring_method in df.coloring_method.unique():
        # group by
        df_vc = df[df.coloring_method == coloring_method].groupby(
            ['graph_name'])['coloring_time', 'colors_used'].mean().reset_index()

        # convert vertex_count from int to string (prevents labels in plot to show up as floats)
        x = [str(i) for i in df_vc.graph_name.tolist()]
        y = df_vc.coloring_time.tolist()
        ax1.plot(x, y, '-o', label=coloring_method, linewidth=0.75)
        y = df_vc.colors_used.tolist()
        ax2.plot(x, y, '-o', label=coloring_method, linewidth=0.75)

    ax1.legend(prop={'size': 12})
    ax2.legend(prop={'size': 12})
    ax1.grid()
    ax2.grid()
    ax1.set_xlabel('Graph name')
    ax2.set_xlabel('Graph name')
    ax1.set_ylabel('Average coloring time in seconds')
    ax2.set_ylabel('Average colors used')
    ax1.set_title(
        'Comparison of coloring algorithms over 36 different graphs')
    ax2.set_title(
        'Comparison of coloring algorithms over 36 different graphs (biased by outliers)')
    ax1.set_yticks(range(0, int(max(ax1.get_yticks())-1)))
    ax1.set_xticklabels(x, rotation=50, ha='right')
    ax2.set_xticklabels(x, rotation=50, ha='right')
    ax1.tick_params(axis='x', labelsize=12)
    ax2.tick_params(axis='x', labelsize=12)
    fig1.tight_layout()
    fig2.tight_layout()
    fig1.savefig('plots/all_time_comparison.png')
    fig2.savefig('plots/all_colors_comparison.png')

    
    # PLOTTING AVG COLORS USED OF ALL GRAPHS EXCEPT v100.gra and v1000.gra
    
    # filter DataFrame by graph_name not starting with v100
    df_nov = df[~df.graph_name.str.contains('^v100')]
    fig = plt.figure(figsize=(15, 8))
    ax = fig.add_subplot(111)

    # for each different coloring method
    for coloring_method in df_nov.coloring_method.unique():
        # group by
        df_vc = df_nov[df_nov.coloring_method == coloring_method].groupby(
            ['graph_name'])['coloring_time', 'colors_used'].mean().reset_index()

        # convert vertex_count from int to string (prevents labels in plot to show up as floats)
        x = [str(i) for i in df_vc.graph_name.tolist()]
        y = df_vc.colors_used.tolist()
        ax.plot(x, y, '-o', label=coloring_method, linewidth=0.75)

    ax.legend(prop={'size': 12})
    ax.grid()
    ax.set_xlabel('Graph name')
    ax.set_ylabel('Average colors used')
    ax.set_title(
        'Comparison of coloring algorithms over 34 different graphs')
    ax.set_xticklabels(x, rotation=50, ha='right')
    ax.tick_params(axis='x', labelsize=12)
    fig.tight_layout()
    fig.savefig('plots/no_v100_colors_comparison.png')

    
    # PLOTTING RESULTS OF RGG GRAPHS

    # filter DataFrame by graph_name starting with rgg
    df_rgg = df[df.graph_name.str.contains('^rgg')]
    fig1 = plt.figure(figsize=(12, 8))
    ax1 = fig1.add_subplot(111)
    fig2 = plt.figure(figsize=(12, 8))
    ax2 = fig2.add_subplot(111)

    # for each different coloring method
    for coloring_method in df.coloring_method.unique():

        # group by vertex_count (nb: in this occasion, this equals to grouping by graph_name since each graph has a different vertex_count)
        df_vc = df_rgg[df_rgg.coloring_method == coloring_method].groupby(
            ['vertex_count'])['coloring_time', 'colors_used'].mean().reset_index()

        # convert vertex_count from int to string (prevents labels in plot to show up as floats)
        x = [str(i) for i in df_vc.vertex_count.tolist()]
        y = df_vc.coloring_time.tolist()
        ax1.plot(x, y, '-o', label=coloring_method, linewidth=0.75)
        y = df_vc.colors_used.tolist()
        ax2.plot(x, y, '-o', label=coloring_method, linewidth=0.75)

    ax1.legend(prop={'size': 12})
    ax2.legend(prop={'size': 12})
    ax1.grid()
    ax2.grid()
    ax1.set_xlabel('Number of vertices')
    ax2.set_xlabel('Number of vertices')
    ax1.set_ylabel('Average coloring time in seconds')
    ax2.set_ylabel('Average colors used')
    ax1.set_title(
        'Comparison of coloring algorithms over RGGs (Random Geometric Graphs)')
    ax2.set_title(
        'Comparison of coloring algorithms over RGGs (Random Geometric Graphs)')
    ax1.set_yticks(range(0, 15))
    fig1.tight_layout()
    fig2.tight_layout()
    ax1.tick_params(axis='x', labelsize=12)
    ax2.tick_params(axis='x', labelsize=12)
    fig1.savefig('plots/rgg_time_comparison.png')
    fig2.savefig('plots/rgg_colors_comparison.png')

    # PLOTTING COLORING TIME PER DIFFERENT NUMBER OF THREADS
    df_par = df[df.coloring_method.str.contains('^par')]
    df_par = df_par.groupby(['coloring_method', 'n_threads'])[
        'coloring_time'].mean().reset_index()
    df_par = df_par.pivot("n_threads", "coloring_method", "coloring_time")
    # print(df_par)
    ax = df_par.plot(kind='bar', stacked=False, rot=0, xlabel='Number of threads',
                     ylabel='Average coloring time in seconds', width=0.6)
    
    for p in ax.patches:
        ax.annotate(np.round(p.get_height(), decimals=2), (p.get_x()+p.get_width()/2.,
                    p.get_height()), ha='center', va='center', xytext=(0, 5), textcoords='offset points')
    fig = ax.get_figure()
    fig.suptitle(
        'Comparison of parallel coloring algorithms for different number of threads')
    fig.tight_layout()
    fig.savefig('plots/par_threads_comparison.png')

