# 🧙‍♂️💫 Making HTTP packets deceptive with `LD_PRELOAD`

This prototype hooks into the `send` and `receive` functions of `glibc` to insert deceptive elements into HTTP packets.
In particular, this prototype is able to modify the response code or any HTTP response header.
Most importantly, this method works without deploying any firewall or proxy component, and without recompiling the original application.

[Most modern applications rely on the `glibc` library](https://linux.die.net/man/2/) to communicate with the operating system, i.e., to read files, or send and receive network packets.
The `LD_PRELOAD` environment variable allows you to specify a list of shared libraries to load before any other libraries, including `glibc`.
This makes it possible to hook into existing `glibc` functions and change their implementation.
This is commonly known as the [`LD_PRELOAD` trick](https://www.goldsborough.me/c/low-level/kernel/2016/08/29/16-48-53-the_-ld_preload-_trick/).

To make this prototype as autonomous as possible, we also describe how to deploy a Kubernetes operator that automatically sets the `LD_PRELOAD` variable,
effectivley allowing automatic, transparent, and flexible deception for all workloads in your cluster.

## Getting started

- ⚙ If you want to build, run, and test the native shared library locally, read the [README](./src/README.md) on the `LD_PRELOAD` module
- 🖥️ If you want to setup your local development environment and test the prototype with containers, read the [DEVELOPER-GUIDE](./doc/DEVELOPER-GUIDE.md)
- ⛵ If you want to deploy this prototype as an operator to Kubernetes, either locally or to AWS, read the [KUBERNETES-GUIDE](./doc/KUBERNETES-GUIDE.md)
- 📊 If you want to conduct or reproduce our performance benchmarks, read the [BENCHMARK-GUIDE](./doc/BENCHMARK-GUIDE.md)

## Folder structure

- [`.devcontainer`](./.devcontainer) holds demo containers and configuration for easy local development
- [`benchmark`](/benchmark) holds code for benchmarking the performance of the prototype and also some [benchmarking results](./benchmark/evaluation/data/)
- [`bin`](./bin) holds pre-built binaries of the [`deception.so`](./bin/mount/deception.so) prototype
- [`doc`](./doc) holds additional documentation, as referenced above
- [`k8s-manifests`](./k8s-manifests) holds manifests for deploying this prototype to Kubernetes environments
- [`src`](./src) holds the source code of the native shared library
- [`third_party`](./third_party) holds third-party dependencies for the native library

## Contributors

**Note: This project is not officially supported by Dynatrace.**

For general questions or inquiries please get in touch with one of the following individuals.

| [<img src="https://github.com/kern17.png" width="100"/>](https://github.com/kern17) | [<img src="https://github.com/blu3r4y.png" width="100"/>](https://github.com/blu3r4y) |
| :-: | :-: |
| [Patrick Kern](https://github.com/kern17) | [Mario Kahlhofer](https://github.com/blu3r4y) |
