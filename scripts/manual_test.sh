#!/usr/bin/env bash

HOST=${HOST:-127.0.0.1}
PORT=${PORT:-8080}
BASE="http://${HOST}:${PORT}"

echo "========== Test 1: GET / =========="
curl -v "${BASE}/"
echo
echo

echo "========== Test 2: GET /index.html =========="
curl -v "${BASE}/index.html"
echo
echo

echo "========== Test 3: GET /not-found.html =========="
curl -v "${BASE}/not-found.html"
echo
echo

echo "========== Test 4: POST / =========="
curl -X POST -v "${BASE}/"
echo
echo

echo "========== Test 5: Bad request =========="
printf 'BAD\r\n\r\n' | nc -w 2 "${HOST}" "${PORT}"
echo
echo

echo "========== Manual tests finished =========="