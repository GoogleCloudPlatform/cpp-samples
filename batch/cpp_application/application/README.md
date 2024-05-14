# To build and run

```
cmake -S . -B build
cmake --build build
build/main
```

# To create docker image

```
docker build --tag=application-image:latest .
```

## To run and enter your image

```
docker run -it --entrypoint bash application-image:latest
```

To exit container, type `exit`
