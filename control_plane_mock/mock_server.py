#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import logging
import threading
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any
from urllib.parse import parse_qs, urlparse


LOGGER = logging.getLogger("control_plane_mock")


@dataclass
class AgentRecord:
    public_key: str
    endpoint: str
    hostname: str
    vpn_ip: str
    registered_at: str
    last_seen: str
    last_seen_epoch: float = field(default_factory=time.time)


class PeerStore:
    def __init__(self, ttl_seconds: int, vpn_cidr_prefix: str) -> None:
        self._ttl_seconds = ttl_seconds
        self._vpn_cidr_prefix = vpn_cidr_prefix
        self._agents_by_key: dict[str, AgentRecord] = {}
        self._vpn_by_key: dict[str, str] = {}
        self._next_host_octet = 2
        self._lock = threading.Lock()

    def register(self, public_key: str, endpoint: str, hostname: str) -> dict[str, Any]:
        with self._lock:
            self._evict_stale_locked()
            now_iso = _utc_now_iso()
            now_epoch = time.time()

            if public_key in self._agents_by_key:
                record = self._agents_by_key[public_key]
                record.endpoint = endpoint
                record.hostname = hostname
                record.last_seen = now_iso
                record.last_seen_epoch = now_epoch
            else:
                vpn_ip = self._vpn_by_key.get(public_key)
                if vpn_ip is None:
                    vpn_ip = f"{self._vpn_cidr_prefix}.{self._next_host_octet}"
                    self._next_host_octet += 1
                    self._vpn_by_key[public_key] = vpn_ip
                record = AgentRecord(
                    public_key=public_key,
                    endpoint=endpoint,
                    hostname=hostname,
                    vpn_ip=vpn_ip,
                    registered_at=now_iso,
                    last_seen=now_iso,
                    last_seen_epoch=now_epoch,
                )
                self._agents_by_key[public_key] = record

            LOGGER.info(
                "register: key=%s hostname=%s endpoint=%s total_agents=%d",
                public_key,
                hostname,
                endpoint,
                len(self._agents_by_key),
            )

            return self._build_peer_response_locked(public_key, include_hash=True)

    def ping(self, caller_public_key: str) -> dict[str, Any] | None:
        with self._lock:
            self._evict_stale_locked()
            record = self._agents_by_key.get(caller_public_key)
            if record is None:
                LOGGER.warning("ping: unknown caller key=%s",
                               caller_public_key)
                return None
            record.last_seen = _utc_now_iso()
            record.last_seen_epoch = time.time()
            LOGGER.info(
                "ping: key=%s hostname=%s total_agents=%d",
                caller_public_key,
                record.hostname,
                len(self._agents_by_key),
            )
            return self._build_peer_response_locked(caller_public_key, include_hash=True)

    def touch(self, caller_public_key: str) -> None:
        """Update last_seen_epoch for a registered agent (e.g. on 304 heartbeats)."""
        with self._lock:
            record = self._agents_by_key.get(caller_public_key)
            if record is not None:
                record.last_seen = _utc_now_iso()
                record.last_seen_epoch = time.time()

    def compare_hash(self, caller_public_key: str) -> str | None:
        with self._lock:
            self._evict_stale_locked()
            if caller_public_key not in self._agents_by_key:
                LOGGER.warning(
                    "compare_hash: unknown caller key=%s", caller_public_key)
                return None
            response = self._build_peer_response_locked(
                caller_public_key, include_hash=True)
            LOGGER.debug("compare_hash: key=%s hash=%s",
                         caller_public_key, response["hash"])
            return response["hash"]

    def _evict_stale_locked(self) -> None:
        now = time.time()
        stale_keys = [
            public_key
            for public_key, record in self._agents_by_key.items()
            if (now - record.last_seen_epoch) > self._ttl_seconds
        ]
        for public_key in stale_keys:
            self._agents_by_key.pop(public_key, None)
            LOGGER.info("evict: key=%s reason=ttl_expired", public_key)

    def _build_peer_response_locked(self, caller_public_key: str, include_hash: bool) -> dict[str, Any]:
        self_record = self._agents_by_key[caller_public_key]
        peers = [
            {
                "publicKey": record.public_key,
                "endpoint": record.endpoint,
                "allowedVpnIps": [f"{record.vpn_ip}/32"],
                "registeredAt": record.registered_at,
                "lastSeen": record.last_seen,
                "hostname": record.hostname,
            }
            for public_key, record in sorted(self._agents_by_key.items())
            if public_key != caller_public_key
        ]

        result = {
            "self": {
                "publicKey": self_record.public_key,
                "endpoint": self_record.endpoint,
                "VpnIp": self_record.vpn_ip,
                "registeredAt": self_record.registered_at,
                "lastSeen": self_record.last_seen,
                "hostname": self_record.hostname,
            },
            "peers": peers,
            "ttl": self._ttl_seconds,
        }

        if include_hash:
            topology = {
                "self": {
                    "publicKey": self_record.public_key,
                    "endpoint": self_record.endpoint,
                    "VpnIp": self_record.vpn_ip,
                    "hostname": self_record.hostname,
                },
                "peers": [
                    {
                        "publicKey": peer["publicKey"],
                        "endpoint": peer["endpoint"],
                        "allowedVpnIps": peer["allowedVpnIps"],
                        "hostname": peer["hostname"],
                    }
                    for peer in peers
                ],
            }
            result["hash"] = hashlib.sha256(
                json.dumps(topology, sort_keys=True,
                           separators=(",", ":")).encode("utf-8")
            ).hexdigest()

        return result


