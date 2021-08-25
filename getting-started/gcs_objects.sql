-- Copyright 2021 Google LLC
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.


-- Contains metadata for GCS objects
--
-- The field names are chosen to match the JSON API field names.
CREATE TABLE gcs_objects (
    name STRING(1024) NOT NULL,
    -- The actual limit is 222 characters, but 256 is easier to remember.
    bucket STRING(256) NOT NULL,
    generation INT64 NOT NULL,
    metageneration INT64 NOT NULL,
    -- TODO(#138) - consider adding these fields, they are redundant,
    --   but maybe are convenient too.
    -- id STRING(2048),
    -- selfLink STRING(2048),
    -- mediaLink STRING(2048),
    timeCreated TIMESTAMP,
    updated TIMESTAMP,
    timeDeleted TIMESTAMP,
    customTime TIMESTAMP,
    temporaryHold BOOL,
    eventBasedHold BOOL,
    retentionExpirationTime TIMESTAMP,
    storageClass STRING(64),
    timeStorageClassUpdated TIMESTAMP,
    size INT64,
    md5Hash STRING(32),
    crc32c STRING(32),

    -- The choice of 256 for the column size is arbitrary.
    contentType STRING(256),
    contentEncoding STRING(256),
    contentDisposition STRING(256),
    contentLanguage STRING(256),
    cacheControl STRING(256),
    metadata JSON,
    -- TODO(#138) - consider adding this field, hard to use
    --     acl JSON,
    owner JSON,
    componentCount INT64,
    etag STRING(32),
    customerEncryption JSON,
    kmsKeyName STRING(256),
) PRIMARY KEY (bucket, name, generation)
