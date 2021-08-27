# Getting Started with GCP and C++

> :warning: This is a work-in-progress, we are writing down the guide in small steps, please do not rely on it until completed.

## Motivation

A typical use of C++ in Google Cloud is to perform parallel computations or analysis and store the results in some kind of database.
In this guide with will build such an application, and deploy it to [Cloud Run], a managed platform to deploy containerized applications.

[Cloud Run]: https://cloud.google.com/run
[Cloud Storage]: https://cloud.google.com/storage
[GCS]: https://cloud.google.com/storage
[Cloud Spanner]: https://cloud.google.com/spanner
[Container Registry]: https://cloud.google.com/container-registry
[cloud-run-quickstarts]: https://cloud.google.com/run/docs/quickstarts
[gcp-quickstarts]: https://cloud.google.com/resource-manager/docs/creating-managing-projects
[buildpacks]: https://buildpacks.io
[docker]: https://docker.com/
[docker-install]: https://store.docker.com/search?type=edition&offering=community
[sudoless docker]: https://docs.docker.com/engine/install/linux-postinstall/
[pack-install]: https://buildpacks.io/docs/install-pack/

## Overview

For this guide, we will "index" the object metadata in a [Cloud Storage] bucket.
Google Cloud Storage (GCS) buckets can contain thousands, millions, and even billions of objects.
GCS can quickly find an object given its name, or list objects with names in a given range, but some applications need
more advance lookups, such as finding all the objects within a certain size, or with a given object type.

In this guide, we will create and deploy an application to scan all the objects in a bucket, and store the full metadata information of each object in a [Cloud Spanner] instance.
Once the information is in a Cloud Spanner table, where one can use normal SQL statements to search for objects.

## Prerequisites

This example assumes that you have an existing GCP (Google Cloud Platform) project.
The project must have billing enabled, as some of the services used in this example require it. If needed, consult:
* the [GCP quickstarts][gcp-quickstarts] to setup a GCP project
* the [cloud run quickstarts][cloud-run-quickstarts] to setup Cloud Run in your
  project

Verify the [docker tool][docker] is functional on your workstation:

```shell
docker run hello-world
# Output: Hello from Docker! and then some more informational messages.
```

If needed, use the [online instructions][docker-install] to download and install
this tool. This guide assumes that you have configured [sudoless docker]. If
you do not want to enable sudoless docker, replace all `docker` commands below with `sudo docker`.

Verify the [pack tool][pack-install] is functional on your workstation. These
instructions were tested with v0.20.0, although they should work with newer
versions. Some commands may not work with older versions.

```shell
pack version
# Output: a version number, e.g., 0.20.0+git-66a4f32.build-2668
```

Throughout the example we will use `GOOGLE_CLOUD_PROJECT` as an environment variable containing the name of the project.

> :warning: this guide uses Cloud Spanner, this service is billed by the hour **even if you stop using it**.
> The charges can reaches the **hundreds** or **thousands** of dollars per month if you configure a large Cloud Spanner instance.
> Please remember to delete any Cloud Spanner resources once you no longer need them.

### Configure the Google Cloud CLI to use your project

```sh
gcloud config set project ${GOOGLE_CLOUD_PROJECT}
# Output: Updated property [core/project].
```

### Make sure the necessary services are enabled

```sh
gcloud services enable cloudbuild.googleapis.com
gcloud services enable containerregistry.googleapis.com
gcloud services enable container.googleapis.com
gcloud services enable pubsub.googleapis.com
gcloud services enable spanner.googleapis.com
# Output: nothing if the services are already enabled.
# for services that are not enabled something like this
#  Operation "operations/...." finished successfully.
```

### Create a Cloud Spanner Instance to host your data

```sh
gcloud beta spanner instances create getting-started-cpp \
    --config=regional-us-central1 \
    --processing-units=100 \
    --description="'Getting Started with C++'"
# Output: Creating instance...done.
```

### Create the Cloud Spanner Database and Table for your data

```sh
gcloud spanner databases create gcs-index \
    --ddl-file=gcs_objects.sql \
    --instance=getting-started-cpp
# Output: Creating database...done.
```

### Create a Cloud Pub/Sub Topic for Indexing Requests

```sh
gcloud pubsub topics create gcs-indexing-requests
# Output: Created topic [projects/..../topics/gcs-indexing-requests].
```

### Get the code for these examples in your workstation

```sh
git clone https://github.com/GoogleCloudPlatform/cpp-samples
# Output: Cloning into 'cpp-samples'...
#   additional informational messages
```

### Change your working directory

```sh
cd cpp-samples/getting-started
# Output: none
```

### Build Docker images for the sample programs

