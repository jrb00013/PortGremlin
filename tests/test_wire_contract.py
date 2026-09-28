"""Contract tests: the host tools and the firmware must agree on the wire.

These are the tests that were missing. The CLI shipped with an unterminated
dict literal, and the README plus the on-device help both advertised a
`0`-`9` mimic-deploy key while digits `1`-`5` were already class toggles.
Nothing in the repo compared the three surfaces, so all of it went unnoticed.
"""

from __future__ import annotations

from conftest import load_tool

import re
import subprocess
import sys
from pathlib import Path

import pytest

CASE_RE = re.compile(r"case\s+'(.+?)'\s*:")
HELP_KEY_RE = re.compile(r"([a-z0-9])\s*-\s*[A-Za-z]")
RANGE_KEY_RE = re.compile(r"([0-9])-([0-9])\s*-\s*[A-Za-z]")
README_RANGE_RE = re.compile(r"`([0-9])`[\u2013-]`([0-9])`")


def _uart_cases(firmware_dir: Path) -> set[str]:
    """Keys handled by the firmware UART dispatcher.

    Character literals may be escaped in C (``case '\\':`` is a single
    backslash), so drop the leading escape marker to recover the wire byte.
    """
    text = (firmware_dir / "portgremlin_uart.c").read_text()
    keys = set()
    for match in CASE_RE.finditer(text):
        raw = match.group(1)
        keys.add((raw[1:] if raw.startswith("\\") else raw).lower())
    return keys


def test_uart_source_parses_as_expected(firmware_dir: Path) -> None:
    """Sanity check on the parser the other tests rely on."""
    cases = _uart_cases(firmware_dir)
    # Core controls that have existed since the first commit.
    for key in ("h", "s", "b", "g", "x", "l", "v", "\\"):
        assert key in cases, f"expected UART key {key!r} in portgremlin_uart.c"
    # Free digits + next-mimic walk the vault; 1-5 remain class toggles.
    for key in ("0", "6", "7", "8", "9", "n"):
        assert key in cases, f"expected mimic UART key {key!r} in portgremlin_uart.c"


def test_cli_imports_cleanly() -> None:
    """The CLI used to ship with an unclosed dict literal and could not run."""
    portgremlin_cli = load_tool("portgremlin-cli")

    assert portgremlin_cli.COMMANDS, "COMMANDS map is empty"


def test_cli_command_values_are_single_chars() -> None:
    portgremlin_cli = load_tool("portgremlin-cli")

    for name, key in portgremlin_cli.COMMANDS.items():
        assert len(key) == 1, f"command {name!r} maps to {key!r}, not a single char"


def test_cli_has_no_duplicate_targets() -> None:
    """Two names may share a key, but only if the key is genuinely shared."""
    portgremlin_cli = load_tool("portgremlin-cli")

    seen: dict[str, str] = {}
    for name, key in portgremlin_cli.COMMANDS.items():
        assert key not in seen, (
            f"{name!r} and {seen[key]!r} both map to {key!r}; "
            "one of them is unreachable"
        )
        seen[key] = name


def test_every_cli_command_is_handled_by_firmware(firmware_dir: Path) -> None:
    """Every key the host CLI can send must be a key the firmware implements.

    This is the drift guard: adding a host command without the matching
    firmware case (or vice versa) fails here instead of silently doing nothing
    on the device.
    """
    portgremlin_cli = load_tool("portgremlin-cli")

    cases = _uart_cases(firmware_dir)
    unknown = {
        name: key
        for name, key in portgremlin_cli.COMMANDS.items()
        if key.lower() not in cases
    }
    assert not unknown, f"CLI sends keys the firmware does not handle: {unknown}"


