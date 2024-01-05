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
import base64
import logging
from logging import Formatter, FileHandler
from flask import Flask, request, jsonify, json

app = Flask(__name__)

# setup logging
file_handler = FileHandler("/var/log/flaskTest.log")
file_handler.setLevel(logging.INFO)
file_handler.setFormatter(Formatter("%(asctime)s %(levelname)s: %(message)s "))
app.logger.setLevel(logging.INFO)
app.logger.addHandler(file_handler)


@app.route("/hello_world", methods=["GET"])
def hello_world():
    app.logger.info("/hello_world was called!")
    return "Hello world!\n"


@app.route("/add-deception", methods=["POST"])
def add_deception():
    """
    Parse the incoming manifest file of the pod and create json-patches. The patches will
    add the deception.so and the honeyaml.yaml file with the deception volume and they also
    add LD_PRELOAD environment variable with the reference to deception.so.
    """
    request_info = request.get_json()
    uid = request_info["request"].get("uid")
    try:
        type_of_action = request_info["request"]["object"]["kind"]
        container_list = request_info["request"]["object"]["spec"]["containers"]
        try:
            name = request_info["request"]["object"]["metadata"]["labels"][
                "app.kubernetes.io/name"
            ]
        except:
            name = container_list[0]["name"]

        app.logger.info(
            f"Object: {{ uid: {uid}, type: {type_of_action}, name: {name}}}"
        )
    except:
        app.logger.info(
            f"Could not parse object with JSON:{str(request_info['request']['object']['containers'])}"
        )
        return simple_admission_response(
            True,
            uid,
            "Failed to parse 'app.kubernetes.io/name' or pod doesn't specify a container!",
        )

    patches = []
    env_LD_PRELOAD = {"name": "LD_PRELOAD", "value": "/opt/deception/deception.so"}

    volume_claim_name = os.getenv(
        "PERSISTENT_VOLUME_CLAIM", "deception-persistent-volume-claim"
    )

    persistent_volume_claim = {
        "name": "deception-files",
        "persistentVolumeClaim": {"claimName": volume_claim_name},
    }

    VOLUME_MOUNTS_DECEPTION_FOLDER = {
        "mountPath": "/opt/deception",
        "name": "deception-files",
        "subPath": "",
    }

    VOLUME_MOUNT_HONEYMAL = {
        "mountPath": "/var/opt/honeyaml.yaml",
        "name": "deception-files",
        "subPath": "honeyaml.yaml",
    }

    # Try parsing volume definition. Might throw exception signaling that the attribute doesn't exist.
    try:
        request_info["request"]["object"]["spec"]["volumes"][0]

        patches.append(
            {"op": "add", "path": "/spec/volumes/-", "value": persistent_volume_claim}
        )
    except:
        patches.append(
            {"op": "add", "path": "/spec/volumes", "value": [persistent_volume_claim]}
        )

    # Try parsing container attributes "env"  and "volumeMounts". If attribute doesn't exist, the exception will
    # be caught and the attribute with an array will be generated. If the attribute exists, a new object will simply
    # appended to the existing array.
    i = 0
    while i < len(container_list):
        try:
            # Will through exception if it doesn't exist.
            request_info["request"]["object"]["spec"]["containers"][i]["env"]

            # Container has already a env variable set. Append new variable.
            patches.append(
                {
                    "op": "add",
                    "path": "/spec/containers/" + str(i) + "/env/-",
                    "value": env_LD_PRELOAD,
                }
            )
        except:
            # Container has no env variable set. Add new env array.
            patches.append(
                {
                    "op": "add",
                    "path": "/spec/containers/" + str(i) + "/env",
                    "value": [env_LD_PRELOAD],
                }
            )

        try:
            # Will through exception if it doesn't exist.
            request_info["request"]["object"]["spec"]["containers"][i]["volumeMounts"]

            patches.append(
                {
                    "op": "add",
                    "path": f"/spec/containers/{i}/volumeMounts/-",
                    "value": VOLUME_MOUNTS_DECEPTION_FOLDER,
                }
            )
            patches.append(
                {
                    "op": "add",
                    "path": f"/spec/containers/{i}/volumeMounts/-",
                    "value": VOLUME_MOUNT_HONEYMAL,
                }
            )
        except:
            patches.append(
                {
                    "op": "add",
                    "path": f"/spec/containers/{i}/volumeMounts",
                    "value": [
                        VOLUME_MOUNTS_DECEPTION_FOLDER,
                        VOLUME_MOUNT_HONEYMAL,
                    ],
                }
            )
        i += 1

    patch_string = json.dumps(patches)
    patch_bytes = patch_string.encode("ascii")
    patch_base64 = base64.b64encode(patch_bytes)

    return deceptive_admission_response(
        uid, "Environment Variable added!", patch_base64
    )


def simple_admission_response(allowed, uid, message):
    return jsonify(
        {
            "apiVersion": "admission.k8s.io/v1",
            "kind": "AdmissionReview",
            "response": {
                "allowed": allowed,
                "uid": uid,
                "status": {"message": message},
            },
        }
    )


def deceptive_admission_response(uid, message, patchBase64):
    return jsonify(
        {
            "apiVersion": "admission.k8s.io/v1",
            "kind": "AdmissionReview",
            "response": {
                "uid": uid,
                "allowed": True,
                "status": {"message": message},
                "patchType": "JSONPatch",
                "patch": patchBase64.decode(),
            },
        }
    )


pageNotFoundHtml = "<!doctype html>\n<html lang=en>\n<title>404 Not Found</title>\n<h1>Not Found</h1>\n<p>The requested URL was not found on the server. If you entered the URL manually please check your spelling and try again.</p>\n"
methodNotAllowedHtml = "<!doctype html>\n<html lang=en>\n<title>500 Internal Server Error</title>\n<h1>Internal Server Error</h1>\n<p>The server encountered an internal error and was unable to complete your request. Either the server is overloaded or there is an error in the application.</p>\n"


@app.errorhandler(404)
def page_not_found(e):
    app.logger.info("pageNotFound was called!")
    app.logger.info(e)
    return pageNotFoundHtml, 404


@app.errorhandler(405)
def method_not_allowed(e):
    app.logger.info("methodNotAllowed was called!")
    return methodNotAllowedHtml, 405


if __name__ == "__main__":
    app_port = int(os.environ.get("FLASK_RUN_PORT", 5001))
    app.run(host="0.0.0.0", port=app_port)
