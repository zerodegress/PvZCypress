# Cypress Web API

This project includes a built-in HTTP control API for dedicated server builds.

## Enable API

Add launch arguments to your server startup command:

- `-enableApi` enable API service (or provide `-apiPort` directly)
- `-apiBind 127.0.0.1` bind address, default is `127.0.0.1`
- `-apiPort 8787` listen port, default is `8787`
- `-apiToken <token>` optional Bearer token for authentication
- `-banlistFilename <path>` optional ban list file path, default is `bans.json`

Example:

```bat
-enableApi -apiBind 127.0.0.1 -apiPort 8787 -apiToken changeme
```

## Authentication

If `-apiToken` is set, all endpoints require:

```http
Authorization: Bearer <token>
```

If no token is configured, authentication is not required.

## Endpoints

### `GET /health`

Simple health check.

Response:

```json
{ "ok": true }
```

### `GET /v1/status`

Returns server/API runtime summary.

Fields:

- `ok`
- `running`
- `queuedCommands`
- `latestLogId`
- `latestEventId`

### `GET /v1/messages?since=<id>&limit=<n>`

Returns recent server log messages from in-memory buffer.

- `since`: exclusive log id (default `0`)
- `limit`: max entries (1..500, default `100`)

Each message contains:

- `id`
- `text`

### `GET /v1/events?since=<id>&limit=<n>&type=<eventType>`

Returns structured server events captured from hook points and API command flow.

- `since`: exclusive event id (default `0`)
- `limit`: max entries (1..500, default `100`)
- `type`: optional exact event type filter

Each event contains:

- `id`
- `ts` (Unix epoch milliseconds)
- `type`
- `source`
- `payload` (JSON object)

Response shape:

```json
{
  "ok": true,
  "events": [
    {
      "id": 101,
      "ts": 1775620000123,
      "type": "player.joined",
      "source": "hook.fb_ServerPlayerManager_addPlayer",
      "payload": {
        "playerId": 4,
        "name": "Alice"
      }
    }
  ],
  "latestEventId": 120
}
```

### `POST /v1/command`

Queue a server command for execution on the server update thread.

Request body:

```json
{ "cmd": "Server.Say 'Hello from API' 5" }
```

Response:

```json
{ "ok": true, "queued": true, "cmd": "..." }
```

## Command Notes

- API commands reuse server console command handlers.
- Supported commands are the `Server.*` commands registered by game module.
- `Server.` prefix is optional when submitting command text.
- For string parameters with spaces, use single quotes, for example:
  - `Server.Say 'Match starting' 6`

## Event Types (Current)

- `command.console_submitted`: command submitted in dedicated server command box.
- `command.api_queued`: command received by Web API and queued.
- `command.api_executed`: queued API command executed on server update thread.
- `command.startup_requested`: startup command from `-startupCommand` captured.
- `command.executed`: any recognized server command execution.
- `command.unknown`: unrecognized command attempted.
- `level.load_requested`: level load message posted.
- `player.connect_attempt`: player connection/create-player message observed.
- `player.joined`: player added to server.
- `player.left`: player disconnected from server.

## ServerEvent Payload Reference

`command.console_submitted`
- `cmd` (string)

`command.api_queued`
- `cmd` (string)

`command.api_executed`
- `cmd` (string)
- `ok` (boolean)

`command.startup_requested`
- `cmd` (string)

`command.executed`
- `cmd` (string)

`command.unknown`
- `cmd` (string)

`level.load_requested`
- `level` (string)
- `gameMode` (string)
- `hostedMode` (string)
- `tod` (string)
- `fadeOut` (boolean)
- `forceReloadResources` (boolean)

`player.connect_attempt`
- `playerName` (string)
- `machineId` (string)
- `shouldDisconnect` (boolean)
- `disconnectReason` (number)
- `disconnectReasonText` (string)

`player.joined`
- `playerId` (number)
- `name` (string)

`player.left`
- `playerId` (number)
- `name` (string)
- `reasonCode` (number)
- `reasonName` (string)
- `reasonText` (string)

## Event Cursor Semantics

- `since` is **exclusive**.  
  Example: if last processed event is `id=120`, next request should use `since=120`.
- Use `latestEventId` from responses as your checkpoint.
- Recommended polling loop:
1. On startup, call `/v1/status` and store `latestEventId` (or use `0` to replay buffered history).
2. Poll `/v1/events?since=<checkpoint>&limit=100`.
3. Process events in ascending order of `id`.
4. Update checkpoint to response `latestEventId` (or the last event `id`).

## Curl Examples

```bash
curl http://127.0.0.1:8787/health
curl "http://127.0.0.1:8787/v1/messages?since=0&limit=50"
curl "http://127.0.0.1:8787/v1/events?since=0&limit=50"
curl -X POST http://127.0.0.1:8787/v1/command \
  -H "Content-Type: application/json" \
  -d '{"cmd":"Server.Say '\''Server online'\'' 6"}'
```

With token:

```bash
curl http://127.0.0.1:8787/v1/status \
  -H "Authorization: Bearer changeme"
```

## Scope and Limitations

- HTTP only (no TLS/HTTPS in-process).
- Intended for local/LAN control plane usage.
- Message history is in-memory and bounded.
