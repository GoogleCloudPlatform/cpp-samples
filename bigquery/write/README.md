# Using BigQuery Storage Write

This example shows how to upload some data to BigQuery using the BigQuery Storage API.
For simplicity, the example uses a hard-coded dataset, table, and schema. It uses
the default "write stream" and always uploads the same data.

If you are not familiar with the BigQuery Storage Write API, we recommend you
first read the [API overview] before starting this guide.

## Compiling the Example

This project uses `vcpkg` to install its dependencies. Clone `vcpkg` in your `$HOME`:

```shell
git clone -C $HOME https://github.com/microsoft/vcpkg.git
```

Install the typical development tools, on Ubuntu you would use:

```shell
apt update && apt install -y build-essential cmake git ninja-build pkg-config g++ curl tar zip unzip
```

In this directory compile the dependencies and the code, this can take as long as an hour, depending on the performance
of your workstation:

```shell
cd cpp-samples/bigquery/write
cmake -S . -B .build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .build
```

The program will be in `.build/single_threaded_write`.

## Pre-requisites

You are going to need a Google Cloud project to host the BigQuery dataset and table used in this example.
You will need to install and configure the BigQuery CLI tool. Follow the
[Google Cloud CLI install][install-sdk] instructions, and then the [quickstart][BigQuery CLI tool] for
the BigQuery CLI tool.

Verify the CLI is working using a simple command to list the active project:

```shell
bq show
```

## Create the data set and table

Create a dataset and table for this example:

```shell
bq mk cpp_samples
bq mk cpp_samples.singers
```

Then configure the schema in the new table:

```shell
bq update cpp_samples.singers schema.json
```

## Run the sample

Run the example, replace the `[PROJECT ID]` placeholder with the id of your project:

```shell
.build/single_threaded_write [PROJECT ID]
```

Verify the example uploaded some data:

```shell
bq query 'select * from cpp_samples.singers limit 5'
```

## Cleanup

Remove the table and dataset:

```shell
bq rm -f cpp_samples.singers
bq rm -f cpp_samples
```

[API overview]: https://cloud.google.com/bigquery/docs/write-api
[BigQuery CLI tool]: https://cloud.google.com/bigquery/docs/bq-command-line-tool
[install-sdk]: https://cloud.google.com/sdk/docs/install-sdk