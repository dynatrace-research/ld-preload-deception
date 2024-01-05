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
import pandas as pd
import pathlib

from constants_and_classes import *
from helper_functions import get_csv_files_in_folder


def load_benchmark_data(basepath: str) -> BenchmarkSuite:
    basepath = pathlib.Path(basepath)

    vm_avg_path = basepath.joinpath("vm", "avg-case")
    vm_worst_path = basepath.joinpath("vm", "worst-case")
    aws_avg_path = basepath.joinpath("aws", "avg-case")
    aws_worst_path = basepath.joinpath("aws", "worst-case")

    # save all file-names of the folder inside an string list

    vm_avg = get_csv_files_in_folder(vm_avg_path)
    vm_worst = get_csv_files_in_folder(vm_worst_path)
    aws_avg = get_csv_files_in_folder(aws_avg_path)
    aws_worst = get_csv_files_in_folder(aws_worst_path)

    if (
        len(vm_avg) != 6
        or len(vm_worst) != 6
        or len(aws_avg) != 6
        or len(aws_worst) != 6
    ):
        raise Exception(
            f"One of the folder of the benchmark does not have the inteded .csv count of 6.\n"
            f"File count of folders: VM_AVG={len(vm_avg)}, VM_worst={len(vm_worst)},"
            f"AWS_worst={len(aws_avg)}, AWS_worst={len(aws_worst)}.\n\n"
            f"Did you transformed all parquet files of {basepath} into csv files?"
        )

    # Since the worst-case VM benchmark of the first benchmark suite (./data/benchmark_suite_1) were deployed directly to Docker,
    # their was e different order. Hence, one have to manually switch out the block below with the current [ServerType.VM, SutSetup.WORST_X]
    # block to load suite 1.
    #    Benchmark(ServerType.VM, SutSetup.WORST_JAVA, DeceptionState.ACTIVE,         vm_worst_path.joinpath(vm_worst[1])),
    #    Benchmark(ServerType.VM, SutSetup.WORST_JAVA, DeceptionState.DEACTIVATED,    vm_worst_path.joinpath(vm_worst[4])),
    #    Benchmark(ServerType.VM, SutSetup.WORST_JAVA, DeceptionState.WITHOUT,        vm_worst_path.joinpath(vm_worst[0])),
    #    Benchmark(ServerType.VM, SutSetup.WORST_PYTHON, DeceptionState.ACTIVE,       vm_worst_path.joinpath(vm_worst[3])),
    #    Benchmark(ServerType.VM, SutSetup.WORST_PYTHON, DeceptionState.DEACTIVATED,  vm_worst_path.joinpath(vm_worst[5])),
    #    Benchmark(ServerType.VM, SutSetup.WORST_PYTHON, DeceptionState.WITHOUT,      vm_worst_path.joinpath(vm_worst[2])),

    # ordering of the file indexes (e.g., vm_avg[INDEX]) is defined by the order in
    # which the systems are deployed and executed within /doc/BENCHMARK-GUIDE.md
    # fmt: off
    benchmark_suite: BenchmarkSuite = BenchmarkSuite([
        Benchmark(ServerType.VM, SutSetup.AVG_ADMIN, DeceptionState.ACTIVE,      vm_avg_path.joinpath(vm_avg[0])),
        Benchmark(ServerType.VM, SutSetup.AVG_ADMIN, DeceptionState.DEACTIVATED, vm_avg_path.joinpath(vm_avg[2])),
        Benchmark(ServerType.VM, SutSetup.AVG_ADMIN, DeceptionState.WITHOUT,     vm_avg_path.joinpath(vm_avg[4])),
        Benchmark(ServerType.VM, SutSetup.AVG_HOME, DeceptionState.ACTIVE,       vm_avg_path.joinpath(vm_avg[1])),
        Benchmark(ServerType.VM, SutSetup.AVG_HOME, DeceptionState.DEACTIVATED,  vm_avg_path.joinpath(vm_avg[3])),
        Benchmark(ServerType.VM, SutSetup.AVG_HOME, DeceptionState.WITHOUT,      vm_avg_path.joinpath(vm_avg[5])),
        Benchmark(ServerType.VM, SutSetup.WORST_JAVA, DeceptionState.ACTIVE,         vm_worst_path.joinpath(vm_worst[0])),
        Benchmark(ServerType.VM, SutSetup.WORST_JAVA, DeceptionState.DEACTIVATED,    vm_worst_path.joinpath(vm_worst[2])),
        Benchmark(ServerType.VM, SutSetup.WORST_JAVA, DeceptionState.WITHOUT,        vm_worst_path.joinpath(vm_worst[4])),
        Benchmark(ServerType.VM, SutSetup.WORST_PYTHON, DeceptionState.ACTIVE,       vm_worst_path.joinpath(vm_worst[1])),
        Benchmark(ServerType.VM, SutSetup.WORST_PYTHON, DeceptionState.DEACTIVATED,  vm_worst_path.joinpath(vm_worst[3])),
        Benchmark(ServerType.VM, SutSetup.WORST_PYTHON, DeceptionState.WITHOUT,      vm_worst_path.joinpath(vm_worst[5])),
        Benchmark(ServerType.AWS, SutSetup.AVG_ADMIN, DeceptionState.ACTIVE,       aws_avg_path.joinpath(aws_avg[0])),
        Benchmark(ServerType.AWS, SutSetup.AVG_ADMIN, DeceptionState.DEACTIVATED,  aws_avg_path.joinpath(aws_avg[2])),
        Benchmark(ServerType.AWS, SutSetup.AVG_ADMIN, DeceptionState.WITHOUT,      aws_avg_path.joinpath(aws_avg[4])),
        Benchmark(ServerType.AWS, SutSetup.AVG_HOME, DeceptionState.ACTIVE,        aws_avg_path.joinpath(aws_avg[1])),
        Benchmark(ServerType.AWS, SutSetup.AVG_HOME, DeceptionState.DEACTIVATED,   aws_avg_path.joinpath(aws_avg[3])),
        Benchmark(ServerType.AWS, SutSetup.AVG_HOME, DeceptionState.WITHOUT,       aws_avg_path.joinpath(aws_avg[5])),
        Benchmark(ServerType.AWS, SutSetup.WORST_JAVA, DeceptionState.ACTIVE,         aws_worst_path.joinpath(aws_worst[0])),
        Benchmark(ServerType.AWS, SutSetup.WORST_JAVA, DeceptionState.DEACTIVATED,    aws_worst_path.joinpath(aws_worst[4])),
        Benchmark(ServerType.AWS, SutSetup.WORST_JAVA, DeceptionState.WITHOUT,        aws_worst_path.joinpath(aws_worst[2])),
        Benchmark(ServerType.AWS, SutSetup.WORST_PYTHON, DeceptionState.ACTIVE,       aws_worst_path.joinpath(aws_worst[1])),
        Benchmark(ServerType.AWS, SutSetup.WORST_PYTHON, DeceptionState.DEACTIVATED,  aws_worst_path.joinpath(aws_worst[5])),
        Benchmark(ServerType.AWS, SutSetup.WORST_PYTHON, DeceptionState.WITHOUT,      aws_worst_path.joinpath(aws_worst[3])),
    ])
    # fmt: on

    load_data_into_benchmark_suite(benchmark_suite)

    return benchmark_suite


