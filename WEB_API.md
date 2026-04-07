# Cypress Web API

This project includes a built-in HTTP control API for dedicated server builds.

## Enable API

Add launch arguments to your server startup command:

- `-enableApi` enable API service (or provide `-apiPort` directly)
- `-apiBind 127.0.0.1` bind address, default is `127.0.0.1`
- `-apiPort 8787` listen port, default is `8787`
- `-apiToken <token>` optional Bearer token for authentication

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

### `GET /v1/messages?since=<id>&limit=<n>`

Returns recent server log messages from in-memory buffer.

- `since`: exclusive log id (default `0`)
- `limit`: max entries (1..500, default `100`)

Each message contains:

- `id`
- `text`

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

## Curl Examples

```bash
curl http://127.0.0.1:8787/health
curl "http://127.0.0.1:8787/v1/messages?since=0&limit=50"
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
