# Speech Samples.

These samples demonstrate how to call the [Google Cloud Speech API](https://cloud.google.com/speech/) using C++.

## Build and Run

1.  **Create a project in the Google Cloud Platform Console**.
    If you haven't already created a project, create one now. Projects enable
    you to manage all Google Cloud Platform resources for your app, including
    deployment, access control, billing, and services.
    1.  Open the [Cloud Platform Console](https://console.cloud.google.com/).
    2.  In the drop-down menu at the top, select Create a project.
    3.  Click Show advanced options. Under App Engine location, select a
        United States location.
    4.  Give your project a name.
    5.  Make a note of the project ID, which might be different from the project
        name. The project ID is used in commands and in configurations.

2.  **Enable billing for your project**.
    If you haven't already enabled billing for your project, [enable billing now](https://console.cloud.google.com/project/_/settings).
    Enabling billing allows the application to consume billable resources such
    as Speech API calls.  See [Cloud Platform Console Help](https://support.google.com/cloud/answer/6288653) for more information about billing settings.

3.  **Enable APIs for your project**.
    [Click here](https://console.cloud.google.com/flows/enableapi?apiid=speech&showconfirmation=true) to visit Cloud Platform Console and enable the Speech API.

4.  **Download service account credentials**.
    These samples use service accounts for authentication.
    1.  Visit the [Cloud Console](http://cloud.google.com/console), and navigate to:
    `API Manager > Credentials > Create credentials > Service account key`
    2.  Under **Service account**, select `New service account`.
    3.  Under **Service account name**, enter a service account name of your choosing.  For example, `transcriber`.
    4.  Under **Role**, select `Project > Service Account Actor`.
    5.  Under **Key type**, leave `JSON` selected.
    2.  Click **Create** to create a new service account, and download the json credentials file.
    3.  Set the `GOOGLE_APPLICATION_CREDENTIALS` environment variable to point to your downloaded service account credentials:
        ```
        export GOOGLE_APPLICATION_CREDENTIALS=/path/to/your/credentials-key.json
        ```
    See the [Cloud Platform Auth Guide](https://cloud.google.com/docs/authentication#developer_workflow) for more information.

4.  **Install gRPC.**
    1.  Visit [the gRPC github repo](https://github.com/grpc/grpc) and follow the instructions to install gRPC.
    2.  Then, follow the instructions in the **Pre-requisites** section to install **protoc**.

5.  **Generate googleapis gRPC source code.**
    1.  Set the environment variable:
        ```
        export LANGUAGE=cpp
        ```
    2.  Visit [the googleapis github repo](https://github.com/googleapis/googleapis) and follow the instructions to generate gRPC source code.
    3.  Set the environment variable `GOOGLEAPIS_GENS_PATH` to the path where you generated the gRPC source code.  For example:
        ```
        export GOOGLEAPIS_GENS_PATH = $HOME/gitrepos/googleapis/gens
        ```

6.  **Download or clone this repo** with

    ```sh
    git clone https://github.com/GoogleCloudPlatform/cpp-docs-samples
    ```

7.  **Run the tests:**
    ```sh
    make run_tests
    ```
