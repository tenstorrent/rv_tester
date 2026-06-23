#!/usr/bin/env bash
# Thin wrapper — first arg is the sim binary, rest are plusargs to forward.
set -eu
exec "$@"
