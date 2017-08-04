# Speech Samples.

These samples demonstrate how to call the [Google Cloud Speech API](https://cloud.google.com/speech/) using C++.

These samples will only build and run on **Linux**.

## Build and Run

1.  **Create a project in the Google Cloud Platform Console**.
    If you haven't already created a project, create one now. Projects enable
    you to manage all Google Cloud Platform resources for your app, including
    deployment, access control, billing, and services.
    1.  Open the [Cloud Platform Console](https://console.cloud.google.com/).
    1.  In the drop-down menu at the top, select Create a project.
    1.  Click Show advanced options. Under App Engine location, select a
        United States location.
    1.  Give your project a name.
    1.  Make a note of the project ID, which might be different from the project
        name. The project ID is used in commands and in configurations.

1.  **Enable billing for your project**.
    If you haven't already enabled billing for your project, [enable billing now](https://console.cloud.google.com/project/_/settings).
    Enabling billing allows the application to consume billable resources such
    as Speech API calls.  See [Cloud Platform Console Help](https://support.google.com/cloud/answer/6288653) for more information about billing settings.

1.  **Enable APIs for your project**.
    [Click here](https://console.cloud.google.com/flows/enableapi?apiid=speech&showconfirmation=true) to visit Cloud Platform Console and enable the Speech API.

1.  **Download service account credentials**.
    These samples use service accounts for authentication.
    1.  Visit the [Cloud Console](http://cloud.google.com/console), and navigate to:
    `API Manager > Credentials > Create credentials > Service account key`
    1.  Under **Service account**, select `New service account`.
    1.  Under **Service account name**, enter a service account name of your choosing.  For example, `transcriber`.
    1.  Under **Role**, select `Project > Service Account Actor`.
    1.  Under **Key type**, leave `JSON` selected.
    1.  Click **Create** to create a new service account, and download the json credentials file.
    1.  Set the `GOOGLE_APPLICATION_CREDENTIALS` environment variable to point to your downloaded service account credentials:
        ```
        export GOOGLE_APPLICATION_CREDENTIALS=/path/to/your/credentials-key.json
        ```
    See the [Cloud Platform Auth Guide](https://cloud.google.com/docs/authentication#developer_workflow) for more information.

1. [Optional] **Use Docker**
    You can use Docker to create an image with required dependencies and run
    the sample.
    1. Follow the instructions [here](Docker.README.md)

1.  **Install gRPC.**
    1.  Visit [the gRPC github repo](https://github.com/grpc/grpc) and follow the instructions to install gRPC.
    1.  Then, follow the instructions in the **Pre-requisites** section to install **protoc**.
    1.  The examples in this directory have been tested with gRPC 1.4.1 if they do not work for you please
        include the gRPC version you used in any bug reports.

1.  **Generate googleapis gRPC source code.**
    1.  Visit [the googleapis github repo](https://github.com/googleapis/googleapis) and follow the instructions to **Generate gRPC Source Code**.
        ```
        export LANGUAGE=cpp
        git clone https://github.com/googleapis/googleapis.git
        cd googleapis
        make all
        ```
    1.  Set the environment variable `GOOGLEAPIS_GENS_PATH` to the path where you generated the gRPC source code.  For example:
        ```
        export GOOGLEAPIS_GENS_PATH = $HOME/gitrepos/googleapis/gens
        ```

1.  **Download or clone this repo** with

    ```sh
    git clone https://github.com/GoogleCloudPlatform/cpp-docs-samples
    ```

1.  **Run the tests:**
    ```sh
    cd cpp-docs-sample/speech/api
    make run_tests
    ```
