#!/bin/bash

OUT_CONF_DIR=./out_confs

if [ -d "$OUT_CONF_DIR" ]; then
  echo "Removing existing directory: $OUT_CONF_DIR"
  rm -rf "$OUT_CONF_DIR"
else
  echo "Directory $OUT_CONF_DIR does not exist. Nothing to remove."
fi
