# Copyright 2024 Dynatrace LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Portions of this code, as identified in remarks, are provided under the
# Creative Commons BY-SA 4.0 or the MIT license, and are provided without
# any warranty. In each of the remarks, we have provided attribution to the
# original creators and other attribution parties, along with the title of
# the code (if known) a copyright notice and a link to the license, and a
# statement indicating whether or not we have modified the code.

import pandas as pd
import seaborn as sns
import matplotlib
import matplotlib.ticker
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.rcsetup import cycler

from constants_and_classes import *
from helper_functions import (
    adjust_box_widths,
    save_plot,
    generate_tick_arr,
    get_csv_files_in_folder,
)

COLOR_PALETTE = cycler(
    "color", ["#a1c9f4", "#ffb482", "#8de5a1", "#8eaafb", "#fea19e", "#89feb5"]
)
RC_CONTEXT = {
    "text.usetex": True,
    "font.family": "Computer Modern",  # "Palatino"
    "font.size": 10,
    "axes.prop_cycle": COLOR_PALETTE,
    "savefig.bbox": "tight",
    "savefig.pad_inches": 0.05,
}

BOXPLOT_LINE_WIDTH = 0.75
PLOT_WIDTH_AND_SPACE = 0.95

# ------------------------------------------------------ Box plots ------------------------------------------------------


@matplotlib.rc_context(RC_CONTEXT)
def plot_boxplots_worst_case(
    df: pd.DataFrame, show_outliers: bool = False, image_name=""
):
    df = df.copy(deep=True)
    fig = plt.figure(figsize=(5, 5))

    X_AXIS_ORDER = [ServerType.VM.value, ServerType.AWS.value]
    x_axis_labels = map(lambda state_value: server_type_name[state_value], X_AXIS_ORDER)

    HUE_ORDER = [
        DeceptionState.WITHOUT.value,
        DeceptionState.DEACTIVATED.value,
        DeceptionState.ACTIVE.value,
    ]
    hue_labels = map(lambda state_value: deception_state_name[state_value], HUE_ORDER)

    ax = sns.boxplot(
        data=df,
        x="server_type",
        order=X_AXIS_ORDER,
        y="time",
        hue="deception_state",
        hue_order=HUE_ORDER,
        showcaps=True,
        showfliers=show_outliers,
        linewidth=BOXPLOT_LINE_WIDTH,
    )
    sns.despine()

    ax.set_ylabel("RTT (ms)")
    ax.set_xlabel("Environment")
    ax.set_xticks(ticks=ax.get_xticks(), labels=x_axis_labels)
    ax.yaxis.grid(True, ls=(0, (1, 3)), which="major", color="grey", alpha=0.25, lw=1)
    ax.legend(loc="upper right", frameon=True, ncol=1)

    adjust_box_widths(fig, PLOT_WIDTH_AND_SPACE)
    for legend, label in zip(ax.get_legend().texts, hue_labels):
        legend.set_text(label)

    if image_name != "":
        save_plot(image_name)
    if show_outliers == False:
        plt.tight_layout()

    plt.show()


