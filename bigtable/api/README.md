# Bigtable Samples.

[![Build Status](https://travis-ci.org/coryan/cpp-docs-samples.svg?branch=master)](https://travis-ci.org/coryan/cpp-docs-samples) [![Build status](https://ci.appveyor.com/api/projects/status/51melcqj0g1whoug?svg=true)](https://ci.appveyor.com/project/coryan/cpp-docs-samples) 

These samples demonstrate how to call the [Google Cloud Bigtable API](https://cloud.google.com/bigtable/) using C++.

## Build and Run

1.  **Create a project in the Google Cloud Console**.
    If you haven't already created a project, create one now. Projects enable
    you to manage all Google Cloud Platform resources for your app, including
    deployment, access control, billing, and services.
    1.  Open the [Google Cloud Console](https://console.cloud.google.com/).
    1.  In the drop-down menu at the top, select Create a project.
    1.  Click Show advanced options. Under App Engine location, select a
        United States location.
    1.  Give your project a name.
    1.  Make a note of the project ID, which might be different from the project
        name. The project ID is used in commands and in configurations.

1.  **Enable billing for your project**.
    If you haven't already enabled billing for your project, [enable billing now](https://console.cloud.google.com/project/_/settings).
    Enabling billing allows the application to consume billable resources such
    as Cloud Bigtable API calls.
    See [Google Cloud Console Help](https://support.google.com/cloud/answer/6288653) for more information about billing settings.

1.  **Enable the Cloud Bigtable Admin APIs for your project**.
    [Click here](https://console.cloud.google.com/flows/enableapi?apiid=bigtableadmin&showconfirmation=true) to visit Google Cloud Console and enable the Bigtable Admin API.

1.  **Enable the Cloud Bigtable APIs for your project**.
    [Click here](https://console.cloud.google.com/flows/enableapi?apiid=bigtable&showconfirmation=true) to visit Google Cloud Console and enable the Bigtable Admin API.

1.  **Download service account credentials**.
    These samples use service accounts for authentication.
    1.  Visit the [Google Cloud Console](http://cloud.google.com/console), and navigate to:
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

1.  **Install gRPC.**
    1.  Visit [the gRPC github repo](https://github.com/grpc/grpc) and follow the instructions to install gRPC.
    1.  Then, follow the instructions in the **Pre-requisites** section to install **protoc**.

1.  **Download or close this repo** with
    ```console
    git clone https://github.com/GoogleCloudPlatform/cpp-docs-samples
    cd cpp-docs-samples
    git submodule update --init
    ```

1.  **Generate googleapis gRPC source code.**
    ```console
    cd bigtable/api
    env -u LANGUAGE make -C googleapis OUTPUT=$PWD/googleapis-gens
    ```

1.  **Compile the examples**
    ```console
    cmake .
    make -j 2
    ```

1.  **Run the examples**
    ```console
    # This should be the name of the project you enabled billing and the APIs for.
    PROJECT=<your project here>

    # ... outside GCE you may need to set:
    # export GOOGLE_APPLICATION_CREDENTIALS=<path to service account private key file>

    ./list_instances $PROJECT

    ./create_instance $PROJECT bt-test-instance cluster-00 us-east1-c

    ./list_instances $PROJECT

    ./create_table $PROJECT bt-test-instance my-table

    ./list_tables $PROJECT bt-test-instance

    ./delete_table $PROJECT bt-test-instance my-table

    ./create_table $PROJECT bt-test-instance daily
    ./create_table $PROJECT bt-test-instance quote-per-row

    ./resources/download_taq.sh

    ./upload_taq $PROJECT bt-test-instance quote-per-row NBBO.txt

    ./upload_taq_batch $PROJECT bt-test-instance quote-per-row 20161024 NBBO.txt

    ./read_row $PROJECT bt-test-instance quote-per-row 20161024

    ./delete_instance $PROJECT bt-test-instance

    ./list_instances $PROJECT
    ```