def load_data_into_benchmark_suite(benchmark_suite: BenchmarkSuite) -> None:
    all_dataframes = []
    teastore_df = []
    python_df = []
    java_df = []

    for benchmark in benchmark_suite.benchmarks:
        benchmark.response_times = pd.read_csv(benchmark.data_file_path, header=1)[
            "response_time"
        ]
        benchmark.start_time = pd.read_csv(benchmark.data_file_path, header=1)[
            "start_time"
        ]
        benchmark.standard_derivation = benchmark.response_times.std()

        assert (
            len(benchmark.response_times) == 50_000
            or len(benchmark.response_times) == 200_000
        )
        assert (
            benchmark.response_times.mean() > 0 and benchmark.response_times.mean() < 20
        )

        benchmark_df = pd.DataFrame.from_dict(
            {
                "time": benchmark.response_times,
                "server_type": benchmark.server_type.value,
                "deception_state": benchmark.deception_state.value,
                "sut_setup": benchmark.sut_setup.value,
                "filename": os.path.basename(benchmark.data_file_path),
            }
        )
        benchmark_df.reset_index(inplace=True)
        benchmark_df.rename(columns={"index": "iteration"}, inplace=True)

        all_dataframes.append(benchmark_df)
        benchmark.panda_df = benchmark_df
        if benchmark.sut_setup == SutSetup.WORST_PYTHON:
            python_df.append(benchmark_df)
        if benchmark.sut_setup == SutSetup.WORST_JAVA:
            java_df.append(benchmark_df)
        if (
            benchmark.sut_setup == SutSetup.AVG_ADMIN
            or benchmark.sut_setup == SutSetup.AVG_HOME
        ):
            teastore_df.append(benchmark_df)

    benchmark_suite.panda_df = pd.concat(all_dataframes, ignore_index=True)
    benchmark_suite.teastore_df = pd.concat(teastore_df, ignore_index=True)
    benchmark_suite.python_df = pd.concat(python_df, ignore_index=True)
    benchmark_suite.java_df = pd.concat(java_df, ignore_index=True)