@matplotlib.rc_context(RC_CONTEXT)
def plot_boxplots_avg_case(
    df: pd.DataFrame, show_outliers=False, image_name="", sut_hue=[]
):
    df = df.copy(deep=True)
    fig = plt.figure(figsize=(7, 6.5))

    X_AXIS_ORDER = [ServerType.VM.value, ServerType.AWS.value]
    x_axis_labels = map(lambda state_value: server_type_name[state_value], X_AXIS_ORDER)

    if len(sut_hue) == 0:
        sut_hue = df[["deception_state", "sut_setup"]].apply(tuple, axis=1)
    else:
        sut_hue = sut_hue.copy(deep=True)

    SUT_DECEPTION_LABEL_ORDER = pd.Series(
        [
            (DeceptionState.WITHOUT.value, SutSetup.AVG_ADMIN.value),
            (DeceptionState.DEACTIVATED.value, SutSetup.AVG_ADMIN.value),
            (DeceptionState.ACTIVE.value, SutSetup.AVG_ADMIN.value),
            (DeceptionState.WITHOUT.value, SutSetup.AVG_HOME.value),
            (DeceptionState.DEACTIVATED.value, SutSetup.AVG_HOME.value),
            (DeceptionState.ACTIVE.value, SutSetup.AVG_HOME.value),
        ]
    )
    new_labels = []
    for old_label in SUT_DECEPTION_LABEL_ORDER:
        new_labels.append(
            f"{sut_setup_name[old_label[1]]} -- {deception_state_name[old_label[0]]}"
        )

    ax = sns.boxplot(
        data=df,
        x="server_type",
        order=X_AXIS_ORDER,
        y="time",
        hue=sut_hue,
        hue_order=SUT_DECEPTION_LABEL_ORDER,
        showcaps=True,
        showfliers=show_outliers,
        linewidth=BOXPLOT_LINE_WIDTH,
    )
    sns.despine()

    ax.set_ylabel("RTT (ms)")
    ax.set_xlabel("Environment")
    ax.set_xticks(ticks=ax.get_xticks(), labels=x_axis_labels)
    ax.yaxis.grid(True, ls=(0, (1, 3)), which="major", color="grey", alpha=0.25, lw=1)

    ax.legend(loc="center", bbox_to_anchor=(0.5, 1.05), ncol=2)

    adjust_box_widths(fig, PLOT_WIDTH_AND_SPACE)
    for legend, label in zip(ax.get_legend().texts, new_labels):
        legend.set_text(label)

    if image_name != "":
        save_plot(image_name)
    if show_outliers == False:
        plt.tight_layout()

    plt.show()


# ------------------------------------------------------ Lag plots ------------------------------------------------------


@matplotlib.rc_context(RC_CONTEXT)
def plot_lagplots(
    df: pd.DataFrame,
    filter_sut_setup: SutSetup = SutSetup.AVG_HOME,
    filter_server_type: ServerType = ServerType.AWS,
    filter_deception_state: DeceptionState = DeceptionState.ACTIVE,
    display_n: int = 50_000,
    image_name="",
    show_min=1,
    show_max=50,
):
    ROLLING_MEAN = 2_000
    df = df.copy(deep=True)
    plt.figure(figsize=(10, 4.5))

    # If the provided df is without warm up the Iteration starts >1.
    offset = df["iteration"][0]

    # select [0:display_n] of each dataset category (e.g., filter_ attributes)
    df = df.query("iteration <= @display_n + @offset -1")

    df.loc[:, "iteration"] = df.loc[:, "iteration"].map(lambda x: x - offset + 1)

    df = df.query("sut_setup == @filter_sut_setup.value")
    df = df.query("server_type == @filter_server_type.value")
    df = df.query("deception_state == @filter_deception_state.value")

    ax = sns.lineplot(
        data=df,
        x="iteration",
        y="time",
        linewidth=0.75,
        label=f"Request with config ["
        f"{server_type_name[filter_server_type.value]}"
        f", {sut_setup_name[filter_sut_setup.value]}"
        f", {deception_state_name[filter_deception_state.value]}"
        f"]",
    )

    pd.options.mode.chained_assignment = None  # default='warn'
    df.loc[:, "rolling_mean"] = df.loc[:, "time"].rolling(ROLLING_MEAN).mean()
    pd.options.mode.chained_assignment = "warn"

    plt.plot(
        df["iteration"], df["rolling_mean"], label=f"Rolling mean of {ROLLING_MEAN}"
    )

    ax.set_yscale("log")
    # preset min and max values if not predefined
    if show_min == 1 and show_max == 50:
        match df.iloc[0]["sut_setup"]:
            case SutSetup.AVG_ADMIN.value:
                if ServerType.AWS == filter_server_type:
                    show_min = 0
                    show_max = 40
                else:
                    show_min = 1
                    show_max = 30
            case SutSetup.AVG_HOME.value:
                if ServerType.AWS == filter_server_type:
                    show_min = 2
                    show_max = 30
                else:
                    show_min = 4
                    show_max = 50
            case SutSetup.WORST_JAVA.value:
                if ServerType.AWS == filter_server_type:
                    show_min = 1
                    show_max = 15
                else:
                    show_min = 2
                    show_max = 30
            case SutSetup.WORST_PYTHON.value:
                if ServerType.AWS == filter_server_type:
                    show_min = 2
                    show_max = 10
                else:
                    show_min = 6
                    show_max = 40

    if show_min == 0:
        show_min = 0.1
    ax.set_ylim(show_min, show_max)
    # set y axis tick label to [1,2,3,4,5,7,10,20,30,40,50,70,100,...]
    # respectful to show_min and show_max
    ax.set_yticks(generate_tick_arr(show_min, show_max))
    ax.get_yaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())

    ax.set_ylabel("RTT (ms)", fontsize=10)
    ax.set_xlabel("Iterations")
    ax.xaxis.grid(False)
    ax.legend(loc="lower right", frameon=False, fancybox=False, ncol=1)
    ax.yaxis.grid(True, ls=(0, (1, 3)), which="both", color="grey", alpha=0.25, lw=1)

    if image_name != "":
        save_plot(image_name)

    plt.tight_layout()
    plt.show()


