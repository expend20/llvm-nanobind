"""TOML-backed exercise metadata loader for the workshop portal POC."""

from __future__ import annotations

from copy import deepcopy
from pathlib import Path
from typing import Any
import tomllib


EXERCISES_DIR = Path(__file__).resolve().parent / "exercises"

_CACHE: dict[str, dict[str, Any]] | None = None
_ORDER: list[str] | None = None


def _load_one(path: Path) -> dict[str, Any]:
    try:
        raw = tomllib.loads(path.read_text(encoding="utf-8"))
    except tomllib.TOMLDecodeError as exc:
        raise ValueError(f"Failed to parse TOML exercise file {path}: {exc}") from exc

    if not isinstance(raw, dict):
        raise ValueError(f"Exercise file {path} must contain a top-level mapping/object.")
    return raw


def _read_required_text(path: Path, label: str, source_path: Path) -> str:
    if not path.exists():
        raise ValueError(f"Exercise file {source_path} is missing required {label} file: {path}")
    return path.read_text(encoding="utf-8")


def _read_optional_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8")


def _resolve_convention_fields(raw: dict[str, Any], source_path: Path) -> dict[str, Any]:
    ex = deepcopy(raw)
    base_dir = source_path.parent
    stem = source_path.stem
    inferred_id = stem.split("_", 1)[1] if "_" in stem else stem
    disallowed_fields = {
        "id",
        "input_ir_file",
        "starter_code_file",
        "solution_code_file",
        "prompt_file",
    }
    present_disallowed = [name for name in disallowed_fields if name in ex]
    if present_disallowed:
        raise ValueError(
            f"Exercise file {source_path} contains disallowed explicit fields: {present_disallowed}. "
            "Use filename conventions instead."
        )

    ex["id"] = inferred_id
    ex["_source_file"] = str(source_path)
    ex["_base_dir"] = str(base_dir)
    ex["_stem"] = stem

    ex["input_ir"] = _read_optional_text(base_dir / f"{stem}.input.ll")
    ex["starter_code"] = _read_required_text(
        base_dir / f"{stem}.starter.py", "starter_code", source_path
    )
    ex["solution_code"] = _read_required_text(
        base_dir / f"{stem}.solution.py", "solution_code", source_path
    )

    if "prompt" not in ex:
        raise ValueError(f"Exercise file {source_path} must include `prompt` text in TOML.")
    if "hints" not in ex or not isinstance(ex["hints"], list):
        ex["hints"] = []

    return ex


def _ensure_loaded() -> tuple[dict[str, dict[str, Any]], list[str]]:
    global _CACHE, _ORDER
    if _CACHE is not None and _ORDER is not None:
        return _CACHE, _ORDER

    if not EXERCISES_DIR.exists():
        raise RuntimeError(f"Exercises directory not found: {EXERCISES_DIR}")

    files = sorted(EXERCISES_DIR.glob("*.toml"))
    if not files:
        raise RuntimeError(f"No exercise TOML files found in: {EXERCISES_DIR}")

    cache: dict[str, dict[str, Any]] = {}
    order: list[str] = []
    for path in files:
        ex = _resolve_convention_fields(_load_one(path), path)
        ex_id = ex["id"]
        if ex_id in cache:
            raise ValueError(f"Duplicate exercise id `{ex_id}` in file {path}.")
        cache[ex_id] = ex
        order.append(ex_id)

    _CACHE = cache
    _ORDER = order
    return cache, order


def list_exercises() -> list[dict[str, object]]:
    cache, order = _ensure_loaded()
    items: list[dict[str, object]] = []
    for ex_id in order:
        ex = cache[ex_id]
        items.append(
            {
                "id": ex["id"],
                "title": ex.get("title", ""),
                "track": ex.get("track", ""),
                "level": ex.get("level", ""),
                "estimated_minutes": ex.get("estimated_minutes", 0),
                "submission_mode": ex.get("submission_mode", "function"),
            }
        )
    return items


def get_exercise(exercise_id: str, include_solution: bool = False) -> dict[str, object]:
    cache, _ = _ensure_loaded()
    if exercise_id not in cache:
        raise KeyError(f"Unknown exercise id: {exercise_id}")
    ex = deepcopy(cache[exercise_id])
    if not include_solution:
        ex.pop("solution_code", None)
    if not include_solution:
        ex.pop("_source_file", None)
        ex.pop("_base_dir", None)
        ex.pop("_stem", None)
    return ex


def get_ordered_ids() -> list[str]:
    _, order = _ensure_loaded()
    return list(order)
