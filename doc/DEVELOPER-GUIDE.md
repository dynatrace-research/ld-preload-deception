# Guide for developing the deception system

This document explains how to build, run and develop the `LD_PRELOAD` deception prototype.

## Getting started

When cloning this repository, make sure to also clone submodules.

```sh
git clone --recurse-submodules https://github.com/dynatrace-research/ld-preload-deception
```

If you forgot that, initialize and update them afterwards.

```sh
git submodule init
git submodule update
```

Before committing new code, ensure that you run the [pre-commit](https://pre-commit.com/) hooks.

```sh
pre-commit install
```

## 🗒️ Prerequisites

* [Docker](https://www.docker.com/products/docker-desktop)
* A system with **glibc version 2.28** so that the shared library is compatible with most newer applications, since glibc is backward-compatible, but not forward-compatible.

We further recommend to develop with [Visual Studio Code](https://code.visualstudio.com/)
and the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension installed.

## 🖥️ Development with Visual Studio Code

1. Open the project in VS Code
2. Use the Quick Open function and run the `Dev Containers: Reopen in Container` command.
   This command takes care of creating a container, mounting the project to `/workspaces`,
   setting the `LD_PRELOAD` environment variable to `/workspaces/bin/mount/deception.so`,
   and mounting [`/bin/mount/honeyaml.yaml`](../bin/mount/honeyaml.yaml) to `/var/opt/honeyaml.yaml`
   and [`bin/mount/deception.log`](../bin/mount/deception.log) to `/var/log/deception.log` in the container.
3. You can access the demo Python web server at [http://localhost:8080](http://localhost:8080)

If you need to develop for a different technology or glibc version, replace the Dockerfile in the `.devcontainer.json` file manually.

## ⚙ Build the shared library

Please refer to the specific [README.md](../src/README.md) of the shared module.

## ⬆ Use the shared library

Please refer to the example mentioned in the [README.md](../src/README.md) of the shared module.

## 🔨 Contribute to the shared library

Please note that you need to restart the processes in which you inject the shared library, after making changes and re-compiling the shared library.
During development, it may be helpful to start a simple backend (e.g., a simple Python webserver) that can be quickly restarted.

To debug and develop the hooked functions further, we resort to simply logging calls to library functions with the [`simpleLogger()`](../src/core/src/Utils.c) utility function.
The log level `loggerPriority` can be adjusted within the [GlobalVariables.c](../src/core/src/structs/GlobalVariables.c) file.
Here is a recommended log format:

```C
simpleLogger(LoggerPriority__INFO, "i-- somefunction(): some variables %d %s! i-- should only be a debug message and deleted with the PR\n", someNumber, someString);
simpleLogger(LoggerPriority__ERROR, "!-- readHoneYamlLine(): yaml_parser_parse error!\n");
simpleLogger(LoggerPriority__INFO, " [-] accept4: new relevant request detected on newSockFd: %d (linked to sockfd %d) \n", newSockfd, sockfd);
simpleLogger(LoggerPriority__INFO, "  |+ send: Http version string is not supported, send default buffer!\n");
```

If the dev containers are breaking locally, possibly due to a faulty shared library, use the `Dev Containers: Reopen Folder Locally` command and delete the `*.so` file.
If the library won't be found, the dynamic linker will just ignore it and not link anything.
Then, simply re-compile the shared library, start new demo containers, and try to fix the bug.

To verify changes to HTTP packets, a local browser should suffice.
