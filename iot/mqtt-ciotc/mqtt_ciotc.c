/******************************************************************************
 * Copyright 2017 Google
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
// [START iot_mqtt_include]
#define _XOPEN_SOURCE 500  // NOLINT
#include <MQTTClient.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <jwt.h>
// [END iot_mqtt_include]

#define TRACE 1 /* Set to 1 to enable tracing */

enum { kClientidMaxlen = 256, kClientidSize };
enum { kTopicMaxlen = 256, kTopicSize };

struct {
  char* address;
  char clientid[kClientidSize];
  char* deviceid;
  char* keypath;
  char* projectid;
  char* region;
  char* registryid;
  char* rootpath;
  char topic[kTopicSize];
  char* payload;
  char* algorithm;
} opts = {.address = "ssl://mqtt.googleapis.com:8883",
          .clientid =
              "projects/{your-project-id}/locations/{your-region-id}/"
              "registries/{your-registry-id}/devices/{your-device-id}",
          .deviceid = "{your-device-id}",
          .keypath = "ec_private.pem",
          .projectid = "{your-project-id}",
          .region = "{your-region-id}",
          .registryid = "{your-registry-id}",
          .rootpath = "roots.pem",
          .topic = "/devices/{your-device-id}/events",
          .payload = "Hello world!",
          .algorithm = "ES256"};

void Usage() {
  printf("mqtt_ciotc <message> \\\n");
  printf("\t--deviceid <your device id>\\\n");
  printf("\t--region <e.g. us-central1>\\\n");
  printf("\t--registryid <your registry id>\\\n");
  printf("\t--projectid <your project id>\\\n");
  printf("\t--keypath <e.g. ./ec_private.pem>\\\n");
  printf("\t--rootpath <e.g. ./roots.pem>\n\n");
}

// [START iot_mqtt_jwt]
/**
 * Calculates issued at / expiration times for JWT and places the time, as a
 * Unix timestamp, in the strings passed to the function. The time_size
 * parameter specifies the length of the string allocated for both iat and exp.
 */
static void GetIatExp(char* iat, char* exp, int time_size) {
  // TODO(#72): Use time.google.com for iat
  time_t now_seconds = time(NULL);
  snprintf(iat, time_size, "%lu", now_seconds);
  snprintf(exp, time_size, "%lu", now_seconds + 3600);
  if (TRACE) {
    printf("IAT: %s\n", iat);
    printf("EXP: %s\n", exp);
  }
}

static int GetAlgorithmFromString(const char* algorithm) {
  if (strcmp(algorithm, "RS256") == 0) {
    return JWT_ALG_RS256;
  }
  if (strcmp(algorithm, "ES256") == 0) {
    return JWT_ALG_ES256;
  }
  return -1;
}

/**
 * Calculates a JSON Web Token (JWT) given the path to a EC private key and
 * Google Cloud project ID. Returns the JWT as a string that the caller must
 * free.
 */
static char* CreateJwt(const char* ec_private_path, const char* project_id,
                       const char* algorithm) {
  char iat_time[sizeof(time_t) * 3 + 2];
  char exp_time[sizeof(time_t) * 3 + 2];
  uint8_t* key = NULL;  // Stores the Base64 encoded certificate
  size_t key_len = 0;
  jwt_t* jwt = NULL;
  int ret = 0;
  char* out = NULL;

  // Read private key from file
  FILE* fp = fopen(ec_private_path, "r");
  if (fp == NULL) {
    printf("Could not open file: %s\n", ec_private_path);
    return "";
  }
  fseek(fp, 0L, SEEK_END);
  key_len = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  key = malloc(sizeof(uint8_t) * (key_len + 1));  // certificate length + \0

  fread(key, 1, key_len, fp);
  key[key_len] = '\0';
  fclose(fp);

  // Get JWT parts
  GetIatExp(iat_time, exp_time, sizeof(iat_time));

  jwt_new(&jwt);

  // Write JWT
  ret = jwt_add_grant(jwt, "iat", iat_time);
  if (ret) {
    printf("Error setting issue timestamp: %d\n", ret);
  }
  ret = jwt_add_grant(jwt, "exp", exp_time);
  if (ret) {
    printf("Error setting expiration: %d\n", ret);
  }
  ret = jwt_add_grant(jwt, "aud", project_id);
  if (ret) {
    printf("Error adding audience: %d\n", ret);
  }
  ret = jwt_set_alg(jwt, GetAlgorithmFromString(algorithm), key, key_len);
  if (ret) {
    printf("Error during set alg: %d\n", ret);
  }
  out = jwt_encode_str(jwt);
  if (!out) {
    perror("Error during token creation:");
  }
  // Print JWT
  if (TRACE) {
    printf("JWT: [%s]\n", out);
  }

  jwt_free(jwt);
  free(key);
  return out;
}
// [END iot_mqtt_jwt]

/**
 * Helper to parse arguments passed to app. Returns false if there are missing
 * or invalid arguments; otherwise, returns true indicating the caller should
 * free the calculated client ID placed on the opts structure.
 *
 * TODO: (class) Consider getopt
 */
