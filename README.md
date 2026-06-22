# 🚀 Spicy Invaders

**A fully handcrafted arcade space shooter built in C++ and SFML.**

Enemy waves, power-ups, an EMP mechanic, explosion animations, and a three-boss arcade mode where every boss has unique attack patterns and health bars. Built from scratch using pure OOP and raw dynamic memory — no STL, no shortcuts.

> Built by **Tabish & Areeb** — CS Students @ FAST-NUCES Lahore

---

## 🎮 Features

- 👾 Progressive enemy waves with escalating difficulty
- 💥 Projectile physics and collision detection
- ⚡ Power-ups including an EMP mechanic that wipes the screen
- 🔥 Explosion animations via spritesheet rendering
- 🏆 Three-boss arcade mode:
  - **BossCruiser** — heavy front assault
  - **BossTwinCannons** — dual turret composed attack patterns
  - **BossMothership** — spawns minions mid-fight
- 🎨 Sprite-based rendering pipeline
- 🧠 GamePhase state machine for smooth scene transitions
- 🛠️ Raw OOP architecture — manual dynamic memory, no STL

---

## 🛠️ Built With

- **Language:** C++
- **Library:** SFML (Simple and Fast Multimedia Library)
- **IDE:** Visual Studio
- **Platform:** Windows

---

## ▶️ How to Run

### Requirements
- Windows OS
- [Visual Studio](https://visualstudio.microsoft.com/) (2019 or later recommended)
- [SFML 2.5.1](https://www.sfml-dev.org/download.php) — make sure it is linked in the project properties

### Steps

1. **Clone or download this repository**
   ```
   git clone https://github.com/tabishfrfr02/SpicyInvaders.git
   ```
   Or click **Code → Download ZIP** and extract it.

2. **Open the solution file**
   - Navigate to the project folder
   - Double-click `SpicyInvaders.sln` to open it in Visual Studio

3. **Set up SFML (if not already linked)**
   - Download SFML 2.5.1 from https://www.sfml-dev.org/download.php
   - In Visual Studio: go to **Project → Properties**
   - Under **C/C++ → General**, add the SFML `include` folder path
   - Under **Linker → General**, add the SFML `lib` folder path
   - Under **Linker → Input**, add the required SFML `.lib` files

4. **Copy SFML .dll files**
   - Copy all SFML `.dll` files into the same folder as the `.exe` (usually `Debug/` or `Release/`)
   - These are found in the SFML `bin/` folder

5. **Build and Run**
   - Press `Ctrl + Shift + B` to build
   - Press `F5` or click **Local Windows Debugger** to run

> ⚠️ Make sure the `assets/` folder (sprites, fonts, sounds) stays in the same directory as the `.exe` when running.

---

## 📁 Project Structure

```
SpicyInvaders/
├── src/                  # All .cpp source files
├── include/              # All .h / .hpp header files
├── assets/               # Sprites, fonts, sounds, spritesheets
├── SpicyInvaders.sln     # Visual Studio solution file
└── README.md
```

---

## 👨‍💻 Developers

| Name | GitHub |
|------|--------|
| Tabish Moin | [@tabishfrfr02](https://github.com/tabishfrfr02) |
| Areeb | idk his username imma text him later  

---

## 📄 License

This project was built for educational purposes as part of coursework at FAST-NUCES Lahore.
