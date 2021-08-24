# Getting Started with GCP and C++

> :warning: This is a work-in-progress, we are writing down the guide in small steps, please do not rely on it until completed.

## Motivation

A typical use of C++ in Google Cloud is to perform parallel computations or analysis and store the results in some kind of database.
In this guide with will build such an application, and deploy it to [Cloud Run], a managed platform to deploy containerized applications.

[Cloud Run]: https://cloud.google.com/run
[Cloud Storage]: https://cloud.google.com/storage
[GCS]: https://cloud.google.com/storage
[Cloud Spanner]: https://cloud.google.com/spanner

## Overview

For this guide, we will "index" the object metadata in a [Cloud Storage] bucket.
Google Cloud Storage (GCS) buckets can contain thousands, millions, and even billions of objects.
GCS can quickly find an object given its name, or list objects with names in a given range, but some applications need
more advance lookups, such as finding all the objects within a certain size, or with a given object type.

Our application will scan potentially large buckets, and store the full metadata information of each object in a [Cloud Spanner] instance, where one can use normal SQL statements to search for objects.

## Prerequisites

This example assumes that you have an existing GCP (Google Cloud Platform) project. The project must have billing enabled, as some of the services used in this example require it. Throughout the example we will use `GOOGLE_CLOUD_PROJECT` as an environment variable containing the name of the project.

> :warning: this guide uses Cloud Spanner, this service is billed by the hour **even if you stop using it**.
> The charges can reaches the **hundreds** or **thousands** of dollars per month if you configure a large Cloud Spanner instance.
> Please remember to delete any Cloud Spanner resources once you no longer need them.

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

### Build the Docker image containing the program


### Create the Cloud Pub/Sub topic and subscription

```sh
gcloud pubsub topics create "--project=${GOOGLE_CLOUD_PROJECT}" gcs-index-requests
```

### Use `gcloud` to send an indexing request

```sh
```