// [START iot_mqtt_opts]
bool GetOpts(int argc, char** argv) {
  int pos = 1;
  bool calcvalues = false;

  if (argc < 2) {
    return false;
  }

  opts.payload = argv[1];

  while (pos < argc) {
    if (strcmp(argv[pos], "--deviceid") == 0) {
      if (++pos < argc) {
        opts.deviceid = argv[pos];
        calcvalues = true;
      } else {
        return false;
      }
    } else if (strcmp(argv[pos], "--region") == 0) {
      if (++pos < argc) {
        opts.region = argv[pos];
        calcvalues = true;
      } else {
        return false;
      }
    } else if (strcmp(argv[pos], "--registryid") == 0) {
      if (++pos < argc) {
        opts.registryid = argv[pos];
        calcvalues = true;
      } else {
        return false;
      }
    } else if (strcmp(argv[pos], "--projectid") == 0) {
      if (++pos < argc) {
        opts.projectid = argv[pos];
        calcvalues = true;
      } else {
        return false;
      }
    } else if (strcmp(argv[pos], "--keypath") == 0) {
      if (++pos < argc) {
        opts.keypath = argv[pos];
      } else {
        return false;
      }
    } else if (strcmp(argv[pos], "--rootpath") == 0) {
      if (++pos < argc) {
        opts.rootpath = argv[pos];
      } else {
        return false;
      }
    } else if (strcmp(argv[pos], "--algorithm") == 0) {
      if (++pos < argc) {
        opts.algorithm = argv[pos];
      } else {
        return false;
      }
    }
    pos++;
  }
  if (calcvalues) {
    int n =
        snprintf(opts.clientid, sizeof(opts.clientid),
                 "projects/%s/locations/%s/registries/%s/devices/%s",
                 opts.projectid, opts.region, opts.registryid, opts.deviceid);
    if (n < 0 || (n > kClientidMaxlen)) {
      if (n < 0) {
        printf("Encoding error for client ID\n");
      } else {
        printf("Error, buffer for storing client ID was too small.\n");
      }
      return false;
    }
    if (TRACE) {
      printf("New client id constructed:\n");
      printf("%s\n", opts.clientid);
    }
    // Construct topic from the given device id.
    int i = snprintf(opts.topic, sizeof(opts.topic), "/devices/%s/events",
                     opts.deviceid);
    if (i < 0 || (i > kTopicMaxlen)) {
      if (i < 0) {
        printf("Encoding error for topic!\n");
      } else {
        printf("Error, buffer for storing topic was too small.\n");
      }
      return false;
    }
    if (TRACE) {
      printf("Topic constructed:\n");
      printf("%s\n", opts.topic);
    }
    return true;  // Caller must free opts.clientid
  }
  return false;
}
// [END iot_mqtt_opts]

static const int kQos = 1;
static const unsigned long kTimeout = 10000L;
static const char* k_username = "unused";

static const unsigned long kInitialConnectIntervalMillis = 500L;
static const unsigned long kMaxConnectIntervalMillis = 6000L;
static const unsigned long kMaxConnectRetryTimeElapsedMillis = 900000L;
static const float kIntervalMultiplier = 1.5f;

/**
 * Publish a given message, passed in as payload, to Cloud IoT Core using the
 * values passed to the sample, stored in the global opts structure. Returns
 * the result code from the MQTT client.
 */
// [START iot_mqtt_publish]
int Publish(char* payload, int payload_size) {
  int rc = -1;
  MQTTClient client = {0};
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token = {0};

  MQTTClient_create(&client, opts.address, opts.clientid,
                    MQTTCLIENT_PERSISTENCE_NONE, NULL);
  conn_opts.keepAliveInterval = 60;
  conn_opts.cleansession = 1;
  conn_opts.username = k_username;
  conn_opts.password = CreateJwt(opts.keypath, opts.projectid, opts.algorithm);
  MQTTClient_SSLOptions sslopts = MQTTClient_SSLOptions_initializer;

  sslopts.trustStore = opts.rootpath;
  sslopts.privateKey = opts.keypath;
  conn_opts.ssl = &sslopts;

  unsigned long retry_interval_ms = kInitialConnectIntervalMillis;
  unsigned long total_retry_time_ms = 0;
  while ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
    if (rc == 3) {  // connection refused: server unavailable
      usleep(retry_interval_ms * 1000);
      total_retry_time_ms += retry_interval_ms;
      if (total_retry_time_ms >= kMaxConnectRetryTimeElapsedMillis) {
        printf("Failed to connect, maximum retry time exceeded.");
        exit(EXIT_FAILURE);
      }
      retry_interval_ms *= kIntervalMultiplier;
      if (retry_interval_ms > kMaxConnectIntervalMillis) {
        retry_interval_ms = kMaxConnectIntervalMillis;
      }
    } else {
      printf("Failed to connect, return code %d\n", rc);
      exit(EXIT_FAILURE);
    }
  }

  pubmsg.payload = payload;
  pubmsg.payloadlen = payload_size;
  pubmsg.qos = kQos;
  pubmsg.retained = 0;
  MQTTClient_publishMessage(client, opts.topic, &pubmsg, &token);
  printf(
      "Waiting for up to %lu seconds for publication of %s\n"
      "on topic %s for client with ClientID: %s\n",
      (kTimeout / 1000), opts.payload, opts.topic, opts.clientid);
  rc = MQTTClient_waitForCompletion(client, token, kTimeout);
  printf("Message with delivery token %d delivered\n", token);
  MQTTClient_disconnect(client, 10000);
  MQTTClient_destroy(&client);

  return rc;
}
// [END iot_mqtt_publish]

/**
 * Connects MQTT client and transmits payload.
 */
// [START iot_mqtt_run]
int main(int argc, char* argv[]) {
  OpenSSL_add_all_algorithms();
  OpenSSL_add_all_digests();
  OpenSSL_add_all_ciphers();

  if (GetOpts(argc, argv)) {
    Publish(opts.payload, strlen(opts.payload));
  } else {
    Usage();
  }

  EVP_cleanup();
}
// [END iot_mqtt_run]
