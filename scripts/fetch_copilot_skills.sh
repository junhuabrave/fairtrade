#!/usr/bin/env bash
set -euo pipefail

REPO_URL=${1:-}
TARGET_DIR="$(dirname "$0")/../tools/copilot-skills"

if [ -z "$REPO_URL" ]; then
  echo "Usage: $0 <git-repo-url>"
  exit 2
fi

mkdir -p "$TARGET_DIR"
if [ -n "$(ls -A "$TARGET_DIR")" ]; then
  echo "Target directory $TARGET_DIR is not empty. Aborting to avoid overwrite."
  exit 3
fi

echo "Cloning $REPO_URL into $TARGET_DIR"
git clone "$REPO_URL" "$TARGET_DIR"
echo "Done. Files are stored in $TARGET_DIR and are gitignored."
