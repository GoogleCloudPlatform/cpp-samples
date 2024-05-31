# Using BigQuery Storage Read

Cloud BigQuery is a data platform that allows users to easily create, manage,
share, and query data using SQL. When you want to access your data, you can read
directly from a table. However, if you want to transform the data in a table by
mapping, filtering, or joining, you need to first make a query. When you make a
query, you can specify a table to store the results. Then you can start a read
session for the table via the BigQuery Storage library and read the rows from
the table.

This example shows how to create a query job using the BigQuery v2 Python API,
and then read the data from the table using the BigQuery Storage C++ API.

If you are not familiar with the BigQuery v2 API or the BigQuery Storage Read
API, we recommend you first read the [API overview] before starting this guide.

## Pre-requisites

You are going to need a Google Cloud project to host the BigQuery dataset and
table used in this example. You will need to install and configure the BigQuery
CLI tool. Follow the [Google Cloud CLI install][install-sdk] instructions, and
then the [quickstart][bigquery cli tool] for the BigQuery CLI tool.

Verify the CLI is working using a simple command to list the active project:

```shell
bq show
```

### Creating the query job

The following script uses the BigQuery v2 python client to create a dataset (if
it does not already exist) and a query job.

```
python3 -m venv env
source env/bin/activate
pip3 install -r requirements.txt
python3 create_query_job.py --project_id [PROJECT-ID] --dataset_name usa_names --table_name top10_names
```

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
cd cpp-samples/bigquery/read
cmake -S . -B .build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .build
```

## Run the sample

Run the example, replace the `[PROJECT ID]` placeholder with the id of your
project:

```shell
.build/arrow_read [PROJECT ID] [DATASET_NAME] [TABLE_NAME]
```

```shell
.build/arrow_read [PROJECT ID] usa_names top10_names
```

## Output

Your output should look like the following:

```
Schema is:
 name: string
total: int64
       name            total
Row 0: James           4942431
Row 1: John            4834422
Row 2: Robert          4718787
Row 3: Michael         4297230
Row 4: William         3822209
Row 5: Mary            3737679
Row 6: David           3549801
Row 7: Richard         2531924
Row 8: Joseph          2472917
Row 9: Charles         2244693
Read 1 record batch(es) and 10 total row(s) from table: projects/[PROJECT-ID]/datasets/usa_names/tables/top10_names
```

## Cleanup

Remove the table and dataset:

```shell
bq rm -f usa_names.top10
bq rm -f usa_names
```

[api overview]: https://cloud.google.com/bigquery/docs/reference/storage
[bigquery cli tool]: https://cloud.google.com/bigquery/docs/bq-command-line-tool
[install-sdk]: https://cloud.google.com/sdk/docs/install-sdk
