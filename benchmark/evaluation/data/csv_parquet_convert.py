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
import pathlib
import pandas as pd
import numpy as np

PATH = "."

# if CONVERT_ALL_PARQUET_TO_CSV = False then all csv files will be converted to parquet
CONVERT_ALL_PARQUET_TO_CSV = True


def get_files(input_folder, extension):
    path = pathlib.Path(input_folder)

    file_paths = []
    for file in os.listdir(path):
        current_path = os.path.join(path, file)

        if os.path.isdir(current_path):
            new_file_paths = get_files(current_path, extension)
            file_paths = file_paths + new_file_paths

        if (
            os.path.isfile(current_path)
            and pathlib.Path(current_path).suffix == extension
        ):
            file_paths.append(current_path)

    return file_paths


def convert_all_files_csv_parquet(input_folder, parquet_to_csv: True):
    all_csv_files = []

    if parquet_to_csv:
        all_csv_files = get_files(input_folder, ".parquet")
    else:
        all_csv_files = get_files(input_folder, ".csv")

    if parquet_to_csv:
        print("Following list of files are transformed from .csv to .parquet:")
    else:
        print("Following list of files are transformed from .parquet to .csv:")
    print(all_csv_files)

    for file in all_csv_files:
        if parquet_to_csv:
            convert_parquet_to_csv(file)
        else:
            convert_csv_to_parquet(file)


def convert_csv_to_parquet(input_file_path):
    print(f"Transform file {os.path.basename(input_file_path)}")

    df = pd.read_csv(input_file_path, encoding="utf-8", header=[0, 1])
    input_file_name_without_extension = os.path.basename(
        os.path.splitext(input_file_path)[0]
    )
    output_file_path = pathlib.Path(
        os.path.dirname(input_file_path), input_file_name_without_extension + ".parquet"
    )

    df.to_parquet(output_file_path, index=False)


def convert_parquet_to_csv(input_file_path):
    print(f"Transform file {os.path.basename(input_file_path)}")

    df = pd.read_parquet(input_file_path)
    input_file_name_without_extension = os.path.basename(
        os.path.splitext(input_file_path)[0]
    )
    output_file_path = pathlib.Path(
        os.path.dirname(input_file_path), input_file_name_without_extension + ".csv"
    )

    header_origin_parquet: np.array = df.columns.values
    # example origin_parquet_header:
    #   ["('Host:  http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/admin', 'start_time')"
    #   "(' ', 'response_time')"]
    # CSV format (first line space at the end is important):
    #   Host:  http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/admin,
    #   start_time, response_time

    header_first_row = (
        header_origin_parquet[0]
        .replace("('", "")
        .replace("', '", "xxx")
        .replace("')", "")
    )
    header_first_0, header_second_0 = header_first_row.split("xxx")

    header_second_row = (
        header_origin_parquet[1]
        .replace("('", "")
        .replace("', '", "xxx")
        .replace("')", "")
    )
    header_first_1, header_second_1 = header_second_row.split("xxx")

    with open(output_file_path, "w+") as file:
        file.write(f"{header_first_0},{header_first_1}\n")
        file.write(f"{header_second_0},{header_second_1}\n")

    df.to_csv(output_file_path, encoding="utf-8", index=False, header=False, mode="a")


convert_all_files_csv_parquet(PATH, CONVERT_ALL_PARQUET_TO_CSV)
