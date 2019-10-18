# Run the Speech sample using Docker

Follow instructions 1 through 4 in the [README.md](README.md) before continuing
with the following instructions.

1. **Service Account**

    Make sure the `GOOGLE_APPLICATION_CREDENTIALS` environment variable is
    pointing to the service account json file.

1. **Install Docker**

    Installing Docker is outside of scope for this README. Review the Docker
    [documentation][1] to get started.

1. **Build**

    Build the Docker image using the following command.

    ```bash
      docker build -t cpp-speech .
    ```

1. **Run Samples**

    Run the Docker image with mounting the service account json file.

    ```bash
      docker run \
        -v "${CPP_SPEECH_SERVICE_ACCOUNT}:/home/service-account.json" \
        cpp-speech bash -c "cd /home/speech/api; make run_tests"
    ```

[1]: https://docs.docker.com/engine/getstarted/