def _utc_now_iso() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def _normalize_route(path: str) -> str:
    prefixes = ("/api/v1", "/_mock/openapi")
    for prefix in prefixes:
        if path == prefix:
            return "/"
        if path.startswith(prefix + "/"):
            return path[len(prefix):]
    return path


def _json_response(handler: BaseHTTPRequestHandler, status: HTTPStatus, payload: dict[str, Any]) -> None:
    body = json.dumps(payload).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json")
    handler.send_header("Content-Length", str(len(body)))
    handler.end_headers()
    handler.wfile.write(body)


def _error_response(handler: BaseHTTPRequestHandler, status: HTTPStatus, message: str) -> None:
    _json_response(handler, status, {"error": message})


def make_handler(store: PeerStore):
    class MockHandler(BaseHTTPRequestHandler):
        def do_POST(self) -> None:  # noqa: N802
            route = _normalize_route(urlparse(self.path).path)
            if route != "/register":
                _error_response(self, HTTPStatus.NOT_FOUND, "Not found")
                return

            content_length = int(self.headers.get("Content-Length", "0"))
            body = self.rfile.read(content_length)

            try:
                payload = json.loads(body.decode("utf-8"))
            except json.JSONDecodeError:
                _error_response(self, HTTPStatus.BAD_REQUEST,
                                "Invalid JSON payload")
                return

            public_key = str(payload.get("publicKey", "")).strip()
            endpoint = str(payload.get("endpoint", "")).strip()
            hostname = str(payload.get("hostname", "")).strip()

            if not public_key or not endpoint or not hostname:
                _error_response(self, HTTPStatus.BAD_REQUEST,
                                "Fields publicKey, endpoint, hostname are required")
                return

            LOGGER.info(
                "http POST /register from=%s key=%s hostname=%s endpoint=%s",
                self.client_address[0],
                public_key,
                hostname,
                endpoint,
            )
            response = store.register(public_key, endpoint, hostname)
            LOGGER.info(
                "http POST /register -> 200 key=%s peers_returned=%d hash=%s",
                public_key,
                len(response.get("peers", [])),
                response.get("hash", ""),
            )
            _json_response(self, HTTPStatus.OK, response)

        def do_GET(self) -> None:  # noqa: N802
            parsed = urlparse(self.path)
            route = _normalize_route(parsed.path)
            if route != "/ping":
                _error_response(self, HTTPStatus.NOT_FOUND, "Not found")
                return

            caller_public_key = self.headers.get("X-Agent-PubKey", "").strip()
            if not caller_public_key:
                query = parse_qs(parsed.query)
                caller_public_key = (
                    query.get("publicKey", [""])[0] or "").strip()

            if not caller_public_key:
                _error_response(self, HTTPStatus.BAD_REQUEST,
                                "Missing X-Agent-PubKey header or publicKey query param")
                return

            current_hash = self.headers.get("X-Peers-Hash", "").strip()
            LOGGER.info(
                "http GET /ping from=%s key=%s client_hash=%s",
                self.client_address[0],
                caller_public_key,
                current_hash,
            )
            server_hash = store.compare_hash(caller_public_key)

            if server_hash is None:
                _error_response(self, HTTPStatus.NOT_FOUND,
                                "Agent is not registered")
                return

            if current_hash and current_hash == server_hash:
                LOGGER.info(
                    "http GET /ping -> 304 key=%s hash_match=true", caller_public_key)
                store.touch(caller_public_key)
                self.send_response(HTTPStatus.NOT_MODIFIED)
                self.end_headers()
                return

            response = store.ping(caller_public_key)
            if response is None:
                _error_response(self, HTTPStatus.NOT_FOUND,
                                "Agent is not registered")
                return
            LOGGER.info(
                "http GET /ping -> 200 key=%s peers_returned=%d hash=%s",
                caller_public_key,
                len(response.get("peers", [])),
                response.get("hash", ""),
            )
            _json_response(self, HTTPStatus.OK, response)

        def log_message(self, format: str, *args: Any) -> None:  # noqa: A003
            return

    return MockHandler


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Stateful Control Plane mock server for EdgeBox VPN agent")
    parser.add_argument("--host", default="127.0.0.1", help="Bind host")
    parser.add_argument("--port", type=int, default=4000, help="Bind port")
    parser.add_argument("--ttl", type=int, default=60,
                        help="TTL for active peers in seconds")
    parser.add_argument("--vpn-prefix", default="10.200.0",
                        help="VPN IPv4 prefix x.y.z")
    parser.add_argument("--log-level", default="INFO",
                        help="Logging level: DEBUG, INFO, WARNING, ERROR")
    args = parser.parse_args()

    logging.basicConfig(
        level=getattr(logging, args.log_level.upper(), logging.INFO),
        format="%(asctime)s %(levelname)s [%(name)s] %(message)s",
    )

    store = PeerStore(ttl_seconds=args.ttl, vpn_cidr_prefix=args.vpn_prefix)
    server = ThreadingHTTPServer((args.host, args.port), make_handler(store))
    print(f"Mock control plane listening on http://{args.host}:{args.port}")
    print("Supported base paths: /api/v1 and /_mock/openapi")
    print("Use X-Agent-PubKey on GET /ping to identify the caller")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
