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

#!/usr/bin/python
import os
import tempfile

from flask import Flask, request

app = Flask(__name__)
created_files = 0


@app.route("/")
def main():
    return "Welcome!\n"


@app.route("/benchmark", methods=["POST"])
def benchmark():
    message = request.json["message"]

    filename = create_and_write_file(message)
    print("File " + filename + " was created!")
    log_file_content(filename)
    delete_content(filename)

    return "Successfully made file operation!\n"


def create_and_write_file(message):
    global created_files
    file_id = created_files
    created_files += 1

    f = tempfile.NamedTemporaryFile(
        suffix=f"performanceFile{file_id}.txt", mode="w+", delete=False
    )
    filename = f.name
    f.write(message)
    f.close()
    return filename


def log_file_content(filename):
    f = open(filename, "r")
    print(f.readline())
    f.close()


def delete_content(filename):
    os.remove(filename)


if __name__ == "__main__":
    app_port = int(os.environ.get("FLASK_RUN_PORT", 5000))
    app.run(host="0.0.0.0", port=app_port)