@matplotlib.rc_context(RC_CONTEXT)
def plot_lagplot_generic(
    df: pd.DataFrame,
    max_value: int = 200_000,
    show_min: float = 1.0,
    show_max: float = 30.0,
    image_name: str = "",
    label_position: str = "lower right",
    label_name: str = "",
):
    ROLLING_MEAN = 2_000
    df = df.copy(deep=True)
    plt.figure(figsize=(10, 4.5))

    df["rolling_mean"] = df["time"].rolling(int(ROLLING_MEAN)).mean()

    if label_name == "":
        label_name = "Requests"
    ax = sns.lineplot(data=df[:max_value], x="iteration", y="time", label=label_name)

    plt.plot(
        df["iteration"][:max_value],
        df["rolling_mean"][:max_value],
        label=f"Rolling mean over {ROLLING_MEAN}",
    )

    ax.set_yscale("log")
    if show_min == 0:
        show_min = 0.1
    ax.set_ylim(show_min, show_max)
    ax.set_yticks(generate_tick_arr(show_min, show_max))
    ax.get_yaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())

    ax.set_ylabel("RTT (ms)", fontsize=10)
    ax.set_xlabel("Iterations")
    ax.xaxis.grid(False)
    ax.legend(loc=label_position, frameon=False, fancybox=False, ncol=1)
    ax.yaxis.grid(True, ls=(0, (1, 3)), which="both", color="grey", alpha=0.25, lw=1)

    if image_name != "":
        save_plot(image_name, tight=True)

    plt.tight_layout()
    plt.show()


# ------------------------------------------------------ Table plots ------------------------------------------------------


# Calculate "min", "max", "median", "average" for all 24 benchmarks (have to be included within df)
def statistic_numbers(current_number):
    return f"{'{:.2f}'.format(round(current_number, 2))}"


def statistic_percentages(current_number):
    return f"{'{:.2f}'.format(round(current_number*100, 4))}\%"


def calculate_general_statistic(df1: pd.DataFrame = None) -> ([[]], [[]]):
    absolut_numbers = []
    for sut in SutSetup:
        for server in ServerType:
            for ldpreload in DeceptionState:
                curr_df = df1.query(
                    "sut_setup==@sut.value and server_type==@server.value and deception_state==@ldpreload.value"
                )
                assert 40_000 == len(curr_df) or 160_000 == len(curr_df)

                name = (
                    f"{server_type_name[server.value]+',':~<4} "
                    f"{sut_setup_name_short[sut.value]+', '} "
                    f"{deception_state_name[ldpreload.value]}"
                )

                time_column = curr_df["time"]

                df_min = statistic_numbers(time_column.min())
                df_max = statistic_numbers(time_column.max())
                df_median = statistic_numbers(time_column.median())
                df_average = statistic_numbers(time_column.mean())

                absolut_numbers.append([name, df_min, df_max, df_median, df_average])

    return absolut_numbers


