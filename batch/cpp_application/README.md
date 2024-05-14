# Using Cloud Batch

This example shows how to run a C++ application on Cloud Batch job using C++.

If you are not familiar with the Batch API, we recommend you first read the
[API overview] before starting this guide.

## The example

The following steps are included:

1. Create a docker image
1. Upload it to Artifact registry
1. Create the job
1. Poll until the job finishes

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
REPOSITORY="application-repo"

gcloud auth login
gcloud config set project ${PROJECT_ID}
# Create the repository
gcloud artifacts repositories create ${REPOSITORY} \
    --repository-format=docker \
    --location=${LOCATION} \
    --description="Store the example C++ application" \
    --immutable-tags \
    --async
```

<details>
  <summary>To verify repo was created</summary>
```
gcloud artifacts repositories list
```

You should see something like

```
application-repo            DOCKER  STANDARD_REPOSITORY  Store the example C++ application  us-central1          Google-managed key  2024-05-13T20:07:11  2024-05-13T20:07:11  0
```

</details>

### 2. Build the image locally

```
cd batch/cpp_application/application
docker build --tag=application-image:latest .
```

### 3. Tag and push the image to the artifact repository

```
# Tag the image
docker tag application-image:latest ${LOCATION}-docker.pkg.dev/${PROJECT_ID}/${REPOSITORY}/application-image:latest

# Push the image
docker push ${LOCATION}-docker.pkg.dev/${PROJECT_ID}/${REPOSITORY}/application-image:latest
```

## 3. Create the job

## Compiling the Example

This project uses `vcpkg` to install its dependencies. Clone `vcpkg` in your
`$HOME`:

```shell
git clone -C $HOME https://github.com/microsoft/vcpkg.git
```

Install the typical development tools, on Ubuntu you would use:

```shell
apt update && apt install -y build-essential cmake git ninja-build pkg-config g++ curl tar zip unzip
```

In this directory compile the dependencies and the code, this can take as long
as an hour, depending on the performance of your workstation:

```shell
cd cpp-samples/batch/cpp_application
cmake -S . -B .build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .build
```

## Run the sample

Run the example, replace the `[PROJECT ID]` placeholder with the id of your
project:

```shell
.build/main [PROJECT ID] us-central1 cpp-application-run application.json application-repo
```

This submits the batch job and then polls until the job is complete.

[api overview]: https://cloud.google.com/batch/docs