def calc_warm_up_dfs(benchmark_suite: BenchmarkSuite, warm_up) -> None:
    all_df = []
    teastore_df = []
    python_df = []
    java_df = []

    for benchmark in benchmark_suite.benchmarks:
        file_name = os.path.basename(benchmark.data_file_path)
        # create data frame beginning at index @warm_up
        benchmark_df = pd.DataFrame.from_dict(
            {
                "time": benchmark.response_times[warm_up:],
                "server_type": benchmark.server_type.value,
                "deception_state": benchmark.deception_state.value,
                "sut_setup": benchmark.sut_setup.value,
                "filename": os.path.basename(file_name),
            }
        )

        benchmark.data_file_path

        benchmark_df.reset_index(inplace=True)
        benchmark_df.rename(columns={"index": "iteration"}, inplace=True)

        all_df.append(benchmark_df)
        if benchmark.sut_setup == SutSetup.WORST_PYTHON:
            python_df.append(benchmark_df)
        if benchmark.sut_setup == SutSetup.WORST_JAVA:
            java_df.append(benchmark_df)
        if (
            benchmark.sut_setup == SutSetup.AVG_ADMIN
            or benchmark.sut_setup == SutSetup.AVG_HOME
        ):
            teastore_df.append(benchmark_df)

    benchmark_suite.panda_warm_up_df = pd.concat(all_df, ignore_index=True)
    benchmark_suite.teastore_warm_up_df = pd.concat(teastore_df, ignore_index=True)
    benchmark_suite.python_warm_up_df = pd.concat(python_df, ignore_index=True)
    benchmark_suite.java_warm_up_df = pd.concat(java_df, ignore_index=True)
    # for TeaStore boxplot
    benchmark_suite.testore_warm_hue = benchmark_suite.teastore_warm_up_df[
        ["deception_state", "sut_setup"]
    ].apply(tuple, axis=1)


def csv_to_df(filepath: str, warm_up: int = 0) -> pd.DataFrame:
    assert filepath.endswith(".csv")

    file_path = pathlib.Path(filepath)
    response_times = pd.read_csv(file_path, header=1)["response_time"]

    assert len(response_times) == 50_000 or len(response_times) == 200_000
    assert response_times.mean() > 0 or response_times.mean() > 100

    df = pd.DataFrame.from_dict(
        {"time": response_times[warm_up:], "file_name": os.path.basename(file_path)}
    )
    df.reset_index(inplace=True)
    df.rename(columns={"index": "iteration"}, inplace=True)
    return df
