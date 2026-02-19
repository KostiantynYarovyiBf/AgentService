# Control Plane Mock

This folder now has two things:

1. **OpenAPI contract docs** in `api/openapi.yaml`
2. **Stateful Python mock server** in `mock_server.py` (for integration tests)

## 1) Run contract docs preview (optional)

```sh
cd control_plane_mock/
npx @redocly/cli preview -d api
```

## 2) Run stateful Python mock (for agent testing)

```sh
cd control_plane_mock/
python3 mock_server.py --host 127.0.0.1 --port 4000 --ttl 60
```

The server supports both API base paths:

- `/api/v1`
- `/_mock/openapi`

So your current client default (`http://localhost:4000/_mock/openapi`) continues to work.

## Behavior

- `POST /register`: stores or updates agent by `publicKey`, assigns VPN IP, returns `self + peers + ttl + hash`
- `GET /ping`: returns topology for caller (`self + peers + ttl + hash`)
- `GET /ping` with matching `X-Peers-Hash`: returns `304`
- stale agents are removed automatically after `ttl`

For `GET /ping`, caller identity is required via:

- `X-Agent-PubKey: <agent_public_key>` (preferred)
- or query parameter `?publicKey=<agent_public_key>`

## Quick curl examples

Register agent A:

```sh
curl -s -X POST http://127.0.0.1:4000/_mock/openapi/register \
	-H 'Content-Type: application/json' \
	-d '{"publicKey":"PUB_A","endpoint":"192.168.1.10:51820","hostname":"agent-a"}' | jq
```

Register agent B:

```sh
curl -s -X POST http://127.0.0.1:4000/_mock/openapi/register \
	-H 'Content-Type: application/json' \
	-d '{"publicKey":"PUB_B","endpoint":"192.168.1.11:51821","hostname":"agent-b"}' | jq
```

Ping as agent A:

```sh
curl -i -s http://127.0.0.1:4000/_mock/openapi/ping \
	-H 'X-Agent-PubKey: PUB_A'
```
