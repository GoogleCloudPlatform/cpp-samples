# Using Cloud Batch

This example shows how to take a job.json and run a Cloud Batch job using C++.

If you are not familiar with the Batch API, we recommend you first read the
[API overview] before starting this guide.

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
cd cpp-samples/batch/simple
cmake -S . -B .build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .build
```

## Run the sample

Run the example, replace the `[PROJECT ID]` placeholder with the id of your
project:

```shell
.build/simple [PROJECT ID] us-central1 test-container-run hello-world-container.json
```

[api overview]: https://cloud.google.com/batch/docs
