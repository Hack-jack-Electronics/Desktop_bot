# 🤖 Desktop_bot

**A screen-and-voice desktop companion bot, powered by a Blackpill (STM32F411CEUX).**

Desktop_bot listens, responds, and shows expressions/status on a screen — built as a compact, always-on companion for your desk.

---

## ✨ Overview

Desktop_bot is an embedded companion device that combines audio input/output with a visual display to create an interactive desk companion. It runs on **FreeRTOS** on an STM32F411CEUX (Blackpill), with every peripheral driver — display, I2S audio, SD card — **written entirely from scratch, with no external driver libraries.**

## 🧩 Features

- 🎤 **Voice input** via I2S MEMS microphone (INMP441) for real-time audio capture
- 🔊 **Audio output** via I2S Class-D amplifier (MAX98357A) — *speaker integration in progress*
- 🖥️ **ILI9341 TFT display** for visual feedback, expressions, or status — driven by a custom, from-scratch SPI driver (no graphics library)
- 💾 **SD card support** (optional) for local audio/data storage
- ⚙️ Runs on **FreeRTOS**, tasks split across audio capture, display rendering, and control logic
- 🔩 Built on **STM32F411CEUX (Blackpill)** using STM32CubeIDE — all peripheral drivers hand-written (no HAL abstraction libraries for the core drivers)

## 🛠️ Hardware

| Component | Part | Purpose |
|---|---|---|
| MCU | STM32F411CEUX (Blackpill) | Core processor |
| Microphone | INMP441 (I2S MEMS mic) | Audio capture |
| Amplifier | MAX98357A (I2S Class-D amp) | Audio playback / speaker driver |
| Display | ILI9341 (SPI TFT) | Visual output / expressions |
| Storage | microSD card (optional) | Local audio/asset storage |

> 📌 **Status:** Mic input, display, and SD card integration are functional. Speaker output via MAX98357A is the current focus.

### Wiring

| Signal | INMP441 | MAX98357A | Blackpill Pin |
|---|---|---|---|
| I2S Clock (BCLK) | SCK | BCLK | *TBD* |
| I2S Word Select (LRCK/WS) | WS | LRC | *TBD* |
| I2S Data | SD (out) | DIN (in) | *TBD* |
| Power | 3.3V | 5V/3.3V | — |
| Ground | GND | GND | — |

*(Fill in exact GPIO pins from your `.ioc` configuration once finalized.)*

#### ILI9341 (SPI)

| Signal | ILI9341 Pin | Blackpill Pin |
|---|---|---|
| SCK | SCK | *TBD* |
| MOSI | SDI (MOSI) | *TBD* |
| MISO | SDO (MISO) | *TBD* |
| Chip Select | CS | *TBD* |
| Data/Command | DC | *TBD* |
| Reset | RESET | *TBD* |
| Power | VCC / LED | 3.3V |
| Ground | GND | — |

## 🧠 Software Architecture

- **RTOS:** FreeRTOS — audio capture, display rendering, and control/interaction logic run as separate tasks with their own priorities, communicating via queues/semaphores.
- **Drivers:** Every peripheral driver (SPI for the ILI9341, I2S for the INMP441/MAX98357A, SD card interface) is written from scratch at the register level — no third-party HAL/graphics/codec libraries.
- **Display rendering:** Custom SPI-based ILI9341 driver handling initialization, framebuffer writes, and drawing primitives, without a graphics library.
- **Audio pipeline:** I2S peripheral configured for full-duplex capture (INMP441) and playback (MAX98357A), with buffering handled via FreeRTOS tasks/queues.

## 📁 Project Structure

```
Desktop_bot/
├── Core/              # Application source (main, peripheral init, HAL config)
├── Drivers/           # STM32 HAL/CMSIS drivers
├── Debug/             # Build output (debug configuration)
├── .settings/         # STM32CubeIDE workspace settings
├── DESKTOP_BOT_PROJECT.ioc   # STM32CubeMX peripheral configuration
├── STM32F411CEUX_FLASH.ld    # Flash linker script
├── STM32F411CEUX_RAM.ld      # RAM linker script
└── README.md
```

## 🚀 Getting Started

### Prerequisites
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
- ST-Link (or compatible) programmer/debugger
- Blackpill STM32F411CEUX board

### Build & Flash
1. Clone the repository:
   ```bash
   git clone https://github.com/Hack-jack-Electronics/Desktop_bot.git
   ```
2. Open **STM32CubeIDE** and import the project (`File → Open Projects from File System`).
3. Review/adjust peripheral configuration in `DESKTOP_BOT_PROJECT.ioc` if needed.
4. Build the project (`Project → Build All`).
5. Connect your ST-Link to the Blackpill and flash the firmware (`Run → Debug` or `Run → Run`).

## 🗺️ Roadmap

- [x] FreeRTOS task architecture set up
- [x] Custom ILI9341 SPI driver (from scratch)
- [x] I2S microphone (INMP441) integration
- [x] SD card read/write support
- [ ] Speaker output via MAX98357A (in progress)
- [ ] Voice interaction / response logic
- [ ] Expression/animation system on display

## 🤝 Contributing

This is an active personal/hobby project. Suggestions and issues are welcome — feel free to open an issue or submit a PR.

## 📄 License

*(Add a license of your choice — e.g. MIT — to clarify usage rights.)*

## 👤 Author

**Hack-jack Electronics**
Built by Tanishk Singhal — embedded systems & robotics enthusiast.
