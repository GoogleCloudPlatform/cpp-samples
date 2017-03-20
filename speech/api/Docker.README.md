# Run Speech Sample with Docker

## Service Account
Add a service account JSON file in this directory named `service-account.json`.
The service account file will be copied into the image produced by the
Dockerfile.

## Install Docker
Installing Docker is outside of scope for this README. Please review Docker
[installation documentation][1] for further instruction.

## Build
Build the docker image using the following command. You may change the name by
changing `cpp-speech` to something you prefer.

```
  $ docker build -t cpp-speech .
```

## Run Samples
Run the docker image using the following command. Note this will vary depending
on the image name used.

```
  $ docker run -it cpp-speech bash
  $ cd /home/cpp-docs-samples/speech/api/
  $ make run_tests
```


[1]: https://docs.docker.com/engine/getstarted/



