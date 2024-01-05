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

import os
import math
import numpy as np
import pathlib
import matplotlib.pyplot as plt
from matplotlib.patches import PathPatch


def get_csv_files_in_folder(folder_path) -> list:
    files = []
    for file in os.listdir(folder_path):
        current_path = os.path.join(folder_path, file)
        if os.path.isfile(current_path) and pathlib.Path(current_path).suffix == ".csv":
            files.append(file)
    return files


def adjust_box_widths(figure, factor):
    """
    Adjust the widths of a seaborn-generated boxplot.

    (c) 2019-07-09, by Eric (https://stackoverflow.com/a/56955897/927377)
    Dynatrace has not made any changes to this code. This code is supplied
    without warranty, and is available under Creative Commons BY-SA 4.0.

    :param fig: The figure object containing the boxplot
    :param factor: The factor by which to adjust the width
    """

    # iterating through Axes instances
    for ax in figure.axes:
        # iterating through axes artists:
        for c in ax.get_children():
            # searching for PathPatches
            if isinstance(c, PathPatch):
                # getting current width of box:
                p = c.get_path()
                verts = p.vertices
                verts_sub = verts[:-1]
                xmin = np.min(verts_sub[:, 0])
                xmax = np.max(verts_sub[:, 0])
                xmid = 0.5 * (xmin + xmax)
                xhalf = 0.5 * (xmax - xmin)

                # setting new width of box
                xmin_new = xmid - factor * xhalf
                xmax_new = xmid + factor * xhalf
                verts_sub[verts_sub[:, 0] == xmin, 0] = xmin_new
                verts_sub[verts_sub[:, 0] == xmax, 0] = xmax_new

                # setting new width of median line
                for line in ax.lines:
                    if np.all(line.get_xdata() == [xmin, xmax]):
                        line.set_xdata([xmin_new, xmax_new])


def save_plot(image_name, tight=False):
    graphics_folder = pathlib.Path("../graphics")
    graphic_path = graphics_folder.joinpath(image_name)
    if tight:
        plt.savefig(graphic_path, bbox_inches="tight")
    else:
        plt.savefig(graphic_path)


def generate_tick_arr(show_min: int, show_max: int) -> []:
    tick_arr = []
    i = math.ceil(show_min)
    if show_min > 5 and show_min < 7 or show_min > 7 and show_min < 10:
        tick_arr.append(show_min)

    while i <= show_max:
        if i == 0:
            i = i + 1
            continue
        elif i <= 5:
            tick_arr.append(i)
        elif i == 7:
            tick_arr.append(i)
        elif i <= 50 and i % 10 == 0:
            tick_arr.append(i)
        elif i == 70:
            tick_arr.append(i)
        elif i <= 500 and i % 100 == 0:
            tick_arr.append(i)
        elif i == 700:
            tick_arr.append(i)
        elif i <= 5000 and i % 1000 == 0:
            tick_arr.append(i)
        elif i == 7000:
            tick_arr.append(i)
        elif i % 10_000 == 0:
            tick_arr.append(i)
        i = i + 1
    return tick_arr
