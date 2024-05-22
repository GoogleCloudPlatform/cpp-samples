## The program

This example runs a Monte Carlo financial simulation that approximates the
future value of a stock price given the inputs.

# To build and run

```
cd cpp-samples/batch/parallel/application
cmake -S . -B build
cmake --build build
build/finsim input.txt
```

# To create docker image

```
docker build --tag=finsim-image:latest .
```

## To run and enter your image

```
docker run -it --entrypoint bash finsim-image:latest
```

To exit container, type `exit`
