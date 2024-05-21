# Using Cloud Batch

This example shows how to take an
[embarrasingly parallel](https://en.wikipedia.org/wiki/Embarrassingly_parallel)
job and run it on Cloud Batch job using C++.

If you are not familiar with the Batch API, we recommend you first read the
[API overview] before starting this guide.

## The example

The following steps are included:

1. Create a docker image
1. Upload it to Artifact registry
1. Create the Cloud Batch job

## Pre-reqs

1. Install the [gcloud CLI](https://cloud.google.com/sdk/docs/install).
1. Install [docker](https://docs.docker.com/engine/install/).

## 1. Create the docker image

The instructions are [here](application/README.md).

## 2. Upload it to Artifact registry

1. \[If it does not already exist\] Create the artifact repository
1. Build the image locally
1. Tag and push the image to the artifact repository

### 1. Create the artifact repository

To run this example, replace the `[PROJECT ID]` placeholder with the id of your
project:

Authorize via gcloud cli

```shell
PROJECT_ID=[PROJECT_ID]
LOCATION="us-central1"
REPOSITORY="parallel-repo"

gcloud auth login
gcloud config set project ${PROJECT_ID}
# Create the repository
gcloud artifacts repositories create ${REPOSITORY} \
    --repository-format=docker \
    --location=${LOCATION} \
    --description="Store the example parallel C++ application" \
    --async
```

<details>
  <summary>To verify repo was created</summary>
```
gcloud artifacts repositories list
```

You should see something like

```
parallel-repo                DOCKER  STANDARD_REPOSITORY  Store the example parallel C++ application  us-central1          Google-managed key  2024-05-21T12:39:54  2024-05-21T12:39:54  0
```

</details>

### 2. Build the image locally

```
cd batch/parallel/application
docker build --tag=fimsim-image:latest .
```

### 3. Tag and push the image to the artifact repository

```
cd batch/parallel/application
gcloud builds submit --region=${LOCATION} --tag ${LOCATION}-docker.pkg.dev/${PROJECT_ID}/${REPOSITORY}/finsim-image:latest
```

## 3. Create the job using the gcloud CLI

1. Replace the `imageURI` field in application.json

```
  "runnables": [
      {
          "container": {
              "imageUri": "${LOCATION_ID}-docker.pkg.dev/${PROJECT_ID}/{REPOSITORY}/finsim-image:latest",
          }
      }
  ],
```

2. Submit the job

```
gcloud batch jobs submit cpp-finsim-cli-run \
    --config=finsim.json \
    --location=us-central1
```

3. Check on the job status

```
gcloud batch jobs describe cpp-finsim-cli-run  --location=us-central1
```

[api overview]: https://cloud.google.com/batch/docs
