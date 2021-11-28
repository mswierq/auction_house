#!/bin/bash
PORT=10000
if [[ $# -gt 0 ]]; then
  PORT=$1
fi
docker build . -t auction_house
echo "Starts server at port: ${PORT}"
docker run --rm -p127.0.0.1:${PORT}:10000 --name auction_house_server auction_house:latest
