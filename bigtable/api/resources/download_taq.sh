#!/bin/sh

set -e

# Found this file in https://github.com/caesar0301/awesome-public-datasets#finance
# if it gets removed or moved we may need to change this script:
curl ftp://ftp.nyxdata.com/Historical%20Data%20Samples/Daily%20TAQ%20Sample/EQY_US_ALL_NBBO_20161024.gz -o NBBO.txt.gz

gunzip NBBO.txt.gz

exit 0
