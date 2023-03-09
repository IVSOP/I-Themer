#!/bin/bash

pkill --exact "ithemer-daemon"

parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
#scriptDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")

cd "$parent_path"
../bin/ithemer-daemon $parent_path/../data
# wont work without full path?
# why not just pass it
