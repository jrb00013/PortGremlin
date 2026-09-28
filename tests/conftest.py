"""Shared fixtures: make the host-side tools importable as modules.

The tools live in ``tools/`` and are standalone scripts, so they are not a
package. Adding that directory to ``sys.path`` lets the tests import them
directly and assert against the real shipped code rather than a copy.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path
from types import ModuleType

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent
TOOLS_DIR = REPO_ROOT / "tools"
FIRMWARE_DIR = REPO_ROOT / "usb_dev_keyboard"

if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))


def load_tool(module_name: str) -> ModuleType:
    """Import a shipped host tool by file stem.

    The tools are standalone scripts with hyphens in their filenames, so they
    cannot be imported by module name. Registering in ``sys.modules`` before
    execution is required because they use ``from __future__ import
    annotations`` together with ``@dataclass``, which resolves annotations
    against the registered module.
    """
    path = TOOLS_DIR / f"{module_name}.py"
    if not path.is_file():
        raise FileNotFoundError(path)

    safe_name = module_name.replace("-", "_")
    spec = importlib.util.spec_from_file_location(safe_name, path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[safe_name] = module
    spec.loader.exec_module(module)
    return module


@pytest.fixture(scope="session")
def repo_root() -> Path:
    return REPO_ROOT


@pytest.fixture(scope="session")
def firmware_dir() -> Path:
    return FIRMWARE_DIR
