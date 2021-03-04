# Fast GCS File Transfer

## Status

This software is offered on an _"AS IS", EXPERIMENTAL_ basis, and only guaranteed to demonstrate concepts -- NOT to act
as production data transfer software. Any and all usage of it is at your sole discretion. Any costs or damages resulting
from its use are the sole responsibility of the user. You are advised to read and understand all source code in this
software before using it for any reason.

---

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

The program will be in `.build/download`.

## Downloading objects

The program receives the bucket name, object name, and destination file as parameter in the command-line, for example:

```shell
.build/download my-bucket gcs-does-not-have-folders/my-large-object.bin destination.bin
```

Will download an object named `gcs-does-not-have-folders/my-large-file.bin` in bucket `my-bucket` to the destination
file `destination.bin`.

The program uses approximately 2 threads per core (or vCPU) to download an object. To change the number of threads use
the `--thread-count` option. For small objects, the program may use fewer threads, you can tune this behavior by setting
the `--minimum-slice-size` to a smaller number.

## Usage

```
usage: download [options] bucket object destination

Download a single GCS object using multiple slices:
--help                               produce help message
--bucket arg                         set the GCS bucket to download from
--object arg                         set the GCS object to download
--destination arg                    set the destination file to download
                                     into
--thread-count arg (=192)            number of parallel streams for the
                                     download
--minimum-slice-size arg (=67108864) minimum slice size
```
