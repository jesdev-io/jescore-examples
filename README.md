# `jescore` Examples

[Check out `jescore`](https://github.com/jesdev-io/jescore)!
 | 
 
 [![Main](https://github.com/jake-is-ESD-protected/jescore-examples/actions/workflows/workflow.yml/badge.svg?branch=main)](https://github.com/jake-is-ESD-protected/jescore-examples/actions/workflows/workflow.yml)

 
 | [![Dev](https://github.com/jake-is-ESD-protected/jescore-examples/actions/workflows/workflow.yml/badge.svg?branch=dev)](https://github.com/jake-is-ESD-protected/jescore-examples/actions/workflows/workflow.yml)

This repo presents various applications utilizing `jescore`. 



## How to use
Examples can best be explored with [PlatformIO](https://platformio.org/). Clone this repo and open it with VSCode or any other IDE that supports PlatformIO, or use its [dedicated CLI](https://docs.platformio.org/en/latest/core/userguide/index.html) for headless or custom usage. If you don't use PlatformIO, be sure that you can compile and link your chosen MCU's required libraries and you can also compile and link the [Official `jescore` Repo](https://github.com/jesdev-io/jescore).

### Example Structure
Examples are organized to be standalone apps. The actual applications utilizing `jescore` and containing the "interesting" code are found in `src/`. If you use PlatformIO, an environment with the same name is selectable, see `platformio.ini`. 

If examples require more low level driver code, for example due to the inclusion of specialized low level drivers, they are shipped with an additional folder called like the example in `lib/`, optionally with the prefix `port_*` (see [notes](#additional-notes)).

## Additional Notes
- The `port_*` prefix under `lib/` comes from porting STM32CubeIDE projects to PlatformIO with [`cube2pio`](https://github.com/jesdev-io/cube2pio)