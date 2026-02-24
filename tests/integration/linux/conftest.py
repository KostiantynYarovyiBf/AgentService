from __future__ import annotations

from collections.abc import Generator

import pytest

from utils import (
    CLEANUP_SCRIPT,
    ENV_FILE,
    SETUP_SCRIPT,
    _has_non_interactive_sudo,
    _parse_env_file,
    _run,
)


@pytest.fixture(name="integration_env")
def fixture_integration_env() -> Generator[dict[str, str], None, None]:
    if not _has_non_interactive_sudo():
        pytest.skip(
            "Integration test requires root or passwordless sudo (sudo -n)")

    for cmd in (SETUP_SCRIPT, CLEANUP_SCRIPT):
        if not cmd.exists():
            pytest.fail(f"Missing required script: {cmd}")

    _run([str(CLEANUP_SCRIPT), "--quiet"], check=False)
    _run([str(SETUP_SCRIPT)])

    if not ENV_FILE.exists():
        pytest.fail(f"Expected env file not generated: {ENV_FILE}")

    env_data = _parse_env_file(ENV_FILE)
    try:
        yield env_data
    finally:
        _run([str(CLEANUP_SCRIPT), "--quiet"], check=False)