<!-- TODO(#138) - add caching in GCR to this command -->

```sh
pack build \
    --builder gcr.io/buildpacks/builder:latest \
    --env GOOGLE_FUNCTION_SIGNATURE_TYPE=http \
    --env GOOGLE_FUNCTION_TARGET=IndexGcsPrefix \
    --path . \
    "gcr.io/${GOOGLE_CLOUD_PROJECT}/getting-started-cpp/index-gcs-prefix"
# Output: a large number of informational messages
#   ... followed by informational messages from the package manager ...
#   ... followed by informational messages from the build system ...
# Successfully built image gcr.io/${GOOGLE_CLOUD_PROJECT}/getting-started-cpp/index-gcs-prefix
```

### Push the Docker images to Google Container Registry

```sh
docker push "gcr.io/${GOOGLE_CLOUD_PROJECT}/getting-started-cpp/index-gcs-prefix:latest"
# Output: The push refers to repository [gcr.io/${GOOGLE_CLOUD_PROJECT}/getting-started-cpp/index-gcs-prefix]
#   ... progress information ...
# latest: digest: sha256:<long hex sha> size: <number>
```

### Deploy the Programs to Cloud Run

```sh
gcloud run deploy index-gcs-prefix \
    --image="gcr.io/${GOOGLE_CLOUD_PROJECT}/getting-started-cpp/index-gcs-prefix:latest" \
    --set-env-vars="SPANNER_INSTANCE=getting-started-cpp,SPANNER_DATABASE=gcs-index,TOPIC_ID=gcs-indexing-requests,GOOGLE_CLOUD_PROJECT=${GOOGLE_CLOUD_PROJECT}" \
    --region="us-central1" \
    --platform="managed" \
    --no-allow-unauthenticated
# Output: Deploying container to Cloud Run service [index-gcs-prefix] in project [....] region [us-central1]
#     Service [gcs-indexing-worker] revision [index-gcs-prefix-00001-yeg] has been deployed and is serving 100 percent of traffic.
#     Service URL: https://index-gcs-prefix-...run.app
```

### Setup the Cloud Pub/Sub subscription

#### Create a Service Account to invoke Cloud Run

Following security best pratices, we recommend creating a service account for
this demo, with only permission to invoke Cloud Run functions, to minimize the
permissions granted to each service.

```sh
gcloud iam service-accounts create getting-started-cpp-push-sa \
   --display-name "Getting Started with C++: Push Cloud Pub/Sub messages to Cloud Run"
# Output: Created service account [getting-started-cpp-push-sa].
```

```sh
gcloud run services add-iam-policy-binding index-gcs-prefix \
   --region="us-central1" \
   --member="serviceAccount:getting-started-cpp-push-sa@${GOOGLE_CLOUD_PROJECT}.iam.gserviceaccount.com" \
   --role="roles/run.invoker"
# Output: Updated IAM policy for service [index-gcs-prefix].
#   ... and the details of the complete IAM policy ...
```

#### Capture the project number

```sh
PROJECT_NUMBER=$(gcloud projects list \
    --filter="project_id=${GOOGLE_CLOUD_PROJECT}" \
    --format="value(project_number)" \
    --limit=1)
# Output: none
```

#### Grant Cloud Pub/Sub permissions to use service accounts in your project

```sh
gcloud projects add-iam-policy-binding "${GOOGLE_CLOUD_PROJECT}" \
    --member=serviceAccount:service-${PROJECT_NUMBER}@gcp-sa-pubsub.iam.gserviceaccount.com \
    --role=roles/iam.serviceAccountTokenCreator
# Output: Updated IAM policy for project [${GOOGLE_CLOUD_PROJECT}].
#    bindings:
#    ... full list of bindings for your project's IAM policy ...
```

### Capture the Service URL

```sh
URL="$(gcloud run services describe index-gcs-prefix \
    --region="us-central1" --format="value(status.url)")"
# Output: none
```

### Create the Cloud Pub/Sub push subscription

```sh
gcloud pubsub subscriptions create indexing-requests-cloud-run-push \
    --topic="gcs-indexing-requests" \
    --push-endpoint="${URL}" \
    --push-auth-service-account="getting-started-cpp-push-sa@${GOOGLE_CLOUD_PROJECT}.iam.gserviceaccount.com" \
    --ack-deadline=600
# Output: Created subscription [projects/${GOOGLE_CLOUD_PROJECT}/subscriptions/indexing-requests-cloud-run-push].
```

### Use `gcloud` to send an indexing request

This will request indexing some public data. The prefix contains less than 100 objects:

```sh
gcloud pubsub topics publish gcs-indexing-requests \
    --attribute=bucket=gcp-public-data-landsat,prefix=LC08/01/006/001
# Output: messageIds:
#     - '....'
```

### Querying the data

```sh
gcloud spanner databases execute-sql gcs-index --instance=getting-started-cpp \
    --sql="select * from gcs_objects where name like '%.txt' order by size desc limit 10"
# Output: metadata for the 10 largest objects with names finishing in `.txt`
```

## Cleanup

> :warning: do not forget to cleanup your billable resources after going through this "Getting Started" guide.

### Remove the Cloud Spanner Instance

```sh
gcloud spanner databases delete gcs-index --instance=getting-started-cpp --quiet
# Output: none
gcloud spanner instances delete getting-started-cpp --quiet
# Output: none
```

### Remove the Cloud Run Deployments

```sh
gcloud run services delete index-gcs-prefix \
    --region="us-central1" \
    --platform="managed" \
    --quiet
# Output:
#   Deleting [index-gcs-prefix]...done.
#   Deleted [index-gcs-prefix].
```

### Remove the Cloud Pub/Sub Subscription

```sh
gcloud pubsub subscriptions delete indexing-requests-cloud-run-push --quiet
# Output: Deleted subscription [projects/${GOOGLE_CLOUD_PROJECT}/subscriptions/indexing-requests-cloud-run-push].
```

### Remove the Cloud Pub/Sub Topic

```sh
gcloud pubsub topics delete gcs-indexing-work-items --quiet
# Output: Deleted topic [projects/${GOOGLE_CLOUD_PROJECT}/topics/gcs-indexing-work-items].
```

### Remove the Container image

```sh
gcloud container images delete gcr.io/${GOOGLE_CLOUD_PROJECT}/getting-started-cpp/index-gcs-prefix:latest --quiet
# Output: Deleted [gcr.io/${GOOGLE_CLOUD_PROJECT}/getting-started-cpp/index-gcs-prefix:latest]
# Output: Deleted [gcr.io/${GOOGLE_CLOUD_PROJECT}/getting-started-cpp/index-gcs-prefix@sha256:....]
```
