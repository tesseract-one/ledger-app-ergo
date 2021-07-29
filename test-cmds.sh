#!/bin/bash

COMMAND=""

case "$1" in
  "name")
    COMMAND="e002000000"
    ;;
  "version")
    COMMAND="e001000000"
    ;;
  "extpubkey")
    COMMAND="e0100100""0D""038000002C800001AD80000000"
    ;;
  "showaddr")
    COMMAND="e0110201""16""00058000002C800001AD800000000000000000000000"
    ;;
  "getaddr")
    COMMAND="e0110101""16""00058000002C800001AD800000000000000000000000"
    ;;
  *)
    echo "Unknown command"
    exit 1
    ;;
esac

echo "$COMMAND" | python3 -m ledgerblue.runScript --apdu