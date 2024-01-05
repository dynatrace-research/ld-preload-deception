# Guide for conducting benchmarks

This document explains how to deploy and conduct benchmarks.
Two different systems under tests (SUTs) were chosen to evaluate the prototype:

- An **"average-case" scenario**, represented by [TeaStore](https://github.com/DescartesResearch/TeaStore), which is a Java application
- A **"worst-case" scenario**, represented by a custom Java and Python application, which was guided by the benchmark scenario
  done by [Lopez et al. (2017) in "A Survey on Function and System Call Hooking Approaches"](https://doi.org/10.1007/s41635-017-0013-2).
  This scenario will do file manipulations and handle TCP packets, which both utilize the same `libc` functions that we hook.

## Prerequisites

For a **local deployment with [Docker](https://www.docker.com/products/docker-desktop)**, only Docker is needed.

For a **local deployment with [Kind](https://kind.sigs.k8s.io/)**, the following tools are needed:

- [Kind](https://kind.sigs.k8s.io/)
- [Docker](https://www.docker.com/products/docker-desktop)
- [kubectl](https://kubernetes.io/docs/tasks/tools/)
- [Skaffold](https://skaffold.dev/docs/install/)
- [Kustomize](https://kubectl.docs.kubernetes.io/installation/kustomize/)

For a **cloud deployment to AWS**, the following tools are needed:

- [AWS CLI](https://aws.amazon.com/cli/)
- An existing EKS cluster with
  - the [Amazon EFS CSI Driver](https://docs.aws.amazon.com/eks/latest/userguide/efs-csi.html) installed
  - the [storageclass-efs.yaml](../k8s-manifests/cloud-service-provider/aws/efs-storageclass/storageclass-efs.yaml) manifest deployed
  - an [Elastic File System](https://aws.amazon.com/efs/) that is accessible for the cluster

Before applying the EFS manifests, replace the `<fs-id>` placeholder with your EFS storage id in the following files:

- [deception-persistent-volume/persistent-volume.yaml](../k8s-manifests/cloud-service-provider/aws/benchmark/deception-persistent-volume/persistent-volume.yaml)
- [locust/persistent-volume.yaml](../k8s-manifests/cloud-service-provider/aws/benchmark/test-bench/locust/persistent-volume.yaml)

## Benchmarking scenarios

We test all combinations of three independent conditions, forming 24 ( = 2 x 3 x 4 ) benchmark runs in total:

- **Condition: Server environment**
  - **VM:** With Docker or Kind on a Intel Xeon E5-2680 v3 VM with 4 cores, 64GB RAM, running Ubuntu 22.04.3
  - **AWS:** Inside an AWS EKS cluster, with t3.medium nodes
- **Condition `LD_PRELOAD` state**
  - **w/o wires:** The control group, an unmodified application
  - **w/ wires=f:** The application with our `LD_PRELOAD` module injected, but disabled
  - **w/ wires=t:** The application with our `LD_PRELOAD` module injected and enabled
- **Condition: Use-case coverage**
  - **"average-case" TeaStore**: Browsing `/admin` and `/tools.descartes.teastore.webui`
  - **"worst-case" Python and Java applications**: Browsing `/benchmark`

Each scenario needs to be deployed manually and the configuration has to be adapted accordingly.
To keep each scenario reproducible, we purge all the containers and redeploy every component again.

### HoneYAML configuration

The [honeyaml.yaml](../bin/mount/honeyaml.yaml) configuration file defines the current state of the deception.
A honeywire (or, wire, in short) is a single "change operation" that the deception system should make to an HTTP response.
Currently, there are two honeywires implemented:

- **`response-code` deception:** Overwrites the status code in HTTP responses, e.g., replaces the original status with `200 OK`.
  This modification can further be conditioned to only modify responses to requests for certain URLs.
- **`http-header` deception:** Replaces a header attribute in HTTP responses, e.g., replaces the `Server` header with a seemingly vulnerable `Apache/1.0.3 (Debian)` value.
  Note that the overwritten text will be padded with spaces to the length of the original header field.

Changes to the configuration file are automatically applied at runtime. No application restart is needed.

We use the following configuration for testing the "average-case" with TeaStore.

```yaml
honeywire:
  kind: response-code
  enabled: true
  name: status-code-admin-path
  description: Returns @value instead of the original status code when @path is requested
  operations:
    - op: replace-status-code
      value: 200
      condition:
        - path: /admin
---
honeywire:
  kind: http-header
  enabled: true
  name: http-header-server-replace
  description: Changes the Server attribute of @key to @value if the key exists
  operations:
    - op: replace-inplace
      key: Server
      value: "Apache/1.0.3 (Debian)"
```

We use the following configuration for testing the "worst-case" with our custom Python and Java application.

```yaml
honeywire:
  kind: response-code
  enabled: true
  name: status-code-admin-path
  description: Returns @value instead of the original status code when @path is requested
  operations:
    - op: replace-status-code
      value: 203
      condition:
        - path: /benchmark
---
honeywire:
  kind: http-header
  enabled: true
  name: http-header-server-replace
  description: Changes the Server attribute of @key to @value if the key exists.
  operations:
    - op: replace-inplace
      key: Date
      value: "Sun, 01 Jan 2023 11:55:00 GMT"
```

## Conducting benchmarks

This section roughly describes how to manually conduct the benchmarks for the aforementioned scenarios.
Please apply good common sense when attempting to reproduce the following.

### 1️⃣ Local deployment with Kind

First, launch a local Kubernetes cluster.
The `cluster-config.yaml` takes care of mounting the folder with the pre-built `deception.so` and `honeyyaml.yaml` files.

```sh
kind create cluster --name deception --config ./k8s-manifests/localdev/kind/cluster-config.yaml
```

#### Benchmarking the "average-case" scenario with Kind

```sh
# setup system, i.e., create namespaces, operator, and test bench
skaffold run -p setup-kind -f ./skaffold-benchmark.yaml

# deploy benchmark-deception-sut-with (operator will inject LD_PRELOAD automatically in namespace benchmark-deception-sut-with)
kubectl create -n benchmark-deception-sut-with -f https://raw.githubusercontent.com/DescartesResearch/TeaStore/master/examples/kubernetes/teastore-clusterip.yaml

# forward test bench port
kubectl port-forward -n deception service/benchmark-tb-locust 8089:8089

# manually change the `honeyyaml.yaml` file to the "average-case" scenario

# connect to http://localhost:8089 and start a benchmark run for /admin w/ wires=t to the following host:
# http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/admin

# re-deploy benchmark-deception-sut-with and restart tb-locust
kubectl delete deployments,pods,services -n benchmark-deception-sut-with -l app=teastore
kubectl create -n benchmark-deception-sut-with -f https://raw.githubusercontent.com/DescartesResearch/TeaStore/master/examples/kubernetes/teastore-clusterip.yaml
kubectl delete pod -n deception -l app.kubernetes.io/name=tb-locust

# connect to http://localhost:8089 and start a benchmark run for /tools.descartes.teastore.webui/ wires=t to the following host:
# http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/tools.descartes.teastore.webui/

# manually disable honeywires by setting `enabled: false` in the `honeyyaml.yaml` file

# re-deploy benchmark-deception-sut-with and restart tb-locust
kubectl delete deployments,pods,services -n benchmark-deception-sut-with -l app=teastore
kubectl create -n benchmark-deception-sut-with -f https://raw.githubusercontent.com/DescartesResearch/TeaStore/master/examples/kubernetes/teastore-clusterip.yaml
kubectl delete pod -n deception -l app.kubernetes.io/name=tb-locust

# connect to http://localhost:8089 and start a benchmark run for /admin w/ wires=f to the following host:
# http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/admin

# re-deploy benchmark-deception-sut-with and restart tb-locust
kubectl delete deployments,pods,services -n benchmark-deception-sut-with -l app=teastore
kubectl create -n benchmark-deception-sut-with -f https://raw.githubusercontent.com/DescartesResearch/TeaStore/master/examples/kubernetes/teastore-clusterip.yaml
kubectl delete pod -n deception -l app.kubernetes.io/name=tb-locust

# connect to http://localhost:8089 and start a benchmark run for /tools.descartes.teastore.webui/ w/ wires=f to the following host:
# http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/tools.descartes.teastore.webui/

# delete SUT and restart tb-locust
kubectl delete deployments,pods,services -n benchmark-deception-sut-with -l app=teastore
kubectl delete pod -n deception -l app.kubernetes.io/name=tb-locust

# deploy SUT without deception (w/o wires)
kubectl create -n benchmark-deception-sut-without -f https://raw.githubusercontent.com/DescartesResearch/TeaStore/master/examples/kubernetes/teastore-clusterip.yaml

# connect to http://localhost:8089 and start a benchmark run for /admin w/o wires to the following host:
# http://teastore-webui.benchmark-deception-sut-without.svc.cluster.local:8080/admin

# re-deploy benchmark-deception-sut-without and restart tb-locust
kubectl delete deployments,pods,services -n benchmark-deception-sut-without -l app=teastore
kubectl create -n benchmark-deception-sut-without -f https://raw.githubusercontent.com/DescartesResearch/TeaStore/master/examples/kubernetes/teastore-clusterip.yaml
kubectl delete pod -n deception -l app.kubernetes.io/name=tb-locust

# connect to http://localhost:8089 and start a benchmark run for /tools.descartes.teastore.webui/ w/o wires to the following host:
# http://teastore-webui.benchmark-deception-sut-without.svc.cluster.local:8080/tools.descartes.teastore.webui/

# copy benchmarking results from the cluster
kubectl get pod -n deception --selector="app.kubernetes.io/name=tb-locust"
kubectl cp deception/${POD-NAME}:/opt/locust/cvs_exports /$(pwd)/bin/mount/benchmark_exports/

# delete SUT and restart tb-locust
kubectl delete deployments,pods,services -n benchmark-deception-sut-without -l app=teastore
```

#### Benchmarking the "worst-case" scenario with Kind

```sh
# setup system, i.e., create namespaces, operator, and test bench
skaffold run -p setup-kind -f ./skaffold-benchmark.yaml

# forward test bench port
kubectl port-forward -n deception service/benchmark-tb-locust 8089:8089

# manually change the `honeyyaml.yaml` file to the "worst-case" scenario

# deploy the "worst-case" scenario SUT with LD_PRELOAD
skaffold run -p deploy-sut -f ./skaffold-benchmark.yaml -n benchmark-deception-sut-with

# connect to http://localhost:8089 and start a benchmark run for /benchmark wires=t to the following two hosts:
# http://benchmark-deception-sut-worst-case-java.benchmark-deception-sut-with.svc.cluster.local:8080/benchmark
# http://benchmark-deception-sut-worst-case-python.benchmark-deception-sut-with.svc.cluster.local:8080/benchmark

# re-deploy the "worst-case" scenario SUT with LD_PRELOAD
skaffold delete -p deploy-sut -f ./skaffold-benchmark.yaml -n benchmark-deception-sut-with

# manually disable honeywires by setting `enabled: false` in the `honeyyaml.yaml` file

# connect to http://localhost:8089 and start a benchmark run for /benchmark wires=f to the following two hosts:
# http://benchmark-deception-sut-worst-case-java.benchmark-deception-sut-with.svc.cluster.local:8080/benchmark
# http://benchmark-deception-sut-worst-case-python.benchmark-deception-sut-with.svc.cluster.local:8080/benchmark

# re-deploy the "worst-case" scenario SUT without LD_PRELOAD
skaffold run -p deploy-sut -f ./skaffold-benchmark.yaml -n benchmark-deception-sut-without

# connect to http://localhost:8089 and start a benchmark run for /benchmark w/o wires to the following two hosts:
# http://benchmark-deception-sut-worst-case-java.benchmark-deception-sut-without.svc.cluster.local:8080/benchmark
# http://benchmark-deception-sut-worst-case-python.benchmark-deception-sut-without.svc.cluster.local:8080/benchmark

# copy benchmarking results from the cluster
kubectl get pod -n deception --selector="app.kubernetes.io/name=tb-locust"
kubectl cp deception/${POD-NAME}:/opt/locust/cvs_exports /$(pwd)/bin/mount/benchmark_exports/

# delete deployment and SUT
skaffold delete -p operator-and-test-bench -f ./skaffold-benchmark.yaml
```

### 2️⃣ Local deployment with Docker

#### Benchmarking the "average-case" scenario with Docker

```sh
# start "average-case" with LD_PRELOAD
docker-compose -f benchmark/system-under-test/average-case/docker-compose_default_LD_PRELOAD.yaml -p avg-with-deception up -d

# start locust test bench
docker run -d --name benchmark-tb-locust --network="host" -v ./bin/mount/benchmark_exports:/opt/locust/cvs_exports benchmark-tb-locust

# manually change the `honeyyaml.yaml` file to the "average-case" scenario

# connect to http://localhost:8089 and start a benchmark run for /admin w/ wires=t to the following host:
# http://localhost:8080/admin

# connect to http://localhost:8089 and start a benchmark run for /tools.descartes.teastore.webui/ w/ wires=t to the following host:
# http://localhost:8080/tools.descartes.teastore.webui/

# manually disable honeywires by setting `enabled: false` in the `honeyyaml.yaml` file

# connect to http://localhost:8089 and start a benchmark run for /admin w/ wires=f to the following host:
# http://localhost:8080/admin

# connect to http://localhost:8089 and start a benchmark run for /tools.descartes.teastore.webui/ w/ wires=f to the following host:
# http://localhost:8080/tools.descartes.teastore.webui/

# delete SUT with LD_PRELOAD
docker-compose -f benchmark/system-under-test/average-case/docker-compose_default_LD_PRELOAD.yaml -p avg-with-deception down

# start "average-case" without LD_PRELOAD
docker-compose -f ./benchmark/system-under-test/average-case/docker-compose_default.yaml -p avg-without-deception up -d

# connect to http://localhost:8089 and start a benchmark run for /admin w/o wires to the following host:
# http://localhost:8080/admin

# connect to http://localhost:8089 and start a benchmark run for /tools.descartes.teastore.webui/ w/o wires to the following host:
# http://localhost:8080/tools.descartes.teastore.webui/

# delete SUT and test bench
docker-compose -f ./benchmark/system-under-test/average-case/docker-compose_default.yaml -p avg-without-deception down
docker rm -f benchmark-tb-locust

# copy the resulting CSV files from ./bin/mount/benchmark_exports to your host
```

#### Benchmarking the "worst-case" scenario with Docker

```sh
# start "worst-case" scenario and locust
docker-compose -f ./benchmark/system-under-test/worst-case/docker-compose.yaml up -d

# connect to http://localhost:8089 and start a benchmark run for /benchmark w/ wires=t to the following host:
# http://benchmark-sut-worst-case-java-without-ldpreload:8080/benchmark
# http://benchmark-sut-worst-case-java-with-ldpreload:8081/benchmark
# http://benchmark-sut-worst-case-python-without-ldpreload:8082/benchmark
# http://benchmark-sut-worst-case-python-with-ldpreload:8083/benchmark

# manually disable honeywires by setting `enabled: false` in the `honeyyaml.yaml` file

# connect to http://localhost:8089 and start a benchmark run for /benchmark w/ wires=t to the following host:
# http://benchmark-sut-worst-case-java-with-ldpreload:8081/benchmark
# http://benchmark-sut-worst-case-python-with-ldpreload:8083/benchmark

# copy the resulting CSV files from ./bin/mount/benchmark_exports to your host
```

### 3️⃣ Cloud deployment in AWS

Before continuing, ensure that you followed the prerequisites that we mentioned at the beginning of the document.
Then, start by creating the required ECR repositories:

```sh
# update your kubeconfig to point to your EKS cluster
aws eks update-kubeconfig --name ${CLUSTER_NAME} --region ${REGION}

# login to your ECR container registry
aws ecr get-login-password --region ${REGION} | docker login --username AWS --password-stdin ${AWS_ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com

# create repositories for the "worst-case" SUT images
aws ecr create-repository --repository-name benchmark-deception-webhook
aws ecr create-repository --repository-name benchmark-tb-locust
aws ecr create-repository --repository-name benchmark-sut-worst-case-java
aws ecr create-repository --repository-name benchmark-sut-worst-case-python
```

#### Benchmarking the "average-case" scenario in AWS

```sh
# setup system, i.e., create namespaces, operator, and test bench
skaffold run -p operator-and-test-bench -f ./skaffold-benchmark.yaml --default-repo ${AWS_ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com

# deploy benchmark-deception-sut-with (operator will inject LD_PRELOAD automatically in namespace benchmark-deception-sut-with)
kubectl create -n benchmark-deception-sut-with -f https://raw.githubusercontent.com/DescartesResearch/TeaStore/master/examples/kubernetes/teastore-clusterip.yaml

# forward test bench port
kubectl port-forward -n deception service/benchmark-tb-locust 8089:8089

# manually change the `honeyyaml.yaml` file to the "average-case" scenario

# connect to http://localhost:8089 and start a benchmark run for /admin w/ wires=t to the following two hosts:
# http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/admin
# http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/tools.descartes.teastore.webui/

# manually disable honeywires by setting `enabled: false` in the `honeyyaml.yaml` file

# connect to http://localhost:8089 and start a benchmark run for /admin w/ wires=t to the following two hosts:
# http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/admin
# http://teastore-webui.benchmark-deception-sut-with.svc.cluster.local:8080/tools.descartes.teastore.webui/

# delete SUT
kubectl delete deployments,pods,services -n benchmark-deception-sut-with -l app=teastore

# deploy SUIT without LD_PRELOAD
kubectl create -n benchmark-deception-sut-without -f https://raw.githubusercontent.com/DescartesResearch/TeaStore/master/examples/kubernetes/teastore-clusterip.yaml

# forward test bench port
kubectl port-forward -n deception service/benchmark-tb-locust 8089:8089

# connect to http://localhost:8089 and start a benchmark run for /admin w/ wires=f to the following two hosts:
# http://teastore-webui.benchmark-deception-sut-without.svc.cluster.local:8080/admin

# delete SUT and forward test bench port
kubectl delete pod -n benchmark-tb-locust-7b5b79478b-s7hvb
kubectl port-forward -n deception service/benchmark-tb-locust 8089:8089

# connect to http://localhost:8089 and start a benchmark run for /admin w/o wires to the following two hosts:
# http://teastore-webui.benchmark-deception-sut-without.svc.cluster.local:8080/tools.descartes.teastore.webui/

# delete SUT
kubectl delete deployments,pods,services -n benchmark-deception-sut-without -l app=teastore
```

#### Benchmarking the "worst-case" scenario in AWS

```sh
# setup system, i.e., create namespaces, operator, and test bench
skaffold run -p operator-and-test-bench -f ./skaffold-benchmark.yaml --default-repo ${AWS_ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com

# manually change the `honeyyaml.yaml` file to the "average-case" scenario

# create SUT with LD_PRELOAD
skaffold run -p deploy-sut -f ./skaffold-benchmark.yaml -n benchmark-deception-sut-with --default-repo ${AWS_ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com

# forward test bench port
kubectl port-forward -n deception service/benchmark-tb-locust 8089:8089

# connect to http://localhost:8089 and start a benchmark run for /admin w/ wires=t to the following two hosts:
# http://benchmark-deception-sut-worst-case-java.benchmark-deception-sut-with.svc.cluster.local:8080/benchmark
# http://benchmark-deception-sut-worst-case-python.benchmark-deception-sut-with.svc.cluster.local:8080/benchmark

# manually disable honeywires by setting `enabled: false` in the `honeyyaml.yaml` file

# connect to http://localhost:8089 and start a benchmark run for /admin w/ wires=f to the following two hosts:
# http://benchmark-deception-sut-worst-case-java.benchmark-deception-sut-with.svc.cluster.local:8080/benchmark
# http://benchmark-deception-sut-worst-case-python.benchmark-deception-sut-with.svc.cluster.local:8080/benchmark

# create SUT without LD_PRELOAD
skaffold run -p deploy-sut -f ./skaffold-benchmark.yaml -n benchmark-deception-sut-without --default-repo ${AWS_ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com

# connect to http://localhost:8089 and start a benchmark run for /admin w/o wires to the following two hosts:
# http://benchmark-deception-sut-worst-case-java.benchmark-deception-sut-without.svc.cluster.local:8080/benchmark
# http://benchmark-deception-sut-worst-case-python.benchmark-deception-sut-without.svc.cluster.local:8080/benchmark

# copy results from cluster
kubectl get pod -n deception --selector="app.kubernetes.io/name=tb-locust"
kubectl cp deception/${POD-NAME}:/opt/locust/cvs_exports /$(pwd)/bin/mount/benchmark_exports/

# delete deployment
skaffold delete -p operator-and-test-bench -f ./skaffold-benchmark.yaml --default-repo ${AWS_ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com
```
