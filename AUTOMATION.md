# Candle Automation API

This fork exposes a small, local HTTP/JSON control plane for test stands and
robotic cells.  It is intended for an external orchestrator (including an AI
agent, a vision service, or a CI-style diagnostics script), not for exposing a
machine to the LAN.

## Discovery and instance identity

Each Candle process opens the first free loopback port in the range
`127.0.0.1:8090` through `127.0.0.1:8189`.  It never listens on Wi-Fi or LAN.
Find running instances like this:

```powershell
8090..8189 | ForEach-Object {
  try { Invoke-RestMethod "http://127.0.0.1:$_/api/v1/status" -TimeoutSec 1 } catch {}
}
```

Every status response contains a stable-for-this-run `instance_id`, the
Windows `pid`, executable path, process start time, Qt version and the API
address.  Select the desired process by `instance_id` or its configured serial
port / TCP host, rather than assuming that the first Candle window is right.

## GUI and console modes

`candle.exe` starts the ordinary visible operator UI.  The automation API
controls that same process, so opening a file, jogging, or starting a job is
immediately reflected in its Candle window.

`candle-cli.exe` is the Windows-console companion.  It uses the same controller
code and HTTP API but never shows the GUI, which makes it suitable for a test
cell service:

```powershell
./candle-cli.exe --automation-port 8091
```

For safety, `candle-cli.exe` does not take over a controller stored in its
settings profile until the caller sends `POST /api/v1/connect`. Pass `--connect`
only when that takeover is intended. The visible GUI can similarly be started
with `--no-connect`.

The GUI executable also accepts `--headless` when a background process is
desired.  Both forms bind only to `127.0.0.1`; `--automation-port` reserves a
specific port, otherwise Candle selects the first free port from 8090..8189.

## Observability first

`GET /healthz` says that the HTTP service is alive.

`GET /readyz` is successful only after Candle has connected and completed the
firmware reset/handshake.

`GET /api/v1/status` (also `/api/v1/events`) returns the active GRBL/Marlin
protocol and transport, connection configuration, readiness, sender/device
states, queue sizes, positions, selected G-code file, height-map state, last
error, and the latest 100 sent/received controller lines.  It is the primary
diagnostic endpoint.  It is deliberately suitable for collecting into logs or
a time-series system.

`POST /api/v1/test/connection` queues only safe inspection commands:

* Marlin: `M115`, `M114`, `M119`;
* GRBL: `M115`, `?`, `$#` (firmware identification is controller-dependent).

The replies appear in `recent_events`.  This is useful for separating a bad
COM/TCP link, a wrong protocol, a controller reset, and an endstop state before
attempting any motion.

## Safety gate

Read-only commands (`M105`, `M114`, `M115`, `M119`, `?`, `$$`, `$#`, `$G`) may
be issued without an arm.  All movement, homing, probing, spindle changes and
file execution need a short-lived explicit arm:

```powershell
$base = 'http://127.0.0.1:8090/api/v1'
Invoke-RestMethod "$base/arm" -Method Post -ContentType application/json -Body '{"seconds":60}'
```

The interval is clamped to 5..300 seconds and can be cancelled with
`POST /api/v1/disarm`.  This prevents a stale local automation process from
moving a machine merely because Candle remains open.  It is a guardrail, not a
functional-safety system: physical E-stop, correctly configured endstops,
machine bounds, workholding and operator clearance remain mandatory.

## Connection profiles

`POST /api/v1/connect` applies a connection profile at runtime.  Poll
`/readyz` after it returns.  Examples:

```powershell
# Lotmaxx Wi-Fi bridge
Invoke-RestMethod "$base/connect" -Method Post -ContentType application/json -Body '{
  "transport":"telnet", "protocol":"marlin", "host":"192.168.1.38", "tcp_port":8888
}'

# Conventional GRBL controller
Invoke-RestMethod "$base/connect" -Method Post -ContentType application/json -Body '{
  "transport":"serial", "protocol":"grbl", "port":"COM7", "baud":115200
}'
```

Supported transports are `serial`, `telnet`, and `websocket`; protocols are
`marlin` and `grbl`.  `POST /api/v1/disconnect` closes the current connection.

## Operations

All request bodies are JSON.  Mutating endpoints below require an active arm
unless marked otherwise.

| Endpoint | Body | Action |
| --- | --- | --- |
| `POST /command` | `{"gcode":"M119"}` | Send one line; read-only queries do not need arm. |
| `POST /jog` | `{"x":1,"y":0,"z":0,"feed":300}` | Relative move. Each axis is limited to +/-10 mm, feed 1..10000 mm/min. |
| `POST /move` | `{"x":10,"y":20,"z":5,"feed":300}` | Absolute G-code move. Keep coordinates within verified machine limits. |
| `POST /home` | `{"axes":"XY"}` | Marlin sends `G28 XY`; GRBL sends `$H`. Valid axis forms: `X`, `Y`, `Z`, `XY`, `XYZ`. |
| `POST /spindle` | `{"enabled":true,"speed":1000}` or `{"enabled":false}` | Send `M3 S...` or `M5`. |
| `POST /file/open` | `{"path":"C:\\jobs\\part.gcode"}` | Load an existing G-code file; does not run it. |
| `POST /file/start` | `{}` | Start the loaded normal file. |
| `POST /file/abort` | `{}` | Abort the sender. |
| `POST /heightmap/configure` | geometry/grid/probe-feed fields | Set the Candle height-map grid. |
| `POST /heightmap/start` | `{}` | Start the configured probe program. |

For a first real-machine test, use `/test/connection`, inspect `M119` in
`recent_events`, then arm for only 30--60 seconds and use a one-millimetre jog
away from any endstop.  Never start an unknown production G-code file as the
first test.

## Vision and a CNC cell

The HTTP layer deliberately does not claim to be a camera or safety system.
Its role is a deterministic machine adapter.  A vision service should perform
calibration and coordinate transformation, verify a workpiece and clearance,
then call `/move`, `/jog`, `/file/open` and `/file/start` against the selected
`instance_id`.  Persist the following with every operation: instance ID,
firmware identity (`M115`), machine/work positions, G-code hash, camera
calibration revision, command/reply trace, and result.  That gives the CNC
line the auditability and diagnostics expected from service-based systems
without coupling image processing into the desktop GUI.
