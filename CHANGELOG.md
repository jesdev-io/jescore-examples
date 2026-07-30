# Changelog

## 2026-07-30

### Added
- 👋 Additional Hello World examples for STM32L476RG, STM32G431KB, STM32H750 and STM32H753
- 📤 Multi UART streamer example
- 📁 STM32H750 Makefile template

### Changed
- 🖨️ Updated examples and templates for the current `jes_print()` API
- 🔆 Use the current `jescore` dependency instead of pinning an old version
- 📚 Completed the README example list for the release

### Fixed
- 📁 Ignore local build outputs

## 2026-05-20

### Added
- 👋 Hello World examples for STM32G431KB and STM32H753
- 📁 Makefile template for STM32H750 (works for all STM32 boards if adjust)

### Changed
- 🖨️ Updated all examples to use `jes_print()`
- 📁 Updated Makefile template to reflect new changes
- 🔆 Unfix `jescore` version
- 🖨️ Switch to `jes_print()`

### Fixed
- 📁 Ignore local builds in git (result of Makefile support)

## 2026-04-21

### Added
- 🛣️ Multi UART streamer example

### Changed
- 🔆 Main `jescore` version 2.3.0
- ⛏️ Set `tool-esptoolpy` to version 1.40501.0 because the newest is broken (?) 

## 2025-11-21

### Added
- ➡️ This changelog
- ✨ `README.md`
- 😺 GitHub Actions
- 🎤 Stereo SPL Meter example

### Changed
- 📛 Repo name: `examples` ➡️ `jescore-examples`
