<<<<<<< HEAD
# 🎞️ VCPlayer – C++ Video Player with SDL2, FFmpeg, and ImGui

A lightweight, modular, and modern video player built using C++17, SDL2, FFmpeg, and Dear ImGui.  
It features a custom ImGui-based interface and includes file browsing and volume control with simple buttons.
=======
Here is an **updated README.md** for your VCPlayer project, reflecting the latest features: video and audio playback, pause, resume, accurate seeking, basic volume control/display, timeline slider, and maximizable window support.

```markdown
# 🎞️ VCPlayer — C++ Video Player (SDL2 & FFmpeg)

VCPlayer is a modern, modular C++17 desktop video player that uses [FFmpeg](https://ffmpeg.org/) for decoding and [SDL2](https://libsdl.org) for audio/video output. It features a custom ImGui interface with volume control, timeline slider, seek, and file dialog.
>>>>>>> 5e3f503 (Added the Video Timer , SLider , Full_screen feature and some keyboard-shortcuts)

---

## ✨ Features

<<<<<<< HEAD
- 📼 **Video Playback** using FFmpeg decoding and SDL2 rendering
- 🖱️ **Custom GUI** using Dear ImGui
- 🗂️ **File Dialog** support using ImGuiFileDialog (no native file dialog dependencies)
- 🔊 **Volume Control** using `+` and `–` buttons in the GUI
- 🧩 Modular C++ class structure (`App`, `VideoPlayer`, `FileDialog`, etc.)
- 💡 Minimal dependencies, easy to build and extend
=======
- 📼 **Video Playback** (FFmpeg decode + SDL2 render, supports most formats)
- 🔉 **Audio Playback** — simple, effective PCM decoding and output
- 🎚️ **Volume Control** — plus/minus buttons + display
- ⏩ **Seeking** — forward/back and instant timeline jumps
- 🕒 **Timeline Slider** and time display ("current / total")
- ⏸️ **Pause & Resume**
- 🗂 **Intuitive ImGui UI** with file browser (ImGuiFileDialog)
- 🪟 **Resizable/maximizable SDL2 window** with maximize button
- 💡 Clean, educational, and extensible codebase
>>>>>>> 5e3f503 (Added the Video Timer , SLider , Full_screen feature and some keyboard-shortcuts)

---

## 🖥️ User Interface

<<<<<<< HEAD
- **Open File:** Load a video using an ImGui file browser
- **Play Video:** Automatically plays selected video
- **Volume Buttons:** Increase or decrease volume with on-screen `+` and `-` buttons
=======
- **Open File:** File menu, ImGui dialog, pick a local video (MP4, AVI, MKV, etc.)
- **Timeline:** Slider with draggable playhead; click/drag to seek, with time display
- **Controls:** Play/Pause, Seek Forward/Back, Volume +/–
- **Volume Display:** Shows current volume percent in the menu/GUI
>>>>>>> 5e3f503 (Added the Video Timer , SLider , Full_screen feature and some keyboard-shortcuts)

---

## 🔧 Dependencies

- C++17
<<<<<<< HEAD
- [SDL2](https://libsdl.org)
- [FFmpeg](https://ffmpeg.org): `libavcodec`, `libavformat`, `libavutil`, `libswscale`, `libswresample`
- [Dear ImGui](https://github.com/ocornut/imgui)
- [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)

---

## 🚀 Build Instructions

```bash
# Clone the repo
git clone https://github.com/ArvendraChhonkar/MyPlayer.git
cd MyPlayer

# Create build directory
mkdir build && cd build

# Run CMake
cmake ..

# Build
make
```

🔧 Make sure SDL2 and FFmpeg development libraries are installed on your system.

📁 Folder Structure
```bash
Copy
Edit
MyPlayer/
├── imgui/                     # Dear ImGui source
├── ImGuiFileDialog/          # ImGui-based file dialog
├── App.cpp / App.h           # Main application logic
├── VideoPlayer.cpp / .h      # Handles decoding and rendering
├── FileDialog.cpp / .h       # Handles file dialog integration
├── main.cpp                  # Entry point
├── CMakeLists.txt
└── README.md
```
🔊 Volume Control
The video player now supports basic volume control via ImGui buttons:

+ button to increase volume

– button to decrease volume

Volume is applied using FFmpeg’s libswresample

📢 This is useful for quick sound adjustment without needing keyboard shortcuts.

🧪 Test Video
For testing purposes, you can use any .mp4, .avi, or .mkv file.
Or place a sample test video (like arvv.mp4) in the root folder.

✅ TODO (Future Features)
⏸️ Pause/Resume

🔁 Seeking (forward/backward)

🎵 Audio visualizations

🌐 URL/network stream support

🛠 Drag-and-drop support

🧑‍💻 Author
Arvendra Chhonkar

GitHub: [ArvendraChhonkar](https://github.com/ArvendraChhonkar)

📜 License
MIT License – feel free to modify and distribute.

yaml
Copy
Edit

---

Let me know if you want a screenshot of the UI, or want a minimal GIF of the video player in action — that could really enhance your `README.md`.
=======
- `libavformat`, `libavcodec`, `libavutil`, `libswscale`, `libswresample` (FFmpeg)
- `libsdl2`
- [Dear ImGui](https://github.com/ocornut/imgui)
- [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)
- [CMake](https://cmake.org) for build

### _Sample install_ (Ubuntu/Debian):

```
sudo apt-get install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libswresample-dev libsdl2-dev cmake g++
```

---

## 🚀 Building

```
git clone https://github.com/ArvendraChhonkar/VCPlayer.git
cd VCPlayer
mkdir build && cd build
cmake ..
make
./MyPlayer
```

---

---

## 💡 Usage

- Start `./MyPlayer`
- Open a file via the GUI
- Control via ImGui buttons/menu:
    - Play/Pause
    - Seek Forward/Back (`>>` / `<<`)
    - Timeline Slider (drag to any point)
    - Volume `++V` / `V--`
- The ImGui menu always shows current volume (as a percent).
- Window is resizable/maximizable.
- Tested on Linux; CMake builds easily on other platforms with required libraries.

---

## 📂 Project Layout

- `App.cpp/.h` — SDL2/ImGui main application class
- `VideoPlayer.cpp/.h` — Handles all FFmpeg decoding, seeking, audio, video, and volume
- `FileDialog.cpp/.h` — ImGui-based file dialog logic
- `main.cpp` — entry point

---

## 🛑 Known Limitations

- Only supports direct PCM (float/S16) audio; some codec formats may not provide sound
- No subtitle or playlist support yet
- Limited audio resampling support
- Local files only (no network streaming)

---

## UI :->>
<img width="849" height="698" alt="image" src="https://github.com/user-attachments/assets/cc3effa5-1eb3-4040-8e0c-e94c71e30702" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/b01d41be-2ea3-4c2d-883e-0c9db2daeabc" />







## 👤 Author

**Arvendra Chhonkar**  
GitHub: [ArvendraChhonkar](https://github.com/ArvendraChhonkar)

---

## 📜 License

MIT License — Free to use, modify, and distribute.

---

## 🤝 Contributing

Contributions and issues welcome!  
See repo Issues or open a pull request.

---

>>>>>>> 5e3f503 (Added the Video Timer , SLider , Full_screen feature and some keyboard-shortcuts)
