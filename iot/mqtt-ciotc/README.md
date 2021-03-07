# Message Queue Telemetry Transport (MQTT) client for Google Cloud IoT Core

This sample app demonstrates connecting to Google Cloud IoT Core and publishing
a single message.

## Building

This example uses `vcpkg` and `CMake` to manage its dependencies. To compile this project:

1. **Install vcpkg.**
   Clone the vcpkg repository to your preferred location. In these instructions we use`$HOME`:
   ```shell
   git clone -C $HOME https://github.com/microsoft/vcpkg.git
   ```

1. **Download or clone this repo** with
   ```shell
   git clone https://github.com/GoogleCloudPlatform/cpp-samples
   ```

1. **Compile this example:**
   Use the `vcpkg` toolchain file to download and compile dependencies. This file would be in the directory you
   cloned `vcpkg` into, `$HOME/vcpkg` if you are following the instructions to the letter. Note that building all the
   dependencies may take several minutes, up to half an hour, depending on the performance of your workstation. These dependencies are cached,
   so a second build should be substantially faster.
   ```sh
   cd cpp-samples/iot/mqtt-ciotc
   cmake -S. -B.build -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build .build
   ```

# Running

After you have successfully built the sample, you are almost ready to run the
sample.

1. Browse to the sample folder
    ```shell
    cd mqtt-ciotc
    ```

1. Create a Cloud Pub/Sub topic
   ```shell
   gcloud pubsub topics create <Your telemetry topic>
   ```

1. Setup the Cloud IOT environment
   ```shell
    ./setup_device.sh --registry-name <Your registry id> \
      --registry-region <e.g. us-central1> \
      --device-id <Your device ID> \
      --telemetry-topic <Your telemetry Cloud Pub/Sub topic>
   ```

1. Run the sample
   ```shell
    ./mqtt_ciotc <message> \
        --deviceid <your device id>\
        --region <e.g. us-central1>\
        --registryid <your registry id>\
        --projectid <your project id>\
        --keypath <e.g. ./ec_private.pem>\
        --algorithm <e.g. RS256>
        --rootpath <e.g. ./roots.pem>
   ```

You must provide the deviceid, registryid, and projectid parameters as they are
used in calculating the client ID used for connecting to the MQTT bridge. The
ecpath parameter should point to your private EC key created when you registered
your device. The rootpath parameter specifies the roots.pem file that can be
downloaded from https://pki.google.com/roots.pem. For example:

   ```shell
   wget https://pki.google.com/roots.pem
   ```

The following example demonstrates usage of the sample if my device ID is
my-device, registry ID is my-registry, and project ID is blue-ocean-123.

   ```shell
    mqtt_ciotc "Hello world!" --deviceid my-device --registryid my-registry \
        --projectid blue-ocean-123 --keypath ./rsa_private.pem --algorithm RS256 \
         --rootpath ./roots.pem --region us-central1    
   ```