def calculate_general_statistic_v2(df1: pd.DataFrame = None) -> ([[]], [[]]):
    absolut_numbers = []
    for sut in SutSetup:
        for server in ServerType:
            baseline_median_float = 0.0

            for ldpreload in DeceptionState:
                curr_df = df1.query(
                    "sut_setup==@sut.value and server_type==@server.value and deception_state==@ldpreload.value"
                )
                assert 40_000 == len(curr_df) or 160_000 == len(curr_df)

                name = (
                    f"{server_type_name[server.value]+',':~<4} "
                    f"{sut_setup_name_short[sut.value]+', '} "
                    f"{deception_state_name[ldpreload.value]}"
                )

                time_column = curr_df["time"]

                df_min = statistic_numbers(time_column.min())
                df_max = statistic_numbers(time_column.max())
                df_median = statistic_numbers(time_column.median())
                df_average = statistic_numbers(time_column.mean())

                df_relative = ""
                if ldpreload == DeceptionState.WITHOUT:
                    baseline_median_float = time_column.median()
                    df_relative = "-"  # baseline
                else:
                    relative = (
                        time_column.median() - baseline_median_float
                    ) / baseline_median_float
                    df_relative = statistic_percentages(relative)

                absolut_numbers.append(
                    [name, df_min, df_max, df_median, df_average, df_relative]
                )

    return absolut_numbers


@matplotlib.rc_context(RC_CONTEXT)
def plot_stable_statistic(df: pd.DataFrame = None, image_name=""):
    df = df.copy(deep=True)
    plt.figure(figsize=(5, 4.5))

    column_headers = ["min", "max", "median", "average"]

    data = calculate_general_statistic(df)
    row_headers = [data[:1][0] for data in data]
    cell_text = [data[1:] for data in data]

    # Hide axes
    ax = plt.gca()
    ax.get_xaxis().set_visible(False)
    ax.get_yaxis().set_visible(False)
    plt.box(on=None)
    table = plt.table(
        cellText=cell_text,
        rowLabels=row_headers,
        colLabels=column_headers,
        loc="center",
    )
    table.scale(1, 1.5)

    if image_name != "":
        save_plot(image_name, tight=True)

    plt.show()


@matplotlib.rc_context(RC_CONTEXT)
def plot_stable_statistic_v2(df: pd.DataFrame = None, image_name=""):
    df = df.copy(deep=True)
    plt.figure(figsize=(5, 4.5))

    column_headers = ["min", "max", "median", "average", "relative"]

    data = calculate_general_statistic_v2(df)
    row_headers = [data[:1][0] for data in data]
    cell_text = [data[1:] for data in data]

    # Hide axes
    ax = plt.gca()
    ax.get_xaxis().set_visible(False)
    ax.get_yaxis().set_visible(False)
    plt.box(on=None)
    table = plt.table(
        cellText=cell_text,
        rowLabels=row_headers,
        colLabels=column_headers,
        loc="center",
    )
    table.scale(1, 1.5)

    if image_name != "":
        save_plot(image_name, tight=True)

    plt.show()


