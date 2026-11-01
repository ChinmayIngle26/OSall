#!/bin/bash
# Problem 23: Build, run and monitor the containerised AI/ML application.
# Requirements: Docker installed and running (user in the 'docker' group or use sudo), curl.
# Usage: bash run_and_monitor.sh

IMAGE=aiml-iris:1.0
NAME=aiml-iris
PORT=5000

command -v docker >/dev/null 2>&1 || { echo "Docker is not installed. Install it first (e.g. sudo apt install docker.io)."; exit 1; }

echo "===== 1. Build the image ====="
docker build -t "$IMAGE" . || exit 1
docker images "$IMAGE"

echo
echo "===== 2. Run the container with resource limits (1 CPU, 512 MB RAM) ====="
docker rm -f "$NAME" >/dev/null 2>&1
START=$(date +%s.%N)
docker run -d --name "$NAME" --cpus=1 --memory=512m -p $PORT:5000 "$IMAGE" || exit 1
until curl -s "http://localhost:$PORT/health" >/dev/null 2>&1; do sleep 0.5; done
END=$(date +%s.%N)
echo "Container ready in $(echo "$END - $START" | bc 2>/dev/null || echo '?') seconds"
docker ps --filter "name=$NAME"

echo
echo "===== 3. Test the ML application ====="
curl -s "http://localhost:$PORT/"; echo
curl -s -X POST -H "Content-Type: application/json" \
     -d '{"features": [5.1, 3.5, 1.4, 0.2]}' "http://localhost:$PORT/predict"; echo
curl -s -X POST -H "Content-Type: application/json" \
     -d '{"features": [6.7, 3.0, 5.2, 2.3]}' "http://localhost:$PORT/predict"; echo

echo
echo "===== 4. Monitor with load (CPU, memory, network, PIDs) ====="
echo "Generating load in the background ..."
( curl -s "http://localhost:$PORT/load?n=1500" >/dev/null ) &
sleep 2
echo "--- docker stats (CPU %, memory, network I/O, block I/O, PIDs) ---"
docker stats --no-stream "$NAME"

echo
echo "===== 5. Process utilization ====="
echo "--- processes inside the container (docker top) ---"
docker top "$NAME"
echo "--- same processes as seen from the host (ps) ---"
PIDS=$(docker top "$NAME" -eo pid 2>/dev/null | tail -n +2 | tr -d ' ' | paste -sd, -)
[ -n "$PIDS" ] && ps -o pid,ppid,user,%cpu,%mem,rss,vsz,comm -p "$PIDS"
echo "--- inside the container (/proc view, PID namespace) ---"
docker exec "$NAME" sh -c 'ps aux 2>/dev/null || ls /proc | grep -E "^[0-9]+$"'

echo
echo "===== 6. Memory and CPU limits (cgroups) ====="
docker exec "$NAME" sh -c 'cat /sys/fs/cgroup/memory.max 2>/dev/null || cat /sys/fs/cgroup/memory/memory.limit_in_bytes; cat /sys/fs/cgroup/cpu.max 2>/dev/null'
docker exec "$NAME" sh -c 'cat /sys/fs/cgroup/memory.current 2>/dev/null || cat /sys/fs/cgroup/memory/memory.usage_in_bytes'

echo
echo "===== 7. Storage utilization ====="
docker system df
echo "--- image size ---";            docker image inspect "$IMAGE" --format '{{.Size}} bytes'
echo "--- writable layer of the container (SIZE column) ---"; docker ps -s --filter "name=$NAME"
echo "--- disk usage inside the container ---"; docker exec "$NAME" df -h /

echo
echo "===== 8. Host view (Linux tools) ====="
free -h
echo; uptime
echo; top -b -n 1 | head -12

echo
echo "===== 9. Clean up ====="
read -rp "Stop and remove the container now? [y/N] " ans
if [[ $ans =~ ^[Yy]$ ]]; then docker rm -f "$NAME"; fi
