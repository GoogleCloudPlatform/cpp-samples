# Large Scale Parallel C++ Jobs with Cloud Pub/Sub and GKE

## Motivation

From time to time the Cloud C++ team needs to generate buckets with millions or hundreds of millions of objects to test
our libraries or applications. We often generate synthetic data for these tests. Like many C++ developers, we are
impatient, and we want our synthetic data to be generated as quickly as possible so we can start with the rest of our
work. This directory contains an example using C++, CPS (Google Cloud Pub/Sub), and GKE (Google Kubernetes Engine) to
populate a GCS (Google Cloud Storage) bucket with millions or hundreds of millions of objects.

## Overview

The basic idea is to break the work into a small number of work items, such as, "create 1,000 objects with this prefix".
We will use a command-line tool to publish these work items to a CPS topic, where they can be reliably delivered to any
number of workers that will execute the work items. We will use GKE to run the workers, as GKE can automatically scale
the cluster based on demand, and as it will restart the workers if needed after a failure. 

Because CPS offers "at least once" semantics, and because the workers may be restarted by GKE, it is important to make
these work items idempotent, that is, executing the work item times produces the same objects in GCS as executing the
work item once.

## Prerequisites

This example assumes that you have an existing GCP (Google Cloud Platform) project. The project must have billing
enabled, as some of the services used in this example require it. Throughput the example we will use
`GOOGLE_CLOUD_PROJECT` as an environment variable containing the name of the project.

### Make sure the necessary services are enabled

```sh
gcloud services enable cloudbuild.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud services enable containerregistry.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud services enable container.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud services enable pubsub.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"
```

### Install the tools to manage GKE clusters

```bash
sudo apt install kubectl
gcloud components install docker-credential-gcr
gcloud auth configure-docker
```

## Bootstrap the Example

### Pick a region to run your project

```
GOOGLE_CLOUD_REGION="us-central1"  # Example, pick other region if preferred
readonly GOOGLE_CLOUD_REGION
```

### Create the GKE cluster

We use preemptible nodes (the `--preemptible` flag) because they have lower cost, and the application can safely
restart. We also configure the cluster to grow as needed, the maximum number of nodes (in this case `64`), should be
set based on your available quota or budget. Note that we enable [workload identity][workload-identity], the recommended
way for GKE-based applications to consume services in Google Cloud.

[workload-identity]: https://cloud.google.com/kubernetes-engine/docs/how-to/workload-identity

```sh
gcloud container clusters create cpp-samples \
      "--project=${GOOGLE_CLOUD_PROJECT}" \
      "--region=${GOOGLE_CLOUD_REGION}" \
      "--preemptible" \
      "--min-nodes=0" \
      "--max-nodes=64" \
      "--enable-autoscaling" \
      "--workload-pool=${GOOGLE_CLOUD_PROJECT}.svc.id.goog"
```

Once created, we configure the `kubectl` credentials to use this cluster:

```sh
gcloud container clusters --region=${GOOGLE_CLOUD_REGION} --project=${GOOGLE_CLOUD_PROJECT} get-credentials cpp-samples
```

### Create a service account for the GKE workload

The GKE workload will need a GCP service account to access GCP resources, pick a name and create the account:

```sh
readonly SA_ID="populate-bucket-worker-sa"
readonly SA_NAME="${SA_ID}@${GOOGLE_CLOUD_PROJECT}.iam.gserviceaccount.com"

gcloud iam service-accounts create "${SA_ID}" \
    "--project=${GOOGLE_CLOUD_PROJECT}" \
    --description="C++ Samples Service Account"
```

### Grant this SA permissions to read from Cloud Pub/Sub

```sh
gcloud projects add-iam-policy-binding "${GOOGLE_CLOUD_PROJECT}" \
    "--member=serviceAccount:${SA_NAME}" \
    "--role=roles/pubsub.subscriber"
```

### Grant this SA permissions to write to any GCS Bucket

```sh
gcloud projects add-iam-policy-binding "${GOOGLE_CLOUD_PROJECT}" \
    "--member=serviceAccount:${SA_NAME}" \
    "--role=roles/storage.objectAdmin"
```


### Create a k8s namespace for the example resources

```sh
NAMESPACE=populate-bucket
kubectl create namespace ${NAMESPACE}
```

### Create a GKE service account

```sh
kubectl create serviceaccount --namespace ${NAMESPACE} worker
```

### Grant the GKE permissions to impersonate the GCP Service Account in GCP

```sh
gcloud iam service-accounts add-iam-policy-binding \
  "--project=${GOOGLE_CLOUD_PROJECT}" \
  "--role=roles/iam.workloadIdentityUser" \
  "--member=serviceAccount:${GOOGLE_CLOUD_PROJECT}.svc.id.goog[${NAMESPACE}/worker]" \
  "${SA_NAME}"
```

### Map the GKE service account to the GCP service account

```sh
kubectl annotate serviceaccount \
  --namespace ${NAMESPACE} worker \
  iam.gke.io/gcp-service-account=${SA_NAME}
```

### Build the Docker image containing the program

```
IMAGE_VERSION=$(git rev-parse --short HEAD)
gcloud builds submit \
    "--project=${GOOGLE_CLOUD_PROJECT}" \
    "--substitutions=SHORT_SHA=${IMAGE_VERSION}" \
    "--config=cloudbuild.yaml"
```

### Create the Cloud Pub/Sub topic and subscription

```sh
gcloud pubsub topics create "--project=${GOOGLE_CLOUD_PROJECT}" populate-bucket 
gcloud pubsub subscriptions create "--project=${GOOGLE_CLOUD_PROJECT}" --topic populate-bucket populate-bucket 
```

### Run the deployment with workers

```sh
./deployment.py --project=${GOOGLE_CLOUD_PROJECT} --image-version=${IMAGE_VERSION} | kubectl apply -f -
kubectl --namespace ${NAMESPACE} autoscale deployment worker  --max 200 --min 1 --cpu-percent 50
```

### Pick a bucket, and create it if needed

```bash
BUCKET_NAME=${GOOGLE_CLOUD_PROJECT}-bucket-1000000
gsutil mb -p ${GOOGLE_CLOUD_PROJECT} gs://${BUCKET_NAME}
```

### Run the program locally to schedule the work

```sh
./populate_bucket schedule \
    --project=${GOOGLE_CLOUD_PROJECT} \
    --topic=populate-bucket \
    --bucket=${BUCKET_NAME} \
    --object-count=1000000 \
    --task-size=100
```
