"""Behaviour tests for the Virtual Lab firmware simulation.

The simulator is the only part of this project that encodes firmware semantics
without needing a LaunchPad attached, which makes it the natural place to pin
down regressions. Two real bugs are covered here:

* ``classify_host`` could only ever return five answers, two of which were
  always wrong -- an Embedded host read as Linux and an Unknown host read as
  macOS. The Oracle could never admit it did not know.
* Driver confusion reached its "contradiction" flag through two paths, but only
  one of them populated the pinned VID/PID. When the genetic engine enabled
  contradiction on its own, the device emitted VID/PID 0x0000, the reserved
  "no device" identity.
"""

from __future__ import annotations

import random

import pytest

from conftest import TOOLS_DIR  # noqa: F401  (ensures tools/ is importable)
import sim_engine as se
from sim_engine import BrainPhase, DeviceClass, HostOS, Persona, PortGremlinSimulator


# --------------------------------------------------------------------------
# Host classification
# --------------------------------------------------------------------------


@pytest.mark.parametrize(
    "host",
    [HostOS.WINDOWS, HostOS.LINUX, HostOS.MACOS, HostOS.EMBEDDED, HostOS.UNKNOWN],
)
def test_every_host_type_is_reachable(host: HostOS) -> None:
    """Each simulated host must be classifiable back to itself.

    Before the fix, EMBEDDED always classified as LINUX and UNKNOWN always as
    macOS -- neither answer was ever produced, so the simulation could not
    represent those hosts at all.
    """
    sim = PortGremlinSimulator()
    correct = 0
    trials = 2000
    for _ in range(trials):
        sim.state.host_os = host
        sim.state.config_latency_ms, sim.state.reset_count = sim._host_latency_profile()
        sim.classify_host()
        if sim.state.host_os is host:
            correct += 1
    assert correct / trials >= 0.6, f"{host.value} only round-trips {correct}/{trials}"


def test_ambiguous_reading_reports_unknown_rather_than_guessing() -> None:
    """A latency matching no known profile must classify as UNKNOWN.

    A confident wrong answer is worse here: SPECTRE selects its persona from
    the classified host, so a bad label silently picks the wrong behaviour.
    """
    sim = PortGremlinSimulator()
    sim.state.config_latency_ms = 75
    sim.state.reset_count = 0
    sim.classify_host()
    assert sim.state.host_os is HostOS.UNKNOWN


def test_embedded_is_not_confused_with_linux() -> None:
    """Direct regression: 120ms was classified as Linux by the old banding."""
    sim = PortGremlinSimulator()
    sim.state.config_latency_ms = 120
    sim.state.reset_count = 0
    sim.classify_host()
    assert sim.state.host_os is HostOS.EMBEDDED


def test_persistent_resets_imply_windows_regardless_of_latency() -> None:
    sim = PortGremlinSimulator()
    sim.state.config_latency_ms = 200
    sim.state.reset_count = 3
    sim.classify_host()
    assert sim.state.host_os is HostOS.WINDOWS


def test_profiles_cover_every_host_os() -> None:
    """The classifier and the synthesiser share one table, so keep them total."""
    assert set(se.HOST_LATENCY_PROFILES) == set(HostOS)
    for host, (latency, resets) in se.HOST_LATENCY_PROFILES.items():
        assert latency > 0, f"{host.value} has a non-positive latency profile"
        assert resets >= 0, f"{host.value} has a negative reset profile"


def test_classification_is_deterministic_for_fixed_input() -> None:
    sim = PortGremlinSimulator()
    results = set()
    for _ in range(5):
        sim.state.config_latency_ms = 95
        sim.state.reset_count = 0
        sim.classify_host()
        results.add(sim.state.host_os)
    assert len(results) == 1, "classification is not deterministic for fixed input"


# --------------------------------------------------------------------------
# Driver confusion / contradiction
# --------------------------------------------------------------------------


def test_unpinned_contradiction_never_emits_reserved_identity() -> None:
    """Regression: genome-driven contradiction emitted VID/PID 0x0000.

    VID 0x0000 is the reserved "no device" identity. The genetic engine can
    flip the contradiction bit without ever populating the pin, so the
    override must be skipped rather than emitting a null identity.
    """
    leaked = 0
    for seed in range(500):
        random.seed(seed)
        sim = PortGremlinSimulator()
        sim.state.contradiction = True
        sim.state.pinned_vid = 0
        sim.state.pinned_pid = 0
        sim.state.malformed = False
        sim.state.genome.malformed = False
        sim.state.auto_cycle = False
        sim.enumerate()
        if sim.state.vid == 0x0000:
            leaked += 1
    assert leaked == 0, f"emitted reserved VID 0x0000 in {leaked}/500 runs"


def test_pinned_contradiction_still_emits_the_pinned_identity() -> None:
    """The driver-confusion feature must keep working after the fix above."""
    hits = 0
    for seed in range(500):
        random.seed(seed)
        sim = PortGremlinSimulator()
        sim.toggle_contradiction()
        sim.state.malformed = False
        pin = (sim.state.pinned_vid, sim.state.pinned_pid)
        sim.enumerate()
        if (sim.state.vid, sim.state.pid) == pin:
            hits += 1
    assert hits == 500, f"pinned identity emitted only {hits}/500 times"


