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

from enum import Enum
import pandas as pd
from typing import TypeAlias

List: TypeAlias = list


class ServerType(Enum):
    VM = 0
    AWS = 1


server_type_name = {
    ServerType.VM.value: "VM",
    ServerType.AWS.value: "AWS",
}


class SutSetup(Enum):
    AVG_ADMIN = 0
    AVG_HOME = 1
    WORST_JAVA = 2
    WORST_PYTHON = 3


sut_setup_name = {
    SutSetup.AVG_ADMIN.value: "/admin",
    SutSetup.AVG_HOME.value: "/tools.descartes.teastore.webui/",
    SutSetup.WORST_JAVA.value: "/benchmark Java",
    SutSetup.WORST_PYTHON.value: "/benchmark Python",
}
sut_setup_name_short = {
    SutSetup.AVG_ADMIN.value: "/admin",
    SutSetup.AVG_HOME.value: "/home/",
    SutSetup.WORST_JAVA.value: "Java",
    SutSetup.WORST_PYTHON.value: "Python",
}
sut_setup_file_name = {
    SutSetup.AVG_ADMIN.value: "admin",
    SutSetup.AVG_HOME.value: "home",
    SutSetup.WORST_JAVA.value: "java",
    SutSetup.WORST_PYTHON.value: "python",
}


class DeceptionState(Enum):
    WITHOUT = 0  # DECEPTION variable is not set
    DEACTIVATED = 1  # DECEPTION set, honey-wires set to true
    ACTIVE = 2  # DECEPTION set, honey-wires set to false


deception_state_name = {
    DeceptionState.WITHOUT.value: "w/o wires",  # "without deception",
    DeceptionState.DEACTIVATED.value: "w/ wires=f",  # "with deactivated deception",
    DeceptionState.ACTIVE.value: "w/ wires=t",  # "with activated deception",
}
deception_state_file_name = {
    DeceptionState.WITHOUT.value: "without_wires",
    DeceptionState.DEACTIVATED.value: "with_wires_false",
    DeceptionState.ACTIVE.value: "with_wires_true",
}


class Benchmark:
    def __init__(
        self,
        server_type: ServerType,
        sut_setup: SutSetup,
        deception_state: DeceptionState,
        data_file_path: str,
        response_times: List[int] = None,
        start_time: List[int] = None,
        panda_df=None,
        standard_derivation: List[int] = None,
    ):
        self.server_type = server_type
        self.sut_setup = sut_setup
        self.deception_state = deception_state
        self.data_file_path = data_file_path
        self.response_times = response_times or []
        self.start_time = start_time or []
        self.panda_df = panda_df or []
        self.standard_derivation = standard_derivation or []

    def __str__(self):
        return f"{self.server_type}, {self.sut_setup}, {self.deception_state}, {self.data_file_path}, {self.response_times}, {self.start_time}"


class BenchmarkSuite:
    def __init__(
        self,
        benchmarks: List[Benchmark] = None,
        panda_df: pd.DataFrame = None,
        teastore_df: pd.DataFrame = None,
        python_df: pd.DataFrame = None,
        java_df: pd.DataFrame = None,
        warm_up=0,
        panda_warm_up_df=None,
        teastore_warm_up_df=None,
        testore_warm_up_hue=None,
        python_warm_up_df=None,
        java_warm_up_df=None,
    ):
        self.benchmarks = benchmarks or []
        # --- data frames (df) are defined within the load_data_into_benchmark_suite() method---
        # panda_df: all data in one frame
        self.panda_df = panda_df or []
        # teastore_df - 12 boxplots: 2 (ServerType) * 3 (DeceptionState) * 2 (SutSetup.AVG_ADMIN, SutSetup.AVG_HOME)
        self.teastore_df = teastore_df or []
        # python_df - 6 boxplots: 2 (ServerType) * 3 (DeceptionState) * 1 (SutSetup.WORST_JAVA)
        self.python_df = python_df or []
        # java_df:  contains 6 boxplots: 2 (ServerType) * 3 (DeceptionState) * 1 (SutSetup.WORST_PYTHON)
        self.java_df = java_df or []

        self.warm_up = warm_up
        self.panda_warm_up_df = panda_warm_up_df or []
        self.teastore_warm_up_df = teastore_warm_up_df or []
        self.python_warm_up_df = python_warm_up_df or []
        self.testore_warm_hue = testore_warm_up_hue or []
        self.java_warm_up_df = java_warm_up_df or []

    def __str__(self):
        tostring = "BenchmarkSuite:{\n"
        for benchmark in self.benchmarks:
            tostring = tostring + "- " + str(benchmark.__str__()) + "\n"
        tostring = tostring + "}"
        return tostring
