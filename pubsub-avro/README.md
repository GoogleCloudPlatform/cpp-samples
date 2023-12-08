# Subscribe to avro records

## Overview

The quickstart shows how to subscribe to receive avro messages that could be for
different schema revisions. This example uses the [Avro C++] library and
[C++ Cloud Pub/Sub] library to use the [Cloud Pub/Sub] service. The setup
involves:

1. Creating an initial schema (Schema 1)
1. Creating a topic with Schema 1
1. Creating a subscription to the topic
1. Publishing a message to the topic with Schema 1
1. Commiting a revision schema (Schema 2)
1. Publishing a message to the topic with Schema 2
1. Recieve both messages using a subscriber

## Prerequisites

### 1. Create a project in the Google Cloud Platform Console

If you haven't already created a project, create one now.

Projects enable you to manage all Google Cloud Platform resources for your app,
including deployment, access control, billing, and services.

1. Open the [Cloud Platform Console](https://console.cloud.google.com/).
1. In the drop-down menu at the top, select Create a project.
1. Give your project a name.
1. Make a note of the project ID, which might be different from the project
   name. The project ID is used in commands and in configurations.

### 2. Enable billing for your project

If you haven't already enabled billing for your project,
[enable billing now](https://console.cloud.google.com/project/_/settings).
Enabling billing allows the application to consume billable resources such as
Pub/Sub API calls.

See
[Cloud Platform Console Help](https://support.google.com/cloud/answer/6288653)
for more information about billing settings.

### 3. Enable APIs for your project

[Click here](https://console.cloud.google.com/flows/enableapi?apiid=speech&showconfirmation=true)
to visit Cloud Platform Console and enable the Pub/Sub and Trace API via the UI.

Or use the CLI:

```
gcloud services enable pubsub.googleapis.com
```

## Build using CMake and Vcpkg

### 1. Install vcpkg

This project uses [`vcpkg`](https://github.com/microsoft/vcpkg) for dependency
management. Clone the vcpkg repository to your preferred location. In these
instructions we use`$HOME`:

```shell
git clone -C $HOME https://github.com/microsoft/vcpkg.git
cd $HOME/vcpkg
./vcpkg install google-cloud-cpp
```

### 2. Download or clone this repo

```shell
git clone https://github.com/GoogleCloudPlatform/cpp-samples
```

### 3. Compile these examples

Use the `vcpkg` toolchain file to download and compile dependencies. This file
would be in the directory you cloned `vcpkg` into, `$HOME/vcpkg` if you are
following the instructions to the letter. Note that building all the
dependencies can take up to an hour, depending on the performance of your
workstation. These dependencies are cached, so a second build should be
substantially faster.

```sh
cd cpp-samples/pubsub-open-telemetry
cmake -S . -B .build -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja
cmake --build .build
```

### (optional) 4. Generate C++ representation of the avro schema using the avro compiler

vcpkg installs the avro compiler. This repository contains two generated `.hh`
files for the corresponding `.avro` schemas. To generate those headers, use the
avro compiler:

```
.build/vcpkg_installed/x64-linux/tools/avro-cpp/avrogencpp -i schema1.avro -o schema1.hh -n v1
.build/vcpkg_installed/x64-linux/tools/avro-cpp/avrogencpp -i schema2.avro -o schema2.hh -n v2
```

If you want to try using this example with own schema, you can replace the test
file with your own and modify the code as needed.

## Setup the Pub/Sub messages

Export the following environment variables to run the setup scripts:

```shell
export GOOGLE_CLOUD_PROJECT=<project-id>
export GOOGLE_CLOUD_TOPIC=avro-topic
export GOOGLE_CLOUD_SUBSCRIPTION=avro-sub
export GOOGLE_CLOUD_SCHEMA_NAME=state
export GOOGLE_CLOUD_SCHEMA_FILE1=schema1.avro
export GOOGLE_CLOUD_SCHEMA_FILE2=schema2.avro
```

```shell
chmod +x ./setup.sh
./setup.sh
```

## Run the example

This will resolve the schemas when recieving the message and return data in the
format of schema2, even if it was sent in the format of schema1.

```sh
.build/quickstart alevenb-test avro-sub schema2.avro
```

If you want to send more message to test, you can use the following commands to
send a message in schema1

```sh
gcloud pubsub topics publish ${GOOGLE_CLOUD_TOPIC} \
--project ${GOOGLE_CLOUD_PROJECT} \
--message '{"name": "New York", "post_abbr": "NY"}'
```

Or in schema2

```sh
gcloud pubsub topics publish ${GOOGLE_CLOUD_TOPIC} \
  --project ${GOOGLE_CLOUD_PROJECT} \
  --message '{"name": "New York", "post_abbr": "NY", "population": 10000}'
```

## Cleanup

To delete the created resources (topic, subscription, schema), run:

```shell
chmod +x ./cleanup.sh
./cleanup.sh
```

## Platform Specific Notes

### macOS

gRPC [requires][grpc-roots-pem-bug] an environment variable to configure the
trust store for SSL certificates, you can download and configure this using:

```bash
curl -Lo roots.pem https://pki.google.com/roots.pem
export GRPC_DEFAULT_SSL_ROOTS_FILE_PATH="$PWD/roots.pem"
```

### Windows

gRPC [requires][grpc-roots-pem-bug] an environment variable to configure the
trust store for SSL certificates, you can download and configure this using:

```console
@powershell -NoProfile -ExecutionPolicy unrestricted -Command ^
    (new-object System.Net.WebClient).Downloadfile( ^
        'https://pki.google.com/roots.pem', 'roots.pem')
set GRPC_DEFAULT_SSL_ROOTS_FILE_PATH=%cd%\roots.pem
```

[avro c++]: https://avro.apache.org/docs/1.11.1/api/cpp/html/
[c++ cloud pub/sub]: https://cloud.google.com/cpp/docs/reference/pubsub/latest
[cloud pub/sub]: https://cloud.google.com/pubsub/docs
[grpc-roots-pem-bug]: https://github.com/grpc/grpc/issues/16571
