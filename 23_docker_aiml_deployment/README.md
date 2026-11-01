# Problem 23 - AI/ML application in a Docker container

Files: `app.py` (Flask + scikit-learn Iris classifier), `requirements.txt`, `Dockerfile`, `.dockerignore`,
`run_and_monitor.sh` (build, run, test and monitor).

## Run
```bash
cd 23_docker_aiml_deployment
bash run_and_monitor.sh          # automatic: build -> run -> test -> monitor
```
Manual commands:
```bash
docker build -t aiml-iris:1.0 .
docker run -d --name aiml-iris --cpus=1 --memory=512m -p 5000:5000 aiml-iris:1.0
curl -X POST -H "Content-Type: application/json" -d '{"features":[5.1,3.5,1.4,0.2]}' localhost:5000/predict
curl "localhost:5000/load?n=1500" &   # generate CPU load
docker stats --no-stream aiml-iris    # CPU, memory, network, block I/O, PIDs
docker top aiml-iris                  # processes in the container
docker exec aiml-iris df -h /         # storage
docker system df                      # images / containers / volumes disk usage
```
Host tools: `top`, `htop`, `free -h`, `vmstat 1`, `ps`, `df -h`, `pidstat`.

## Monitoring tools and what they show
| Resource | Tool | What to observe |
|---|---|---|
| CPU | `docker stats`, `top`, `ps -o %cpu` | CPU % rises during `/load`; capped at 100 % by `--cpus=1` |
| Memory | `docker stats`, `free -h`, cgroup `memory.current` | usage vs the 512 MB limit; exceeding it triggers the OOM killer for that container only |
| Storage | `docker system df`, `docker ps -s`, `df -h` | image layers are shared/read-only; container writable layer is small |
| Processes | `docker top`, `ps`, `docker exec ... ps` | the same process has a host PID and a different PID (1) inside its PID namespace |

## Benefits of container-based deployment (evaluation)
1. **Portability / reproducibility** - the image contains Python, libraries and code, so it runs the same on a laptop, server or cloud ("works on my machine" problem solved).
2. **Isolation** - namespaces (PID, network, mount) and cgroups isolate the app; dependency conflicts with other apps disappear.
3. **Resource control** - `--cpus` and `--memory` limit what the app can use, protecting the host and other containers.
4. **Light weight vs virtual machines** - containers share the host kernel, so they start in seconds and use far less memory/disk than a VM with a full guest OS.
5. **Fast deployment and scaling** - start many replicas from the same image; layer caching makes rebuilds quick; easy rollback by image tag.
6. **Security** - runs as a non-root user with a minimal image (smaller attack surface).
7. **Trade-offs** - shares the host kernel (weaker isolation than a VM), container storage is ephemeral (use volumes for data), and ML images can be large (scikit-learn/numpy).
