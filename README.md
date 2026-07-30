<img width="1920" height="300" alt="jescore_examples_logo_banner" src="https://github.com/user-attachments/assets/b05f7ef9-e437-47c2-84b4-22feb22bd528" />

# `jescore` Examples
|Main Repo 🔆|`main` CI 📦️|`dev` CI 🚢|Support 🙏|
|-|-|-|-|
|[<img src="https://github.com/user-attachments/assets/2fc4f696-0a6c-444b-a99b-053f9bee6d59" width="100"/>](https://github.com/jesdev-io/jescore)|[![Main](https://github.com/jesdev-io/jescore-examples/actions/workflows/workflow.yml/badge.svg?branch=main)](https://github.com/jesdev-io/jescore-examples/actions/workflows/workflow.yml)|[![Dev](https://github.com/jesdev-io/jescore-examples/actions/workflows/workflow.yml/badge.svg?branch=dev)](https://github.com/jesdev-io/jescore-examples/actions/workflows/workflow.yml)|[![Buy me a coffee](https://img.shields.io/badge/Ko--fi-Support%20Me-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/jseshack)|

**This repo presents various applications utilizing `jescore`.**

## How to use
Examples can best be explored with [PlatformIO](https://platformio.org/). Clone this repo and open it with VSCode or any other IDE that supports PlatformIO, or use its [dedicated CLI](https://docs.platformio.org/en/latest/core/userguide/index.html) for headless or custom usage. If you don't use PlatformIO, be sure that you can compile and link your chosen MCU's required libraries and you can also compile and link the [Official `jescore` Repo](https://github.com/jesdev-io/jescore).

### Example Structure
Examples are organized to be standalone apps. The actual applications utilizing `jescore` and containing the "interesting" code are found in `src/`. If you use PlatformIO, an environment with the same name is selectable, see `platformio.ini`. 

If examples require more low level driver code, for example due to the inclusion of specialized low level drivers, they are shipped with an additional folder called like the example in `lib/`, optionally with the prefix `port_*` (see [notes](#additional-notes)).

## Additional Notes
- The `port_*` prefix under `lib/` comes from porting STM32CubeIDE projects to PlatformIO with [`cube2pio`](https://github.com/jesdev-io/cube2pio)

## List of Examples
|Example|Requires|Platform|
|-|-|-|
|👋 `hello_world_arduino`||ESP32 + Arduino|
|👋 `hello_world_nucleol432kc`||STM32 NUCLEOL432KC|
|👋 `hello_world_nucleol476rg`||STM32 NUCLEOL476RG|
|👋 `hello_world_nucleo_g431kb`||STM32 NUCLEOG431KB|
|👋 `hello_world_stm32h750`||STM32H750B-DK|
|👋 `hello_world_stm32h753`||STM32H753I-EVAL|
|🧮 `fsm_cli_arduino`||ESP32 + Arduino|
|⏳️ `sync_async_arduino`||ESP32 + Arduino|
|📤 `multi_uart_streamer_arduino`||ESP32 + Arduino|
|🎤 `stereo_spl_meter_nucleol432kc`|[I²S](lib/port_stereo_spl_meter_nucleol432kc/)|STM32 NUCLEOL432KC|