@matplotlib.rc_context(RC_CONTEXT)
def plot_relative_comparison_statistic(df: pd.DataFrame = None, image_name=""):
    df = df.copy(deep=True)
    plt.figure(figsize=(4, 3))

    # table
    column_headers = [
        deception_state_name[DeceptionState.WITHOUT.value],
        deception_state_name[DeceptionState.DEACTIVATED.value],
        deception_state_name[DeceptionState.ACTIVE.value],
    ]

    row_headers = []
    data = []

    relative_min = [100, 100, 100, 100]
    relative_max = [0, 0]
    relative_sum = [0, 0]
    relative_average = [0, 0]

    print("Create relative comparison statistic:")
    for sut in SutSetup:
        for server in ServerType:
            print(
                f"... for {server_type_name[server.value]}, {sut_setup_name_short[sut.value]}"
            )
            # if sut == SutSetup.AVG_HOME or sut == SutSetup.WORST_JAVA or sut == SutSetup.WORST_PYTHON:
            # continue

            deception_state = DeceptionState.WITHOUT
            baseline_df = curr_df = df.query(
                "sut_setup==@sut.value and server_type==@server.value and deception_state==@deception_state.value"
            )
            deception_state = DeceptionState.DEACTIVATED
            wires_deactivate_df = curr_df = df.query(
                "sut_setup==@sut.value and server_type==@server.value and deception_state==@deception_state.value"
            )
            deception_state = DeceptionState.ACTIVE
            wires_active_df = curr_df = df.query(
                "sut_setup==@sut.value and server_type==@server.value and deception_state==@deception_state.value"
            )

            baseline_median = baseline_df["time"].median()
            wires_false_median = wires_deactivate_df["time"].median()
            wires_true_median = wires_active_df["time"].median()

            wires_false_to_baseline_median_ratio = (
                wires_false_median - baseline_median
            ) / baseline_median

            wires_true_to_baseline_median_ratio = (
                wires_true_median - baseline_median
            ) / baseline_median

            relative_sum[0] = relative_sum[0] + wires_false_to_baseline_median_ratio
            relative_sum[1] = relative_sum[1] + wires_true_to_baseline_median_ratio

            if relative_min[0] > wires_false_to_baseline_median_ratio:
                relative_min[0] = wires_false_to_baseline_median_ratio
            if relative_min[1] > wires_true_to_baseline_median_ratio:
                relative_min[1] = wires_true_to_baseline_median_ratio

            if relative_max[0] < wires_false_to_baseline_median_ratio:
                relative_max[0] = wires_false_to_baseline_median_ratio
            if relative_max[1] < wires_true_to_baseline_median_ratio:
                relative_max[1] = wires_true_to_baseline_median_ratio

            name = (
                f"{server_type_name[server.value]+',':~<4} "
                f"{sut_setup_name_short[sut.value]+', '} "
            )
            row_headers.append(name)
            data.append(
                [
                    statistic_numbers(baseline_median),
                    statistic_percentages(wires_false_to_baseline_median_ratio),
                    statistic_percentages(wires_true_to_baseline_median_ratio),
                ]
            )

    # additional footer table presenting averages over relative overhead w/ wires=f and w/ wires=t
    TEST_SYSTEMS = 8  # 8 = 2x ServerType * 4 SutSetup
    relative_average[0] = relative_sum[0] / TEST_SYSTEMS
    relative_average[1] = relative_sum[1] / TEST_SYSTEMS

    row_headers.append("")
    row_headers.append("Minimum")
    row_headers.append("Maximum")
    row_headers.append("Average")

    data.append(["", "", ""])
    data.append(
        [
            "",
            statistic_percentages(relative_min[0]),
            statistic_percentages(relative_min[1]),
        ]
    )
    data.append(
        [
            "",
            statistic_percentages(relative_max[0]),
            statistic_percentages(relative_max[1]),
        ]
    )
    data.append(
        [
            "",
            statistic_percentages(relative_average[0]),
            statistic_percentages(relative_average[1]),
        ]
    )

    # Hide axes
    ax = plt.gca()
    ax.get_xaxis().set_visible(False)
    ax.get_yaxis().set_visible(False)
    plt.box(on=None)
    table = plt.table(
        cellText=data, rowLabels=row_headers, colLabels=column_headers, loc="center"
    )

    table.auto_set_font_size(False)
    table.set_fontsize(RC_CONTEXT["font.size"])
    table.scale(1.0, 1.5)

    cell_height = table[1, 1].get_bbox().y1

    plt.gca().add_patch(
        patches.Rectangle(
            (-0.18, cell_height * 1 + 0.02),
            1.1,
            cell_height * 0.875,
            edgecolor="white",
            facecolor="white",
            fill=True,
            clip_on=False,
        )
    )

    if image_name != "":
        save_plot(image_name, tight=True)

    plt.show()


