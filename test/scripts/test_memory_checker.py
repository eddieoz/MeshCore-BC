import pytest
from scripts.memory_checker import parse_memory_usage, check_memory_thresholds


def test_parse_memory_usage_basic():
    output = """
RAM:   [======    ]  60.7% (used 142944 bytes from 235520 bytes)
Flash: [======    ]  63.2% (used 447528 bytes from 708608 bytes)
""".strip()
    result = parse_memory_usage(output)
    assert result["ram_percent"] == 60.7
    assert result["ram_used"] == 142944
    assert result["ram_total"] == 235520
    assert result["flash_percent"] == 63.2
    assert result["flash_used"] == 447528
    assert result["flash_total"] == 708608


def test_parse_memory_usage_missing():
    result = parse_memory_usage("No memory data here")
    assert result is None


def test_check_memory_thresholds_pass():
    stats = {
        "ram_percent": 60.7,
        "flash_percent": 63.2,
    }
    errors = check_memory_thresholds(stats, ram_max=80.0, flash_max=90.0)
    assert errors == []


def test_check_memory_thresholds_fail_ram():
    stats = {
        "ram_percent": 85.0,
        "ram_used": 200000,
        "ram_total": 235520,
        "flash_percent": 50.0,
        "flash_used": 350000,
        "flash_total": 708608,
    }
    errors = check_memory_thresholds(stats, ram_max=80.0, flash_max=90.0)
    assert len(errors) == 1
    assert "RAM" in errors[0]


def test_check_memory_thresholds_fail_flash():
    stats = {
        "ram_percent": 50.0,
        "ram_used": 117760,
        "ram_total": 235520,
        "flash_percent": 95.0,
        "flash_used": 673178,
        "flash_total": 708608,
    }
    errors = check_memory_thresholds(stats, ram_max=80.0, flash_max=90.0)
    assert len(errors) == 1
    assert "Flash" in errors[0]
