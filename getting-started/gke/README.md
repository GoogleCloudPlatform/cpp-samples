# Getting Started with GKE and C++

> :warning: this guide is work-in-progress. It is know to be incomplete,
> and none of the instructions have been validated or tested in any way.

This guide builds upon the general [Getting Started with C++](../README.md) guide.
It deploys the GCS indexing application to GKE instead of Cloud Run, taking advantage of the
long-running servers in GKE to improve throughput.

## Motivation

<!--
 TODO(coryan) - something about how batching multiple mutations is more efficient, but requires long-running servers
-->

[Cloud Build]: https://cloud.google.com/build
[Cloud Run]: https://cloud.google.com/run
[Cloud Storage]: https://cloud.google.com/storage
[Cloud Cloud SDK]: https://cloud.google.com/sdk
[Cloud Shell]: https://cloud.google.com/shell
[GCS]: https://cloud.google.com/storage
[Cloud Spanner]: https://cloud.google.com/spanner
[Container Registry]: https://cloud.google.com/container-registry
[Pricing Calculator]: https://cloud.google.com/products/calculator
[cloud-run-quickstarts]: https://cloud.google.com/run/docs/quickstarts
[gcp-quickstarts]: https://cloud.google.com/resource-manager/docs/creating-managing-projects
[buildpacks]: https://buildpacks.io
[docker]: https://docker.com/
[docker-install]: https://store.docker.com/search?type=edition&offering=community
[sudoless docker]: https://docs.docker.com/engine/install/linux-postinstall/
[pack-install]: https://buildpacks.io/docs/install-pack/

## Overview


## Prerequisites

This example assumes that you have an existing GCP (Google Cloud Platform) project.
The project must have billing enabled, as some of the services used in this example require it. If needed, consult:
* the [GCP quickstarts][gcp-quickstarts] to setup a GCP project
* the [cloud run quickstarts][cloud-run-quickstarts] to setup Cloud Run in your
  project

Use your workstation, a GCE instance, or the [Cloud Shell] to get a command-line prompt. If needed, login to GCP using:

```sh
gcloud auth login
```

Verify the [docker tool][docker] is functional on your workstation:

```sh
docker run hello-world
# Output: Hello from Docker! and then some more informational messages.
```

If needed, use the [online instructions][docker-install] to download and install
this tool. This guide assumes that you have configured [sudoless docker]. If
you do not want to enable sudoless docker, replace all `docker` commands below with `sudo docker`.

Throughout the example we will use `GOOGLE_CLOUD_PROJECT` as an environment variable containing the name of the project.

```sh
export GOOGLE_CLOUD_PROJECT=[PROJECT ID]
```

> :warning: this guide uses Cloud Spanner, this service is billed by the hour **even if you stop using it**.
> The charges can reaches the **hundreds** or **thousands** of dollars per month if you configure a large Cloud Spanner instance. Consult the [Pricing Calculator] for details.
> Please remember to delete any Cloud Spanner resources once you no longer need them.

### Configure the Google Cloud CLI to use your project

We will issue a number of commands using the [Google Cloud SDK], a command-line tool to interact with Google Cloud services.  Adding the `--project=$GOOGLE_CLOUD_PROJECT` to each invocation of this tool quickly becomes tedious, so we start by configuring the default project:

```sh
gcloud config set project $GOOGLE_CLOUD_PROJECT
# Output: Updated property [core/project].
```

### Make sure the necessary services are enabled

Some services are not enabled by default when you create a Google Cloud Project, so we start by enabling all the services we will need.

```sh
gcloud services enable run.googleapis.com
gcloud services enable cloudbuild.googleapis.com
gcloud services enable containerregistry.googleapis.com
gcloud services enable container.googleapis.com
gcloud services enable pubsub.googleapis.com
gcloud services enable spanner.googleapis.com
# Output: nothing if the services are already enabled.
# for services that are not enabled something like this
#  Operation "operations/...." finished successfully.
```

### Get the code for these examples in your workstation

So far, we have not created any C++ code. It is time to compile and deploy our application, as we will need the name and URL of the deployment to wire the remaining resources. First obtain the code:

```sh
git clone https://github.com/GoogleCloudPlatform/cpp-samples
# Output: Cloning into 'cpp-samples'...
#   additional informational messages
```

### Build Docker images for the sample programs


### Create a Cloud Spanner Instance to host your data

As mentioned above, this guide uses [Cloud Spanner] to store the data. We create the smallest possible instance. If needed we will scale up the instance, but this is economical and enough for running small jobs.

> :warning: Creating the Cloud Spanner instance incurs immediate billing costs, even if the instance is not used.

```sh
gcloud beta spanner instances create getting-started-cpp \
    --config=regional-us-central1 \
    --processing-units=100 \
    --description="Getting Started with C++"
# Output: Creating instance...done.
```

### Create the Cloud Spanner Database and Table for your data