def test_toggle_contradiction_always_pins_a_valid_identity() -> None:
    for seed in range(200):
        random.seed(seed)
        sim = PortGremlinSimulator()
        sim.toggle_contradiction()
        assert sim.state.pinned_vid != 0
        assert sim.state.pinned_pid != 0


def test_haunted_persona_pins_a_valid_identity() -> None:
    for seed in range(200):
        random.seed(seed)
        sim = PortGremlinSimulator()
        sim._apply_persona_config(Persona.HAUNTED)
        assert sim.state.contradiction is True
        assert sim.state.pinned_vid != 0
        assert sim.state.pinned_pid != 0


def test_contradiction_disabled_leaves_identity_unpinned_by_default() -> None:
    sim = PortGremlinSimulator()
    assert sim.state.contradiction is False
    assert sim._contradiction_pinned() is False


def test_evolution_path_populates_the_pin() -> None:
    """The genetic engine must not enable contradiction without a pin."""
    random.seed(11)
    sim = PortGremlinSimulator()
    sim.start()
    sim.toggle_evolve()
    sim.state.genome.contradiction = True
    sim.state.pinned_vid = 0
    sim.state.pinned_pid = 0
    sim.tick(1.0)
    assert sim.state.contradiction is True
    assert sim._contradiction_pinned() is True


# --------------------------------------------------------------------------
# General invariants
# --------------------------------------------------------------------------


def test_persona_cycle_covers_every_non_manual_persona() -> None:
    non_manual = [p for p in Persona if p is not Persona.MANUAL]
    seen = []
    current = Persona.MANUAL
    for _ in range(len(non_manual)):
        current = Persona.cycle(current)
        seen.append(current)
    assert len(set(seen)) == len(non_manual), "persona cycle repeated before covering all"
    assert set(seen) == set(non_manual)


def test_persona_cycle_returns_to_start() -> None:
    current = Persona.MANUAL
    for _ in range(len(Persona) - 1):
        current = Persona.cycle(current)
    assert Persona.cycle(current) is Persona.CHIMERA


def test_mimic_vault_indices_wrap() -> None:
    sim = PortGremlinSimulator()
    total = len(se.MIMIC_VAULT)
    sim.deploy_mimic(0)
    first = (sim.state.vid, sim.state.pid)
    sim.deploy_mimic(total)
    assert (sim.state.vid, sim.state.pid) == first


def test_mimic_vault_identities_are_well_formed() -> None:
    for profile in se.MIMIC_VAULT:
        assert profile.vid not in (0x0000, 0xFFFF), f"{profile.product} has reserved VID"
        assert profile.pid not in (0x0000, 0xFFFF), f"{profile.product} has reserved PID"
        assert isinstance(profile.device_class, DeviceClass)


def test_toggling_brain_resets_the_escalation_ladder() -> None:
    sim = PortGremlinSimulator()
    sim.state.brain_phase = BrainPhase.CHAOS
    sim.toggle_brain()
    assert sim.state.brain_phase is BrainPhase.IDLE
    assert sim.state.brain_active is True
    sim.toggle_brain()
    assert sim.state.brain_phase is BrainPhase.IDLE
    assert sim.state.brain_active is False


def test_enabling_evolution_disables_the_brain() -> None:
    sim = PortGremlinSimulator()
    sim.toggle_brain()
    assert sim.state.brain_active is True
    sim.toggle_evolve()
    assert sim.state.brain_active is False
    assert sim.state.evolve_active is True


def test_enumeration_counter_and_device_cache_are_bounded() -> None:
    random.seed(5)
    sim = PortGremlinSimulator()
    sim.start()
    sim._apply_persona_config(Persona.STORM)
    for _ in range(400):
        sim.enumerate()
    assert sim.state.enum_count == 400
    assert len(sim.state.host_devices) <= 24


def test_event_log_is_bounded() -> None:
    sim = PortGremlinSimulator()
    for i in range(900):
        sim.log("test", f"event {i}")
    assert len(sim.state.events) <= 500


def test_pain_score_and_tolerance_stay_in_range() -> None:
    random.seed(21)
    sim = PortGremlinSimulator()
    sim.start()
    sim._apply_persona_config(Persona.HAUNTED)
    for _ in range(300):
        sim.enumerate()
    assert 0.0 <= sim.state.pain_score <= 100.0
    assert 0 <= sim.state.tolerance <= 100


def test_tick_is_inert_when_not_running() -> None:
    sim = PortGremlinSimulator()
    sim.state.auto_cycle = True
    before = sim.state.enum_count
    for _ in range(50):
        sim.tick(1.0)
    assert sim.state.enum_count == before


def test_reset_restores_defaults_but_keeps_the_detected_host() -> None:
    sim = PortGremlinSimulator()
    sim.set_host_os(HostOS.MACOS)
    sim._apply_persona_config(Persona.STORM)
    sim.start()
    sim.enumerate()
    sim.reset()
    assert sim.state.host_os is HostOS.MACOS
    assert sim.state.persona is Persona.MANUAL
    assert sim.state.enum_count == 0
    assert sim.state.running is False


def test_overdrive_engages_the_full_stack() -> None:
    sim = PortGremlinSimulator()
    sim.overdrive()
    assert sim.state.brain_active is True
    assert sim.state.evolve_active is True
    assert sim.state.auto_cycle is True
    assert sim.state.brain_phase is BrainPhase.IDLE
