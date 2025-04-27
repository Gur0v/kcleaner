# 🌱 kcleaner
A powerful utility for Linux kernel management, offering an improved alternative to Gentoo's eclean-kernel.

---
## 🔍 Overview
`kcleaner` helps maintain a clean and efficient system by simplifying the management of installed Linux kernels. It provides an intuitive interface for listing, selectively removing, or automatically cleaning old kernel versions, helping prevent `/boot` partition overflow while ensuring system bootability.

---
## ✨ Features
- **Smart Listing** 📋: View all installed kernels with numbered indices, sizes, and running status.
- **Selective Cleaning** 🎯: Remove specific kernels using individual numbers or ranges.
- **Auto-Clean Mode** 🤖: Automatically keep only the latest and currently running kernels.
- **Safety First** 🛡️: Built-in safeguards prevent creating unbootable systems.

---
## 🚀 Quick Start
### Installation
1. Clone the repository:
   ```bash
   git clone --depth=1 https://github.com/username/kcleaner
   cd kcleaner
   ```
2. Build the project:
   ```bash
   cc -march=native -O3 -pipe kcleaner.c -o kcleaner
   ```
3. Install to your system:
   ```bash
   sudo install -Dm 755 kcleaner -t /usr/bin
   ```

---
## 📚 Usage
### Listing Installed Kernels 📋
```bash
kcleaner -l
```
This displays all installed kernels with:
- Sequential numbers for reference
- Version information
- Size on disk
- Running status indicator
---
### Deleting Specific Kernels 🗑️
```bash
sudo kcleaner -d INDEX[,INDEX,RANGE]
```
#### Examples:
```bash
# Delete kernels #2 and #4
sudo kcleaner -d 2,4

# Delete kernels #1 through #3, plus #5 and #8 through #10
sudo kcleaner -d 1-3,5,8-10
```

---
### Auto-Clean Mode 🧠
```bash
sudo kcleaner -a
```
This intelligent mode:
- Preserves your currently running kernel
- Keeps the latest kernel version
- Removes all other kernels
- Perfect for routine system maintenance

---
### Help Information ℹ️
```bash
kcleaner
```
Shows usage information and available options.

---
## 🛡️ Safety Features
- **Running Kernel Protection**: Warns before removing your active kernel
- **Last Kernel Protection**: Warns when attempting to delete all kernels
- **Permission Verification**: Ensures root privileges for deletion operations
- **Confirmation Prompts**: Requires explicit confirmation before any deletion

---
## 🔧 How It Works
`kcleaner` identifies kernel files in `/boot` (prefixed with "vmlinuz-") and their corresponding modules in `/lib/modules/`. For kernel deletion, it removes both the kernel files and modules, preserving system integrity while freeing up valuable disk space.

The auto-clean feature uses version sorting to identify the latest kernel and preserves both it and the running kernel (if different), ensuring system stability while minimizing disk usage.

---
## 📜 License
This project is licensed under the [GPL-3.0 License.](LICENSE)
