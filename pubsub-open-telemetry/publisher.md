# Publisher

The publisher application lets the user configure a tracing enabled Pub/Sub Publisher client to see how different configuration settings change the produced telemetry data. 

## Example traces 

### Cloud Trace

To find the traces, navigate to the Cloud Trace UI.

#### Publish trace

![Screenshot of the publish span in the Cloud Trace UI running publisher.](assets/publish_span.png)

#### Create trace

![Screenshot of the create span in the Cloud Trace UI running publisher.](assets/create_span.png)

## Build and run

For setup instructions, refer to the [README.md](README.md).

### Using CMake and Vcpkg

#### Run basic publisher examples
```shell
.build/publisher [project-name] [topic-id]
.build/publisher [project-name] [topic-id] -n 1000
.build/publisher [project-name] [topic-id] --message-size 0
.build/publisher [project-name] [topic-id] --tracing-rate 0.01 -n 10
```

#### Flow control example
```shell
.build/publisher [project-name] [topic-id] -n 5 --max-pending-messages 2 --publisher-action reject
.build/publisher [project-name] [topic-id] -n 5 --max-pending-messages 2 --publisher-action block
.build/publisher [project-name] [topic-id] -n 5 --max-pending-messages 2 --publisher-action ignore
.build/publisher [project-name] [topic-id] -n 5 --message-size 10 --max-batch-bytes  60 --publisher-action block
```

#### Batching example
```shell
.build/publisher [project-name] [topic-id] -n 5 --max-batch-messages 2 --max-hold-time 100
.build/publisher [project-name] [topic-id] -n 5 --message-size 10 --max-batch-bytes 60  --max-hold-time 1000
```

#### To see all options

```shell 
.build/publisher --help
Usage: .build/publisher <project-id> <topic-id>
A simple publisher application with Open Telemetery enabled:
  -h [ --help ]                   produce help message
  --project-id arg                the name of the Google Cloud project
  --topic-id arg                  the name of the Google Cloud topic
  --tracing-rate arg (=1)         otel::BasicTracingRateOption value
  --max-queue-size arg (=2048)    set the max queue size for open telemetery
  -n [ --message-count ] arg (=1) the number of messages to publish
  --message-size arg (=1)         the desired message payload size
  --max-pending-messages arg      pubsub::MaxPendingMessagesOption value
  --max-pending-bytes arg         pubsub::MaxPendingBytesOption value
  --publisher-action arg          pubsub::FullPublisherAction value 
                                  (block|ignore|reject)
  --max-hold-time arg             pubsub::MaxHoldTimeOption value in us
  --max-batch-bytes arg           pubsub::MaxBatchBytesOption value
  --max-batch-messages arg        pubsub::MaxBatchMessagesOption value
```

### Using Bazel

#### Run basic publisher examples
```shell
bazel run //:publisher [project-name] [topic-id]
bazel run //:publisher -- [project-name] [topic-id] -n 1000
bazel run //:publisher -- [project-name] [topic-id] --message_size 0
bazel run //:publisher -- [project-name] [topic-id] --tracing-rate 0.01 -n 10
```
