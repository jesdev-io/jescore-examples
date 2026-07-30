# Changelog

## 2026-07-30

### Added
- 🧪 Developer access example for hiding CLI-only maintenance commands behind an unlock step
- 🎛️ Hardware virtualization example for driving interrupt-style behavior from CLI commands
- 🔁 Inter-MCU control example for forwarding CLI commands to another `jescore` device over UART
- 👋 Missing README/changelog coverage for the STM32L476RG and STM32H750 Hello World examples

### Changed
- 📚 Completed the README example list for the release

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
