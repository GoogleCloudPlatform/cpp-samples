# Cloud Run for C++

This repository contains an example showing how to deploy C++ applications in
Google Cloud Run.

## Prerequisites

Install the command-line tools, for example:

```bash
gcloud components install beta
gcloud components install kubectl
gcloud components install docker-credential-gcr
gcloud auth configure-docker
```

## Bootstrap the Indexer

Pick the project to run the service and the spanner instance.

```bash
export GOOGLE_CLOUD_PROJECT=...
```

Run the program to bootstrap the indexer, this will create the Cloud Spanner
instance and databases to store the data, the Cloud Run deployments to run the
indexer, two service accounts to run the Cloud Run deployments, and a Cloud
Pub/Sub topic to receive GCS metadata updates. It will also setup the IAM
permissions so Cloud Pub/Sub can push these updates to Cloud Run.

```bash
./bootstrap-gcs-indexer.sh
```

## Index an existing Bucket

Then connect the program to one of the buckets in this project. This will
configure the bucket to publish any metadata updates to the topic created above,
and start a program to index the existing metadata in the bucket.

```bash
./refresh-index.sh my-bucket-name
```

For larger buckets, if they are well partitioned using prefixes, consider using:

```bash
./refresh-index.sh my-bucket-name/prefix1 my-bucket-name/prefix2 ... my-bucket-name/prefixN
```
