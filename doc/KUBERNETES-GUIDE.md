# Guide for deploying the deception system in Kubernetes

This document explains how to build and deploy a deception operator in Kubernetes, which takes care of injecting the LD_PRELOAD module into every app in your cluster.
Our deployment is only meant for demo environments, partly because we hard-coded the TLS certificates and keys in the [deception-secret.yaml](../k8s-manifests/base/webhook-environment/deception-secret.yaml)
and [mutating-webhook-configuration.yaml](../k8s-manifests/base/webhook-environment/mutating-webhook-configuration.yaml) files.
For production environments we suggest re-working this to the [CA Injector from cert-manager](https://cert-manager.io/docs/concepts/ca-injector/) instead.

## 🗒️ Prerequisites

For a playground deployment, you will definitely need the following tools:

* [Docker](https://www.docker.com/products/docker-desktop)
* [kubectl](https://kubernetes.io/docs/tasks/tools/)
* [Skaffold](https://skaffold.dev/docs/install/)
* [Kustomize](https://kubectl.docs.kubernetes.io/installation/kustomize/)

Additionally, for a local deployment you also need [Kind](https://kind.sigs.k8s.io/).
For a deployment in AWS, you will also need the [AWS CLI](https://aws.amazon.com/cli/).

## ⛵ Local cluster deployment

### 🅰 Initial cluster setup

1. Launch a local Kubernetes cluster with Kind. The `cluster-config.yaml` ensures that the `deception.so` and `honeyaml.yaml` are mounted directly to the nodes.

   ```sh
   kind create cluster --name deception --config ./k8s-manifests/localdev/kind/cluster-config.yaml
   ```

2. Run `kubectl get nodes` to verify the connection to the respective control plane

### 🅱 Application deployment and deception operator deployment

1. Select the namespace where you want to inject the shared library. There two options:

   1. Create a new namespace `deception-dev`

      ```sh
      kubectl create namespace deception-dev
      ```

   2. Use an existing namespace. Have a look at the section [Deployment to different namespaces](#deployment-to-different-namespaces) below.

2. Deploy the operator and its resources.
   This will create a namespace, the webhook, secrets, a persistent volume (and its claims), and a mutating webhook configuration.

   ```sh
   skaffold run -p kind-webhook
   ```

3. Deploy your actual application. To quickly demo the prototype, you can use our demo application.
   Possibly adapt the target namespace `-n` if you need to. Possibyl add `--port-forward` to directly access the service.

   ```sh
   skaffold run -p deploy-all -f ./skaffold-demo-deployment.yaml -n deception-dev
   ```

## ☁ AWS cloud deployment

### Additional AWS prerequisites

We assume you are running an [Elastic Kubernetes Service (EKS)](https://aws.amazon.com/eks/) cluster in AWS.
The following additional components are required:

* [Elastic Container Registry (ECR)](https://aws.amazon.com/ecr/) to store the `deception-python-webhook-dev` image
  * Optionally, if you use [skaffold-demo-deployment.yaml](../skaffold-demo-deployment.yaml), also add those images
* [Elastic File System (EFS)](https://aws.amazon.com/efs/) storage with static provisioning, to store the shared library
  * Follow the [official user guide for the EFS CSI driver](https://docs.aws.amazon.com/eks/latest/userguide/efs-csi.html) to setup EFS
  * Ensure that you have configured an IAM policy that allows you to access EFS from your EKS cluster
  * After creation, include the content of the [mount](../bin/mount) folder inside the storage, easiest by creating a container, mounting the volume, and copying the files manually

Also ensure sure to replace the `<fs-id>` placeholder in [persistent-volume.yaml](../k8s-manifests/cloud-service-provider/aws/webhook-environment/persistent-volume.yaml) with your EFS ID.

### EKS Deployment

1. Let `aws` update your kubeconfig to be connected to EKS.
   This command will also return you the account ID needed in the next step.
   It is encoded in the ARN `arn:aws:eks:${REGION}:${AWS_ACCOUNT_ID}:cluster/${CLUSTER_NAME}`.

   ```sh
   aws eks update-kubeconfig --name ${CLUSTER_NAME} --region ${REGION}
   ```

2. Let `aws` log you into your ECR repository so that Docker pushes images to that.

   ```sh
   aws ecr get-login-password --region ${REGION} | docker login --username AWS --password-stdin ${AWS_ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com
   ```

3. Push the webhook image to ECR and deploy the webhook to the EKS cluster.
   Therefore, the namespace `deception` will be created for the webhook.
   The `deception-dev` namespace has to be generated manually and the webhook will mount the deception system to newly created pods inside the namespace afterwards.
   If you want to deploy the webhook for a different namespace, take a look at section [Deployment to different namespaces] (#deployment-to-different-namespaces) before proceeding.
   **Reminder:** Make sure to replace the `<fs-id>` placeholder inside [persistent-volume.yaml](../k8s-manifests/cloud-service-provider/aws/webhook-environment/persistent-volume.yaml)
   to the EFS ID that holds the content of the [mount/](../bin/mount) folder.

   ```sh
   kubectl create namespace deception-dev
   skaffold run -p aws-webhook --default-repo ${AWS_ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com
   ```

4. 🎉 Finished. Let's create new pods and see the prototype in action. For a simple demo, use our test deployment.

     ```sh
     kubectl create namespace deception-dev
     skaffold run -p deploy-all -f ./skaffold-demo.yaml -n deception-dev --default-repo ${AWS_ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com
     ```

## Deployment to different namespaces

To deploy the webhook to a namespace other than `deception-dev` two changes need to be applied to the manifest files.
Make these changes before deploying the webhook, i.e., before running the skaffold profile for the webhook.

* In [mutating-webhook-configuration.yaml](../k8s-manifests/base/webhook-environment/mutating-webhook-configuration.yaml)
  change the value for `kubernetes.io/metadata.name` inside `webhooks.namespaceSelector.matchLabels` to your namespace name
* In [persistent-volume-claim.yaml](../k8s-manifests/localdev/webhook/persistent-volume-claim.yaml):
  change the value for `namespace` inside `metadata` to your namespace name

## 🔥 Cleanup

Use `skaffold delete` to delete deployments with the chosen profile used to start up the system.
Make sure to first delete all pods that uses the persistent volume. With the test deployment, this can be achieved with:

```sh
# possibly adapt the target namespace argument
skaffold delete -p deploy-all -f skaffold-demo-deployment.yaml -n deception-dev

# optionally delete the 'deception-dev' namespace as well
skaffold run -p deception-dev-namespace
```

Delete the mutating webhook and manually delete all pods that link to its volume.

```sh
kubectl delete mutatingWebhookConfiguration deception-python-webhook
```

After that, release all resources inside the namespace `deception`, including the namespace itself.

```sh
skaffold delete -p kind-webhook
```

### 🔥⛵ Cleanup loocal cluster deployment

If the entire local cluster shall be deleted, use the following command.

```sh
kind delete cluster --name deception
```
