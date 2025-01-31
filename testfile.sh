#!/bin/bash

let "i=0"

var="abcdefghijklmnopqrstuvwxyz"
len=${#var}

if [ "$1" = "" ]; then
  >&2 echo "Usage: ./testfile.sh <number of bytes>"
  exit 1
fi

chars=$1

while [ "$chars" -gt 0 ]; do
  i=$((i + 1))
  printf "%d " "$i"

  for x in $(seq 0 $(($len - 1))); do
    char="${var:$x:1}"
    echo -n "$char"
    chars=$((chars - 1))
    if [ "$chars" -lt 1 ]; then
      break
    fi
  done
  echo  # Print a newline after each iteration
done