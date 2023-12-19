// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "avro/Compiler.hh"
#include "avro/DataFile.hh"
#include "avro/Decoder.hh"
#include "avro/Stream.hh"
#include "avro/ValidSchema.hh"
#include "google/cloud/pubsub/message.h"
#include "google/cloud/pubsub/schema_client.h"
#include "google/cloud/pubsub/subscriber.h"
#include "google/cloud/pubsub/subscription.h"
#include "schema1.h"
#include "schema2.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) try {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <project-id> <subscription-id> <avro-file>\n";
    return 1;
  }

  std::string const project_id = argv[1];
  std::string const subscription_id = argv[2];
  std::string const avro_file = argv[3];

  auto constexpr kWaitTimeout = std::chrono::seconds(30);

  // Create a namespace alias to make the code easier to read.
  namespace pubsub = ::google::cloud::pubsub;

  //! [START pubsub_subscribe_avro_records_with_revisions]
  auto subscriber = pubsub::Subscriber(pubsub::MakeSubscriberConnection(
      pubsub::Subscription(project_id, subscription_id)));

  // Create a schema client.
  auto schema_client =
      pubsub::SchemaServiceClient(pubsub::MakeSchemaServiceConnection());

  // Read the reader schema. This is the schema you want the messages to be
  // evaluated using.
  std::ifstream ifs(avro_file);
  avro::ValidSchema reader_schema;
  avro::compileJsonSchema(ifs, reader_schema);

  std::unordered_map<std::string, avro::ValidSchema> revisions_to_schemas;
  auto session = subscriber.Subscribe(
      [&](pubsub::Message const& message, pubsub::AckHandler h) {
        // Get the reader schema revision for the message.
        auto schema_name = message.attributes()["googclient_schemaname"];
        auto schema_revision_id =
            message.attributes()["googclient_schemarevisionid"];
        // If we haven't received a message with this schema, look it up.
        if (revisions_to_schemas.find(schema_revision_id) ==
            revisions_to_schemas.end()) {
          auto schema_path = schema_name + "@" + schema_revision_id;
          // Use the schema client to get the path.
          auto schema = schema_client.GetSchema(schema_path);
          if (!schema) {
            std::cout << "Schema not found:" << schema_path << "\n";
            return;
          }
          avro::ValidSchema writer_schema;
          std::stringstream in;
          in << schema.value().definition();
          avro::compileJsonSchema(in, writer_schema);
          revisions_to_schemas[schema_revision_id] = writer_schema;
        }
        auto writer_schema = revisions_to_schemas[schema_revision_id];

        auto encoding = message.attributes()["googclient_schemaencoding"];
        if (encoding == "JSON") {
          std::stringstream in;
          in << message.data();
          auto avro_in = avro::istreamInputStream(in);
          avro::DecoderPtr decoder = avro::resolvingDecoder(
              writer_schema, reader_schema, avro::jsonDecoder(writer_schema));
          decoder->init(*avro_in);

          v2::State state;
          avro::decode(*decoder, state);
          std::cout << "Name: " << state.name << "\n";
          std::cout << "Postal Abbreviation: " << state.post_abbr << "\n";
          std::cout << "Population: " << state.population << "\n";
        } else {
          std::cout << "Unable to decode. Received message using encoding"
                    << encoding << "\n";
        }
        std::move(h).ack();
      });
  // [END pubsub_subscribe_avro_records_with_revisions]

  std::cout << "Waiting for messages on " + subscription_id + "...\n";

  // Blocks until the timeout is reached.
  auto result = session.wait_for(kWaitTimeout);
  if (result == std::future_status::timeout) {
    std::cout << "timeout reached, ending session\n";
    session.cancel();
  }

  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
} catch (const std::exception& error) {
  std::cout << error.what() << std::endl;
}
