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

from locust import HttpUser, task, run_single_user
import numpy as np
import os
import csv
import time

# commandline options https://github.com/SvenskaSpel/locust-plugins#command-line-options
# locust -f any-locustfile-that-imports-locust_plugins.py --help

class BenchmarkSetup(HttpUser):
    # locust variable
    fixed_count = 1

    # object variables
    request_repetitions = 200_000

    # variables for csv export
    dir = "cvs_exports"
    filename = None  # will defined later since time when executed is included

    errors_during_test = 0
    csv_next_row_id = 0
    csv_logs_buffer_size = request_repetitions // 10
    csv_logs_colums_count = 2
    csv_logs_buffer = np.empty(
        [csv_logs_buffer_size, csv_logs_colums_count], dtype=float
    )

    def on_start(self):
        print("suite started")
        self.reset_buffer()

        # add handler to forward finished requests to csv exporter
        self.environment.events.request.add_listener(self.request_success_listener)

        # create directory for the benchmarks data (csv)
        if not os.path.exists(self.dir):
            os.mkdir(self.dir)

        # create benchmark file with name "benchmark_<time in millisec>.csv"
        current_time = time.time()
        filename = (
            f"benchmark_{time.strftime('%H-%M-%S', time.localtime(current_time))}"
            f"_{int(current_time * 1000)}.csv"
        )
        self.filename = os.path.join(self.dir, filename)
        if not os.path.exists(self.filename):
            print("PWD: " + os.getcwd() + "\n" + self.filename + "\n")
            open(self.filename, "w").close()

        # write csv header
        with open(self.filename, "a+") as f:
            f.write("Host: " + self.host + ", \n")
            f.write("start_time,response_time\n")

    def on_stop(self):
        self.write_statistic()
        self.reset_buffer()
        self.environment.events.request.remove_listener(self.request_success_listener)
        print("suite finished")

    def request_success_listener(
        self, start_time, response_time, response, exception, **_kwargs
    ):
        if exception or response.status_code != 200:
            self.errors_during_test += 1
            if self.errors_during_test % 1000 == 1:
                print(
                    f"Error: Host {self.host} returned HTTP status code {response.status_code}!"
                    f" Message: {self.errors_during_test}"
                )

        self.saveStatistic(start_time, response_time)

    def reset_buffer(self):
        self.errors_during_test = 0
        self.csv_next_row_id = 0
        self.csv_logs_buffer = np.empty(
            [self.csv_logs_buffer_size, self.csv_logs_colums_count], dtype=float
        )

    def saveStatistic(self, start_time, response_time):
        if self.csv_next_row_id >= self.csv_logs_buffer_size:
            self.write_statistic()
        self.csv_logs_buffer[self.csv_next_row_id] = [start_time, response_time]
        self.csv_next_row_id += 1

    def write_statistic(self):
        with open(self.filename, "a+") as f:
            csv_writer = csv.writer(f)

            for i in range(self.csv_next_row_id):
                csv_writer.writerow(self.csv_logs_buffer[i])
            self.csv_next_row_id = 0

    @task
    def benchmark(self):
        for i in range(self.request_repetitions):
            if "/benchmark" in self.host:
                self.client.post("", json={"message": f"Benchmark nr. {i}"})
            else:
                self.client.get("")

        # stop benchmark
        if hasattr(self, "debugFlag"):
            # stop user and with that exit run_single_user() function
            self.stop()
        else:
            self.environment.runner.quit()


# if launched directly, e.g. "python3 debugging.py", not "locust -f debugging.py"
if __name__ == "__main__":
    BenchmarkSetup.debugFlag = True
    BenchmarkSetup.host = "http://localhost:8080/admin"
    run_single_user(BenchmarkSetup)