@matplotlib.rc_context(RC_CONTEXT)
def plot_relative_comparison_statistic_v2(df: pd.DataFrame = None, image_name=""):
    df = df.copy(deep=True)
    plt.figure(figsize=(4, 1.5))

    # table
    column_headers = [
        deception_state_name[DeceptionState.DEACTIVATED.value],
        deception_state_name[DeceptionState.ACTIVE.value],
    ]
    row_headers = []
    data = []

    relative_min = [100, 100, 100, 100]
    relative_max = [0, 0]
    relative_sum = [0, 0]
    relative_average = [0, 0]

    print("Create relative comparison statistic:")
    for sut in SutSetup:
        for server in ServerType:
            print(
                f"... for {server_type_name[server.value]}, {sut_setup_name_short[sut.value]}"
            )
            # if sut == SutSetup.AVG_HOME or sut == SutSetup.WORST_JAVA or sut == SutSetup.WORST_PYTHON:
            # continue

            deception_state = DeceptionState.WITHOUT
            baseline_df = curr_df = df.query(
                "sut_setup==@sut.value and server_type==@server.value and deception_state==@deception_state.value"
            )
            deception_state = DeceptionState.DEACTIVATED
            wires_deactivate_df = curr_df = df.query(
                "sut_setup==@sut.value and server_type==@server.value and deception_state==@deception_state.value"
            )
            deception_state = DeceptionState.ACTIVE
            wires_active_df = curr_df = df.query(
                "sut_setup==@sut.value and server_type==@server.value and deception_state==@deception_state.value"
            )

            baseline_median = baseline_df["time"].median()
            wires_false_median = wires_deactivate_df["time"].median()
            wires_true_median = wires_active_df["time"].median()

            wires_false_to_baseline_median_ratio = (
                wires_false_median - baseline_median
            ) / baseline_median

            wires_true_to_baseline_median_ratio = (
                wires_true_median - baseline_median
            ) / baseline_median

            relative_sum[0] = relative_sum[0] + wires_false_to_baseline_median_ratio
            relative_sum[1] = relative_sum[1] + wires_true_to_baseline_median_ratio

            if relative_min[0] > wires_false_to_baseline_median_ratio:
                relative_min[0] = wires_false_to_baseline_median_ratio
            if relative_min[1] > wires_true_to_baseline_median_ratio:
                relative_min[1] = wires_true_to_baseline_median_ratio

            if relative_max[0] < wires_false_to_baseline_median_ratio:
                relative_max[0] = wires_false_to_baseline_median_ratio
            if relative_max[1] < wires_true_to_baseline_median_ratio:
                relative_max[1] = wires_true_to_baseline_median_ratio

    # additional footer table presenting averages over relative overhead w/ wires=f and w/ wires=t
    TEST_SYSTEMS = 8  # 8 = 2x ServerType * 4 SutSetup
    relative_average[0] = relative_sum[0] / TEST_SYSTEMS
    relative_average[1] = relative_sum[1] / TEST_SYSTEMS

    row_headers.append("Minimum")
    row_headers.append("Maximum")
    row_headers.append("Average")

    data.append(
        [
            statistic_percentages(relative_min[0]),
            statistic_percentages(relative_min[1]),
        ]
    )
    data.append(
        [
            statistic_percentages(relative_max[0]),
            statistic_percentages(relative_max[1]),
        ]
    )
    data.append(
        [
            statistic_percentages(relative_average[0]),
            statistic_percentages(relative_average[1]),
        ]
    )

    # Hide axes
    ax = plt.gca()
    ax.get_xaxis().set_visible(False)
    ax.get_yaxis().set_visible(False)
    plt.box(on=None)
    table = plt.table(
        cellText=data, rowLabels=row_headers, colLabels=column_headers, loc="center"
    )

    table.auto_set_font_size(False)
    table.set_fontsize(RC_CONTEXT["font.size"])
    table.scale(1.0, 1.5)

    if image_name != "":
        save_plot(image_name, tight=True)

    plt.show()
