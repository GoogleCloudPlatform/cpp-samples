# Speech Samples.

These samples demonstrate how to call the [Google Cloud Speech API](https://cloud.google.com/speech/) using C++.

We only test these samples on **Linux**.  If you are running [Windows](#Windows) and [macOS](#macOS) please see
the additional notes for your platform.

## Build and Run

1. **Create a project in the Google Cloud Platform Console**. If you haven't already created a project, create one now.
   Projects enable you to manage all Google Cloud Platform resources for your app, including deployment, access control,
   billing, and services.
    1. Open the [Cloud Platform Console](https://console.cloud.google.com/).
    1. In the drop-down menu at the top, select Create a project.
    1. Give your project a name.
    1. Make a note of the project ID, which might be different from the project name. The project ID is used in commands
       and in configurations.

1. **Enable billing for your project**. If you haven't already enabled billing for your
   project, [enable billing now](https://console.cloud.google.com/project/_/settings). Enabling billing allows the
   application to consume billable resources such as Speech API calls.
   See [Cloud Platform Console Help](https://support.google.com/cloud/answer/6288653) for more information about billing
   settings.

1. **Enable APIs for your project**.
   [Click here](https://console.cloud.google.com/flows/enableapi?apiid=speech&showconfirmation=true) to visit Cloud
   Platform Console and enable the Speech API.

1. **If needed, override the Billing Project**.
   If you are using a [user account] for authentication, you need to set the `GOOGLE_CLOUD_CPP_USER_PROJECT`
   environment variable to the project you created in the previous step. Be aware that you must have
   `serviceusage.services.use` permission on the project.  Alternatively, use a service account as described next.

[user account]: https://cloud.google.com/docs/authentication#principals 

1. **Download service account credentials**. These samples can use service accounts for authentication.
    1. Visit the [Cloud Console](http://cloud.google.com/console), and navigate to:
       `API Manager > Credentials > Create credentials > Service account key`
    1. Under **Service account**, select `New service account`.
    1. Under **Service account name**, enter a service account name of your choosing. For example, `transcriber`.
    1. Under **Role**, select `Project > Owner`.
    1. Under **Key type**, leave `JSON` selected.
    1. Click **Create** to create a new service account, and download the json credentials file.
    1. Set the `GOOGLE_APPLICATION_CREDENTIALS` environment variable to point to your downloaded service account
       credentials:
       ```
       export GOOGLE_APPLICATION_CREDENTIALS=/path/to/your/credentials-key.json
       ```
   See the [Cloud Platform Auth Guide](https://cloud.google.com/docs/authentication#developer_workflow) for more
   information.

1. **Install vcpkg.**
   This project uses [`vcpkg`](https://github.com/microsoft/vcpkg) for dependency management. Clone the vcpkg repository
   to your preferred location. In these instructions we use`$HOME`:
   ```shell
   git clone -C $HOME https://github.com/microsoft/vcpkg.git
   ```

1. **Download or clone this repo** with
   ```shell
   git clone https://github.com/GoogleCloudPlatform/cpp-samples
   ```

1. **Compile these examples:**
   Use the `vcpkg` toolchain file to download and compile dependencies. This file would be in the directory you
   cloned `vcpkg` into, `$HOME/vcpkg` if you are following the instructions to the letter. Note that building all the
   dependencies can take up to an hour, depending on the performance of your workstation. These dependencies are cached,
   so a second build should be substantially faster.
   ```sh
   cd cpp-samples/speech/api
   cmake -S. -B.build -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build .build
   ```

1. **Run the examples:**
   ```shell
   .build/transcribe --bitrate 16000 resources/audio2.raw
   .build/transcribe resources/audio.flac
   .build/transcribe resources/quit.raw
   .build/streaming_transcribe --bitrate 16000 resources/audio2.raw
   .build/streaming_transcribe resources/audio.flac
   .build/streaming_transcribe resources/quit.raw
   .build/streaming_transcribe_coroutines --bitrate 16000 resources/audio2.raw
   .build/streaming_transcribe_coroutines resources/audio.flac
   .build/streaming_transcribe_coroutines resources/quit.raw
   .build/streaming_transcribe_singlethread ---bitrate 16000 resources/audio.raw
   .build/transcribe gs://cloud-samples-tests/speech/brooklyn.flac
   .build/async_transcribe gs://cloud-samples-tests/speech/vr.flac
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

[grpc-roots-pem-bug]: https://github.com/grpc/grpc/issues/16571
[choco-cmake-link]: https://chocolatey.org/packages/cmake
[homebrew-cmake-link]: https://formulae.brew.sh/formula/cmake
[cmake-download-link]: https://cmake.org/download/
