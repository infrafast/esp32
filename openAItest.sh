#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./openAItest.sh x32
#   ./openAItest.sh xairosc
# Optional env overrides:
#   HOST=192.168.0.16 PORT=10024 CMD=/xremote ./openAItest.sh xairosc

CONSOLE_TYPE="${1:-xairosc}"
HOST="${HOST:-192.168.0.16}"
PORT="${PORT:-10024}"
CMD="${CMD:-/xremote}"
TIMEOUT="${TIMEOUT:-1500}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

exec node "$SCRIPT_DIR/openai-remote.ts" \
  --host "$HOST" \
  --port "$PORT" \
  --protocol "$CONSOLE_TYPE" \
  --command "$CMD" \
  --timeout "$TIMEOUT"
