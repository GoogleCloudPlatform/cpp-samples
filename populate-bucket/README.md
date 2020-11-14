# Large Scale Parallel C++ Jobs with Cloud Pub/Sub and GKE

## Motivation

From time to time the Cloud C++ team needs to generate buckets with millions or hundreds of millions of objects to test
our libraries or applications. We often generate synthetic data for these tests. Like many C++ all developers, we are
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

```bash
gcloud components install docker-credential-gcr
gcloud auth configure-docker
```

## Bootstrap the Example

### Pick a region to run your project

```
GOOGLE_CLOUD_REGION="${REGION:-us-central1}"
readonly GOOGLE_CLOUD_REGION
```

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

### Create the GKE cluster

```sh
gcloud container clusters create cpp-samples-parallel-jobs \
      "--project=${GOOGLE_CLOUD_PROJECT}" \
      "--region=${GOOGLE_CLOUD_REGION}" \
      "--preemptible" \
      "--min-nodes=0" \
      "--max-nodes=60" \
      "--enable-autoscaling" \
      "--workload-pool=${GOOGLE_CLOUD_PROJECT}.svc.id.goog"
gcloud container clusters get-credentials cpp-samples-parallel-jobs
```

### Create a service account for the workers

```sh
readonly SA_ID="cpp-samples-sa"
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
kubectl create namespace cpp-samples-sa
```

### Create the service account id in the k8s cluster

```sh
kubectl create serviceaccount --namespace cpp-samples-sa worker
```

### Grant the k8s permissions to impersonate the SA in GCP

```sh
gcloud iam service-accounts add-iam-policy-binding \
  --role roles/iam.workloadIdentityUser \
  --member "serviceAccount:${GOOGLE_CLOUD_PROJECT}.svc.id.goog[cpp-samples-sa/worker]" \
  ${SA_NAME}
```

### Annotate the Service Account

```sh
kubectl annotate serviceaccount \
  --namespace cpp-samples-sa worker \
  iam.gke.io/gcp-service-account=${SA_NAME}
```

### Build the Docker images containing the program

```
gcloud builds submit \
    "--project=${GOOGLE_CLOUD_PROJECT}" \
    "--substitutions=SHORT_SHA=$(git rev-parse --short HEAD)" \
    "--config=cloudbuild.yaml"
```

### Create the Cloud Pub/Sub topic and subscription

```sh
gcloud pubsub topics create "--project=${GOOGLE_CLOUD_PROJECT}" populate-bucket 
gcloud pubsub subscriptions create "--project=${GOOGLE_CLOUD_PROJECT}" --topic populate-bucket populate-bucket 
```

### Run the deployment with workers

```sh
./deployment.py --project=${GOOGLE_CLOUD_PROJECT} | kubectl apply -f -
kubectl autoscale deployment cpp-samples-populate-bucket --max 100 --min 1 --cpu-percent 50
```

### Pick a bucket, and create it if needed

```bash
BUCKET_NAME=...
gsutil -p ${GOOGLE_CLOUD_PROJECT} mb gs://${BUCKET_NAME}
```

### Run the program locally to schedule the work

```sh
./populate_bucket schedule \
    --project=${GOOGLE_CLOUD_PROJECT} \
    --topic=populate-bucket \
    --bucket=${BUCKET_NAME}
```