def test_onboard_help_advertises_only_real_keys(firmware_dir: Path) -> None:
    """The on-device help must not advertise unimplemented keys.

    Digits 1-5 are class toggles; free digits 0 and 6-9 plus 'n' deploy
    mimic profiles. Help must not claim the whole 0-9 range is mimic.
    """
    text = (firmware_dir / "portgremlin_uart.c").read_text()
    cases = _uart_cases(firmware_dir)

    advertised: set[str] = set()
    for lo, hi in RANGE_KEY_RE.findall(text):
        for code in range(int(lo), int(hi) + 1):
            advertised.add(str(code))
    for key in HELP_KEY_RE.findall(text):
        advertised.add(key)
    # Normalise the double-character operators the help renders as "+/-".
    advertised = {a for a in advertised if a not in ("/", "*")}

    missing = sorted(k for k in advertised if k.lower() not in cases)
    assert not missing, f"on-device help advertises unimplemented keys: {missing}"

    # Regression: do not re-advertise 1-5 as mimic deploy.
    help_block = text.split("PortGremlinUARTPrintHelp", 1)[-1].split(
        "PortGremlinUARTPrintStatus", 1
    )[0]
    assert "0-9" not in help_block, "help must not claim digits 1-5 are mimic deploy"


def test_mimic_apply_is_reachable_from_uart(firmware_dir: Path) -> None:
    """PortGremlinMimicApply must be called from the UART dispatcher."""
    text = (firmware_dir / "portgremlin_uart.c").read_text()
    assert "PortGremlinMimicApply" in text
    assert "DeployMimicAndReenum" in text
    assert "bForceReenum" in text


def test_readme_documents_only_real_keys(repo_root: Path, firmware_dir: Path) -> None:
    """Keys named in the README UART table must exist in the firmware."""
    readme = (repo_root / "README.md").read_text()
    cases = _uart_cases(firmware_dir)

    section = readme.split("## UART Commands", 1)[-1].split("##", 1)[0]
    rows = [ln for ln in section.splitlines() if ln.strip().startswith("|")]
    assert rows, "could not locate the README UART command table"

    claimed: set[str] = set()
    for row in rows[2:]:  # skip header + separator
        first_cell = row.split("|")[1]
        claimed.update(re.findall(r"`(.)`", first_cell))
        for lo, hi in README_RANGE_RE.findall(first_cell):
            claimed.update(str(c) for c in range(int(lo), int(hi) + 1))

    missing = sorted(k for k in claimed if k.lower() not in cases)
    assert not missing, f"README advertises unimplemented UART keys: {missing}"


def test_telemetry_regex_is_not_greedy() -> None:
    r"""Two @PG events in one chunk must both be recovered.

    The original pattern `@PG(\{.*\})` spanned from the first brace to the
    last, so a coalesced pair produced invalid JSON and both events were
    dropped by the bare ``except JSONDecodeError``.
    """
    import json

    ow = load_tool("portgremlin-overwatch")

    line = (
        '@PG{"e":"host","os":"Linux","lat":95,"rst":0}\r'
        '@PG{"e":"enum","vid":"046D","pid":"C52B","cls":"Keyboard","n":7}'
    )
    events = []
    for match in ow.PG_JSON_RE.finditer(line):
        events.append(json.loads(match.group(1))["e"])
    assert events == ["host", "enum"]


def test_telemetry_payloads_are_flat_json(firmware_dir: Path) -> None:
    """The parser assumes payloads contain no nested objects.

    ``PG_JSON_RE`` matches ``{[^{}]*}``. That is correct only while every
    telemetry payload is flat, so assert the C format strings stay flat.
    """
    text = (firmware_dir / "portgremlin_telemetry.c").read_text()
    payloads = re.findall(r'@PG\{(.*?)\}', text, re.DOTALL)
    assert payloads, "no @PG payloads found in portgremlin_telemetry.c"
    for payload in payloads:
        assert "{" not in payload, f"nested object in telemetry payload: {payload!r}"


def test_all_host_tools_compile() -> None:
    """Every shipped tool must at least parse."""
    repo = Path(__file__).resolve().parent.parent
    tools = sorted((repo / "tools").glob("*.py"))
    assert tools, "no tools found"
    for tool in tools:
        result = subprocess.run(
            [sys.executable, "-m", "py_compile", str(tool)],
            capture_output=True,
            text=True,
        )
        assert result.returncode == 0, f"{tool.name} failed to compile:\n{result.stderr}"


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
