# Multi-stream detection pipelines

Three ways to run YOLO26 object detection over many RTSP streams on a Modalix
DevKit, with video and detection metadata delivered to Neat Insight. A chooser
page on `:8080` starts one and stops the others — they share one MLA and one set
of Insight channels, so only one may hold the hardware at a time.

| Pipeline | Topology | Adding a stream | Use it for |
| --- | --- | --- | --- |
| **scale** `:8090` | one fused graph, one shared detector, encoded passthrough | restarts everything (~10–30 s) | highest stream counts |
| **live** `:8091` | one graph per stream, re-encode | builds live, others keep running | adding/removing streams without downtime |
| **group** `:8092` | the scale app run as several independent processes | restarts one group | isolating failures to a subset |

Both topologies ship inside this repository, so the bundle is self-contained.
There is one entry point, [`src/python/main.py`](../src/python/main.py), and the
pipeline picks the topology: `live` runs it with `--mode adaptive` (one graph per
stream), while `scale` and `group` run it with `--mode fused` (one fused graph
with a shared detector).

## Setup on a new machine

Everything is path-relative, so clone anywhere. Two addresses have to be set:
this SDK container's IP (as the board and your browser see it) and your DevKit's.

```bash
cd <clone>/pipelines
./setup-adaptive-pipeline.sh <host-ip> <board-ip>      # e.g. ./setup-adaptive-pipeline.sh 192.0.2.10 192.0.2.72
```

It rewrites both addresses across the bundle, fixes the container's
`CONTAINER_HOST_IP` (what Insight advertises for WebRTC), repairs the DevKit's
NFS fstab and watchdog, restarts Insight, and brings up the chooser and the
three panels. Safe to re-run; it discovers the current addresses itself. Pass
only `<host-ip>` when just this machine moved.

Then open **`http://<board-ip>:8080/`**.

### Prerequisites

- The DevKit NFS-mounts this clone at the same absolute path the container sees.
  Clone inside the exported tree, or set `DEVKIT_ROOT=`.
- `neat update` has been run on the DevKit (`/home/sima/pyneat/bin/python`).
- Key-based SSH from the container to the DevKit.
- **The YOLO26n model pack.** All three pipelines use the same build. Fetch it
  into `models/` at the repository root (gitignored, so it is never committed):

  ```bash
  mkdir -p models
  curl -L -o models/yolo26n-det-int8-b1.tar.gz \
    https://github.com/SiMa-ai/apps-adaptive-object-detection/releases/download/v1.0.0/yolo26n-det-int8-b1.tar.gz
  ```
- **Insight media, named by prefix.** The tier table picks sources by filename:
  `2160p_*` for 4K, `1080p_*` for 1080p, `video*` for 720p. With an empty or
  differently-named library every `up` fails with `no '<prefix>*' media in
  Insight`. Upload clips through the panel or
  `curl -sk -F file=@clip.mp4 https://<host-ip>:9900/api/upload/media`.
  Use sources at ~25–30 fps; a very high-rate clip (150 fps) starts and streams
  video but yields no detections.

## CLI

The panels and the CLI are the same code.

```bash
cd pipeline-scale          # or pipeline-live / pipeline-group
python3 pipeline.py up 5   # start with 5 streams
python3 pipeline.py add    # add one
python3 pipeline.py status # per-channel bitrate and detection fps
python3 pipeline.py down   # stop detector and Insight sources
```

Always stop with `down`. `SIGKILL` strands decoder and CVU pools, and the next
run can then fail to allocate at a count that worked before.

## Stream ceilings

`TIERS` in each `pipeline.py` holds counts measured on one DevKit with specific
clips — treat them as a starting point and re-measure on your own hardware.
`status` is the check that matters: every channel should hold a steady fps near
the source rate. A channel showing bitrate but `0.0` fps is delivering video
with no detections.

## Layout

```
pipelines/
  setup-adaptive-pipeline.sh   one-shot setup / re-point
  launcher.py .sh .html  the chooser on :8080
  web-assets/            fonts and logo shared by all four pages
  pipeline-{scale,live,group}/
      pipeline.py        control logic + CLI
      ui_server.py       the panel's HTTP server
      ui.sh              start/stop that panel
      web/index.html
```

Generated at runtime and gitignored: `*-run.yaml` (rewritten on every `up`),
`ui-state.json`, `logs/`.
