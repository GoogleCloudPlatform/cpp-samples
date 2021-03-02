# Download GCS objects using multiple parallel streams


## Compiling

This project uses `vcpkg` to install its dependencies. Clone `vcpkg` in your `$HOME`:

```shell
git clone -C $HOME https://github.com/microsoft/vcpkg.git
```

Install the typical development tools, on Ubuntu you would use:

```shell
apt update && apt install -y build-essential cmake git ninja-build pkg-config g++ curl tar zip unzip
```

In this directory compile the dependencies and the code, this can take as long as an hour, depending
on the performance of your workstation:

```shell
cd cpp-samples/gcs-parallel-download
cmake -S. -B.build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .build
```

The program will be in `.build/gcs_parallel_download`.

