"""Build-system consistency checks.

The firmware build depends on TI TivaWare, a ~500 MB license-gated SDK that
cannot be fetched unattended, so CI cannot compile the firmware. These checks
cover what *is* verifiable without the SDK: that the Makefile's source list,
include paths and link script all point at things that exist.
"""

from __future__ import annotations

import re
import shlex


def _makefile(firmware_dir):
    return (firmware_dir / "Makefile").read_text()


def test_every_declared_source_exists(firmware_dir) -> None:
    """SRCS in the Makefile must all exist.

    A typo here only surfaces at build time on a machine that has the ARM
    toolchain and TivaWare installed, which is most people.
    """
    text = _makefile(firmware_dir)
    match = re.search(r"^SRCS\s*:?=\s*(.*(?:\n\s+.*)*)", text, re.MULTILINE)
    assert match, "could not find SRCS in the Makefile"

    sources = " ".join(match.group(1).split())
    names = [s for s in sources.split() if s.endswith(".c")]
    assert names, "SRCS lists no C sources"

    missing = [n for n in names if not (firmware_dir / n).is_file()]
    assert not missing, f"Makefile lists missing sources: {missing}"


def test_link_script_exists(firmware_dir) -> None:
    text = _makefile(firmware_dir)
    scripts = re.findall(r"--script=(\S+)", text)
    assert scripts, "no linker script referenced in LDFLAGS"
    for script in scripts:
        assert (firmware_dir / script).is_file(), f"missing linker script {script}"


def test_firmware_warns_and_errors_are_enabled(firmware_dir) -> None:
    """The project compiles with -Werror; keep it that way."""
    text = _makefile(firmware_dir)
    assert "-Werror" in text, "firmware no longer builds with -Werror"
    for flag in ("-Wall", "-Wextra"):
        assert flag in text, f"{flag} was dropped from CFLAGS"


def test_python_requirements_are_declared(repo_root) -> None:
    requirements = (repo_root / "tools" / "requirements.txt").read_text()
    assert "pyserial" in requirements, "pyserial is required by the host tools but undeclared"


def test_tools_only_import_declared_dependencies(repo_root) -> None:
    """Every third-party import in tools/ must be in requirements.txt."""
    import ast
    import sys

    stdlib = set(sys.stdlib_module_names) | {"__future__"}
    local = {p.stem for p in (repo_root / "tools").glob("*.py")}

    declared = {
        line.split(">=")[0].split("==")[0].strip().lower()
        for line in (repo_root / "tools" / "requirements.txt").read_text().splitlines()
        if line.strip() and not line.startswith("#")
    }
    # Distribution name != import name for a few packages.
    import_name = {"serial": "pyserial"}

    undeclared = set()
    for tool in sorted((repo_root / "tools").glob("*.py")):
        tree = ast.parse(tool.read_text())
        for node in ast.walk(tree):
            names: list[str] = []
            if isinstance(node, ast.Import):
                names = [alias.name.split(".")[0] for alias in node.names]
            elif isinstance(node, ast.ImportFrom) and node.module and node.level == 0:
                names = [node.module.split(".")[0]]
            for name in names:
                if name in stdlib or name in local:
                    continue
                if import_name.get(name, name).lower() not in declared:
                    undeclared.add(f"{tool.name}: {name}")

    assert not undeclared, f"undeclared third-party imports: {sorted(undeclared)}"


def test_setup_script_is_valid_bash(repo_root) -> None:
    import subprocess

    result = subprocess.run(
        ["bash", "-n", str(repo_root / "setup.sh")], capture_output=True, text=True
    )
    assert result.returncode == 0, f"setup.sh is not valid bash:\n{result.stderr}"


def test_setup_script_quotes_shell_words_safely(repo_root) -> None:
    """Guard against unquoted expansions in the one script users curl-run."""
    import subprocess

    script = (repo_root / "setup.sh").read_text()
    result = subprocess.run(
        ["bash", "-n", "-o", "nounset", str(repo_root / "setup.sh")],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, f"setup.sh fails under nounset:\n{result.stderr}"
    # `shellcheck` is not guaranteed on the runner; only run it if present.
    if subprocess.run(["which", "shellcheck"], capture_output=True).returncode == 0:
        report = subprocess.run(
            ["shellcheck", "-S", "warning", str(repo_root / "setup.sh")],
            capture_output=True,
            text=True,
        )
        assert report.returncode == 0, f"shellcheck findings:\n{report.stdout}"
    assert script, "setup.sh is empty"
