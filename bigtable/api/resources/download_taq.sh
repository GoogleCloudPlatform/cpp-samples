#!/bin/sh
# Copyright 2017, Google Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

set -e

# Found this file in https://github.com/caesar0301/awesome-public-datasets#finance
# if it gets removed or moved we may need to change this script:
curl ftp://ftp.nyxdata.com/Historical%20Data%20Samples/Daily%20TAQ%20Sample/EQY_US_ALL_NBBO_20161024.gz -o NBBO.txt.gz

gunzip NBBO.txt.gz

exit 0
