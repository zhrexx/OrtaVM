<div style="height: 20px;" aria-label="Padding"></div>
<div style="text-align: center; margin-bottom: 20px;">
    <img src="assets/logo.svg" height="128" alt="Logo" style="border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);"/>
</div>

<h2 style="text-align: center; margin-top: -5px; font-family: 'Arial', sans-serif; color: #2c3e50;">OrtaVM</h2>
<h4 style="text-align: center; margin: 0px; font-family: 'Arial', sans-serif; color: #7f8c8d;">A Powerful Virtual Machine for Orta Language</h4>
<div style="height: 30px;" aria-label="Padding"></div>

<hr style="border-top: 2px solid #ecf0f1; margin: 20px 0;"/>

### Overview

OrtaVM is a simple yet powerful virtual machine designed to execute programs written in the Orta programming language. Orta compiles to bytecode, offering a fast and efficient execution environment. The goal of OrtaVM is to provide a lightweight and versatile virtual machine with a focus on performance, simplicity, and portability. Orta, as a minimalist language, emphasizes clarity and ease of use, while still supporting a variety of tasks. The VM uses a stack-based architecture for flexibility, making it suitable for both small and large projects. The key objectives and principles of OrtaVM are outlined at the top of `orta.h`.

---

### Features

- <span style="font-weight: bold; color: #3498db;">Cross-Platform</span>: Works on Windows, Linux, and macOS.
- <span style="font-weight: bold; color: #3498db;">High Performance</span>: Written in C for optimized and fast execution.
- <span style="font-weight: bold; color: #3498db;">Compilation</span>: Efficient bytecode generation.
- <span style="font-weight: bold; color: #3498db;">Good Error Handling</span>: Informative Error Messages
---

### Installation

You can either download precompiled binaries or compile from source:

1. **Download Precompiled Binaries** from the [Releases](https://github.com/zhrexx/OrtaVM/releases) section.
2. **Compile from Source**:
   ``` bash
   $ git clone https://github.com/zhrexx/OrtaVM.git
   $ cd OrtaVM
   # Linux
   $ ./tools/build.sh
   # Windows
   $ ./tools/build.cmd
   ```
