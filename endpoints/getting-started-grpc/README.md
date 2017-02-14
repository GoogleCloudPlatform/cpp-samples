# Endpoints Getting Started with gRPC & C++ Quickstart

It is assumed that you have a working C++ environment,
[ProtoBuf](https://github.com/google/protobuf/blob/master/src/README.md) and
[gRPC](http://www.grpc.io/docs/quickstart/cpp.html) installed, and a
Google Cloud account and [SDK](https://cloud.google.com/sdk/)
configured.

You either need Docker installed or the ability to compile Linux
64-bit static C++ binaries.

1. Build the local client and server.

    ```bash
    make
    ```

1. Test running the code, optional:

    ```bash
    # In the background or another terminal run the server:
    ./greeter_server

    # Check the client parameters:
    ./greeter_client

    # Run the client
    ./greeter_client localhost:50051 test
    ```

1. Generate the `out.pb` from the proto file.

    ```bash
    protoc --include_imports --include_source_info protos/helloworld.proto --descriptor_set_out out.pb
    ```

1. Edit, `api_config.yaml`. Replace `MY_PROJECT_ID` with your project id.

1. Deploy your service config to Service Management:

    ```bash
    gcloud service-management deploy out.pb api_config.yaml
    # The Config ID should be printed out, looks like: 2017-02-01r0, remember this

    # set your project to make commands easier
    GCLOUD_PROJECT=<Your Project ID>

    # Print out your Config ID again, in case you missed it
    gcloud service-management configs list --service hellogrpc.endpoints.${GCLOUD_PROJECT}.cloud.goog
    ```

1. Also get an API key from the Console's API Manager for use in the
   client later. (https://console.cloud.google.com/apis/credentials)

1. Build the static server binary. Either

  1. Use the Docker build:

      ```bash
      make clean
      docker build -t static-server-builder -f Dockerfile-builder .
      docker run --rm static-server-builder > greeter_server_static
      chmod +x greeter_server_static
      ```

  1. If you can build a static Linux x64 binary, just run:

      ```bash
      make greeter_server_static
      ```

1. Build a docker image for your gRPC server, store in your Registry

    ```bash
    gcloud container builds submit --tag gcr.io/${GCLOUD_PROJECT}/cpp-grpc-hello:1.0 .
    ```

1. Either deploy to GCE (below) or GKE (further down)

### GCE

1. Create your instance and ssh in.

    ```bash
    gcloud compute instances create grpc-host --image-family gci-stable --image-project google-containers --tags=http-server
    gcloud compute ssh grpc-host
    ```

1. Set some variables to make commands easier

    ```bash
    GCLOUD_PROJECT=$(curl -s "http://metadata.google.internal/computeMetadata/v1/project/project-id" -H "Metadata-Flavor: Google")
    SERVICE_NAME=hellogrpc.endpoints.${GCLOUD_PROJECT}.cloud.goog
    SERVICE_CONFIG_ID=<Your Config ID>
    ```

1. Pull your credentials to access Container Registry, and run your
   gRPC server container

    ```bash
    /usr/share/google/dockercfg_update.sh
    docker run -d --name=grpc-hello gcr.io/${GCLOUD_PROJECT}/cpp-grpc-hello:1.0
    ```

1. Run the Endpoints proxy

    ```bash
    docker run --detach --name=esp \
        -p 80:9000 \
        --link=grpc-hello:grpc-hello \
        gcr.io/endpoints-release/endpoints-runtime:1 \
        -s ${SERVICE_NAME} \
        -v ${SERVICE_CONFIG_ID} \
        -P 9000 \
        -a grpc://grpc-hello:50051
    ```

1. Back on your local machine, get the external IP of your GCE instance.

    ```bash
    gcloud compute instances list
    ```

1. Run the client

    ```bash
    ./greeter_client <IP of GCE Instance>:80 <API Key from Console>
    ```

1. Cleanup

    ```bash
    gcloud compute instances delete grpc-host
    ```

### GKE

1. Create a cluster

    ```bash
    gcloud container clusters create my-cluster
    ```

1. Edit `container-engine.yaml`. Replace `SERVICE_NAME`,
   `SERVICE_CONFIG_ID`, and `GCLOUD_PROJECT` with your values.

1. Deploy to GKE

    ```bash
    kubectl create -f ./container-engine.yaml
    ```

1. Get IP of load balancer, run until you see an External IP.

    ```bash
    kubectl get svc grpc-hello
    ```

1. Run the client

    ```bash
    ./greeter_client <IP of GKE LoadBalancer>:80 <API Key from Console>
    ```

1. Cleanup

    ```bash
    gcloud container clusters delete my-cluster
    ```
