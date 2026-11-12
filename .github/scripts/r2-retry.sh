#!/usr/bin/env bash
# Runs `aws <args>` against R2 and retries transient failures (InvalidPart on a multipart
# upload, first-byte timeouts). Usage: r2-retry.sh s3 cp|sync <src> <dst> --endpoint-url <url> ...
# Attempts and backoff: R2_RETRY_ATTEMPTS (default 5), R2_RETRY_BACKOFF seconds (default 10, linear).
set -uo pipefail

attempts="${R2_RETRY_ATTEMPTS:-5}"
backoff="${R2_RETRY_BACKOFF:-10}"

# Fewer, larger parts and low concurrency: R2 rejects a multipart completion when a part
# upload was retried or raced, and the multi-GB iOS templates have hundreds of parts at the 8 MB default.
aws configure set default.s3.max_concurrent_requests 4
aws configure set default.s3.multipart_chunksize 64MB
aws configure set default.s3.multipart_threshold 128MB
aws configure set default.retry_mode adaptive
aws configure set default.max_attempts 5

for ((attempt = 1; attempt <= attempts; attempt++)); do
  aws "$@" && exit 0
  rc=$?
  if ((attempt < attempts)); then
    echo "::warning::aws $1 $2 failed (exit ${rc}), attempt ${attempt}/${attempts}; retrying in $((attempt * backoff))s"
    sleep $((attempt * backoff))
  fi
done

echo "::error::aws $1 $2 failed after ${attempts} attempts"
exit "${rc}"
