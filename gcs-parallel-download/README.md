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

In this directory compile the dependencies and the code, this can take as long as an hour, depending on the performance
of your workstation:

```shell
cd cpp-samples/gcs-parallel-download
cmake -S. -B.build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .build
```

The program will be in `.build/gcs_parallel_download`.

## Downloading objects

The program receives the bucket name, object name, and destination file as parameter in the command-line, for example:

```shell
.build/gcs_parallel_download my-bucket folder/my-large-object.bin destination.bin
```

Will download an object named `folder/my-large-file.bin` in bucket `my-bucket` to the destination file
`destination.bin`.

The program uses approximately 2 threads per core (or vCPU) to download an object. To change the number of threads use
the `--thread-count` option. For small objects, the program may use fewer threads, you can tune this behavior by setting
the `--minimum-slice-size` to a smaller number.