A Cloud Spanner instance is just the allocation of compute resources for your databases. Think of them as a virtual set of database servers dedicated to your databases. Initially these servers have no databases or tables associated with the resources. We need to create a database and table that will host the data for this demo:

```sh
gcloud spanner databases create gcs-index \
    --ddl-file=gcs_objects.sql \
    --instance=getting-started-cpp
# Output: Creating database...done.
```

### Create a Cloud Pub/Sub Topic for Indexing Requests

Publishers send messages to Cloud Pub/Sub using a **topic**. These are named, persistent resources. We need to create one to configure the application.

```sh
gcloud pubsub topics create gcs-indexing-requests
# Output: Created topic [projects/..../topics/gcs-indexing-requests].
```

### Configure IAM permissions

#### Capture the project number

```sh
PROJECT_NUMBER=$(gcloud projects list \
    --filter="project_id=$GOOGLE_CLOUD_PROJECT" \
    --format="value(project_number)" \
    --limit=1)
# Output: none
```

#### Grant Cloud Pub/Sub permissions to use service accounts in your project

```sh
gcloud projects add-iam-policy-binding "$GOOGLE_CLOUD_PROJECT" \
    --member=serviceAccount:service-$PROJECT_NUMBER@gcp-sa-pubsub.iam.gserviceaccount.com \
    --role=roles/iam.serviceAccountTokenCreator
# Output: Updated IAM policy for project [$GOOGLE_CLOUD_PROJECT].
#    bindings:
#    ... full list of bindings for your project's IAM policy ...
```

### Wait for the build to complete

Look at the status of your build using:

```sh
gcloud builds list --ongoing
# Output: the list of running jobs
```

If your build has completed the list will be empty. If you need to wait for this build to complete (it should take about 15 minutes) use:

```sh
gcloud builds log --stream $(gcloud builds list --ongoing --format="value(id)")
# Output: the output from the build, streamed.
```

### Deploy the Programs to GKE

TODO(coryan)

### Use `gcloud` to send an indexing request

This will request indexing some public data. The prefix contains less than 100 objects:

```sh
gcloud pubsub topics publish gcs-indexing-requests \
    --attribute=bucket=gcp-public-data-landsat,prefix=LC08/01/006/001
# Output: messageIds:
#     - '....'
```

### Querying the data

The data should start appearing in the Cloud Spanner database. We can use the `gcloud` tool to query this data.

```sh
gcloud spanner databases execute-sql gcs-index --instance=getting-started-cpp \
    --sql="select * from gcs_objects where name like '%.txt' order by size desc limit 10"
# Output: metadata for the 10 largest objects with names finishing in `.txt`
```

## Optional: Scaling Up

> :warning: The following steps will incur significant billing costs. Use the [Pricing Calculator] to estimate the costs. If you are uncertain as to these costs, skip to the [Cleanup Section](#cleanup).

To scan a larger prefix we will need to scale up the Cloud Spanner instance. We use a `gcloud` command for this:

```sh
gcloud beta spanner instances update getting-started-cpp --processing-units=3000
# Output: Updating instance...done.
```

and then index a prefix with a few thousand objects

```sh
gcloud pubsub topics publish gcs-indexing-requests \
    --attribute=bucket=gcp-public-data-landsat,prefix=LC08/01/006
# Output: messageIds:
#     - '....'
```

You can monitor the work queue using the console:

```sh
google-chrome "https://console.cloud.google.com/cloudpubsub/subscription/detail/indexing-requests-cloud-run-push?project=$GOOGLE_CLOUD_PROJECT"
```

Or count the number of indexed objects:

```sh
gcloud spanner databases execute-sql gcs-index --instance=getting-started-cpp \
    --sql="select count(*) from gcs_objects"
# Output:
#    (Unspecified)    --> the count(*) column name
#    225446           --> the number of rows in the `gcs_objects` table (the actual number may be different)
```

It is also interesting to see the number of instances for the job:

```sh
google-chrome https://pantheon.corp.google.com/run/detail/us-central1/index-gcs-prefix/metrics?project=$GOOGLE_CLOUD_PROJECT
```

## Cleanup

> :warning: Do not forget to cleanup your billable resources after going through this "Getting Started" guide.

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
# Output: Deleted subscription [projects/$GOOGLE_CLOUD_PROJECT/subscriptions/indexing-requests-cloud-run-push].
```

### Remove the Cloud Pub/Sub Topic

```sh
gcloud pubsub topics delete gcs-indexing-requests --quiet
# Output: Deleted topic [projects/$GOOGLE_CLOUD_PROJECT/topics/gcs-indexing-requests].
```

### Remove the Container image

```sh
gcloud container images delete gcr.io/$GOOGLE_CLOUD_PROJECT/getting-started-cpp/index-gcs-prefix:latest --quiet
# Output: Deleted [gcr.io/$GOOGLE_CLOUD_PROJECT/getting-started-cpp/index-gcs-prefix:latest]
# Output: Deleted [gcr.io/$GOOGLE_CLOUD_PROJECT/getting-started-cpp/index-gcs-prefix@sha256:....]
```
