#!/bin/bash

set -e

if [ -e pack ]; then
  rm -r pack
fi

mkdir pack

./build/fermity_tools head response_template_head.txt pack/rth.t0
./build/fermity_tools html response_template_content.txt pack/rtc.t0

echo "Done."

