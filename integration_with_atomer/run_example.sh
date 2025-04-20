#!/bin/bash

ATOMER_OUTPUTS_DIR=./example/atomer_outputs
BASE_CONFIG_DIR=./base_conf
RESULTS_CONFIG_DIR=./out_confs

echo "Running AnacondaConfFromAtomerResult.py with the following parameters:"
echo "  --atomer_outputs_dir: $ATOMER_OUTPUTS_DIR"
echo "  --base_config_dir:    $BASE_CONFIG_DIR"
echo "  --result_config_dir:  $RESULTS_CONFIG_DIR"
echo ""

python3 AnacondaConfFromAtomerResult.py --atomer_outputs_dir "$ATOMER_OUTPUTS_DIR" \
                                        --base_config_dir "$BASE_CONFIG_DIR" \
                                        --result_config_dir "$RESULTS_CONFIG_DIR"
