## Building GBP for Linux

If you want to rebuild yourself the application binary **for Linux** and not use the provided AppImage binary in the "Releases" folder, here is the procedure to rebuild from scratch the AppImage from the sources. Once everything is installed and prepared (Steps 1 to 5), **a single command compiles gbp and builds the 2 AppImages** (Step 6, see "Two versions of GBP for Linux" below) :
`./gbp_linux_compile_and_deploy.sh x.y.z`

### Two versions of GBP for Linux

Two AppImages can be built from the same sources. They differ only by the documentation :

| Version                      | Command to build only this one (Step 6)            | Build directory (`BUILD_DIR`)                          | AppImage produced                    |
| ---------------------------- | -------------------------------------------------- | ------------------------------------------------------ | ------------------------------------ |
| **Embedded manuals version** | `./gbp_linux_compile_and_deploy.sh x.y.z manual`   | `…/build/gbp/Desktop_Qt_<qt_version>-Release`          | `gbp-x.y.z-x86_64.AppImage`          |
| **Online manuals version**   | `./gbp_linux_compile_and_deploy.sh x.y.z nomanual` | `…/build/gbp/Desktop_Qt_<qt_version>-Release-nomanual` | `gbp-x.y.z-nomanual-x86_64.AppImage` |

**By default, without `manual` or `nomanual`, `./gbp_linux_compile_and_deploy.sh x.y.z` builds both versions** (see Step 6).

- **Embedded manuals version** : the User Manual, the Quick Tutorial (PDF, English and French) and the Change Log (PDF, English) are inside the executable. Help → User Manual, Help → Quick Tutorial and Help → Change log open them in the PDF viewer.
- **Online manuals version** : for when disk space is critical. Much smaller, it contains none of these PDFs. Help → User Manual, Help → Quick Tutorial and Help → Change log only show the address where the PDF can be downloaded on GitHub, with a button to copy it : the user downloads the PDF himself. **GBP never accesses the Internet.** The addresses are `https://github.com/redmoon1945/gbp/raw/master/doc/<file>`, where `<file>` is `user_manual-en.pdf`, `user_manual-fr.pdf`, `quick_tutorial-en.pdf`, `quick_tutorial-fr.pdf` or `changelog-en.pdf`.

In both versions, the Welcome document (PDF, English and French, "src/resources/welcome-xx.pdf", about 60 KB each) is inside the executable (src/gbp_welcome.qrc) and opens in the PDF viewer at the first start.

The choice is made at compile time by the CMake option **`GBP_EMBEDDED_MANUALS`** : `ON` (the default) for the Embedded manuals version, `OFF` for the Online manuals version. The scripts set it for you ; in Qt Creator, see "Building in Qt Creator" at the end of Step 6. Each version has its own build directory, so building one never erases the other.

**When publishing the Online manuals version** : the 5 PDF files above must be in the "doc" directory of the branch "master" on GitHub, with these exact names (they are copies of the ones in "src/resources"). The addresses do not contain the version : they always give the PDF currently on "master", so update these files at each release.

### Step 1 : Install Ubuntu 22.04

On a new machine or VM, install the latest version of **UBUNTU 22.04** with 80 GB disk space on local File System. Then perform all the updates required by running the software updater.
For a VM, it is also required to configure your host OS to have a share folder with the guest system (to transfer gbp code and produced AppImage). See :
https://sysguides.com/share-files-between-kvm-host-and-linux-guest-using-virtiofs

### Step 2 : Install additional packages required

To prepare for later install of Qt Framework and building of gbp, some additional packages are required (compiler, OpenGL and XCB libraries, CMake and Ninja). Run the following :
`sudo apt install build-essential libgl1-mesa-dev`
`sudo apt install libxcb-cursor0 libxcb-cursor-dev`
`sudo apt install cmake ninja-build`

> **Note on FUSE:** older versions of this guide had you install `libfuse2` here to be able to *run* AppImages. That's no longer needed — the AppImages we produce (see Step 6) use a bundled static runtime that doesn't depend on `libfuse2`/`libfuse3` at all. It only needs the `fusermount` helper binary, which ships by default as part of Ubuntu's `fuse3` package on 22.04, 24.04, and 26.04 — nothing to install.

### Step 3 : Install Qt

Download and run the **Qt online installer**. Try this link : https://www.qt.io/download-open-source (look at "Download Qt for open source use")
Install Qt in default directory ($HOME/Qt). In this guide, `<qt-version>` is the Qt version installed (e.g. 6.11.3). The component **required** to build gbp is :

* Qt `<qt-version>` : **Desktop (gcc 64-bit)** and all its sub items (Additional Libraries, ...), except Android and Web Assembly stuff

The scripts use Qt from `$HOME/Qt/<qt-version>/gcc_64`, and CMake and Ninja from Step 2 (CMake and Ninja installed with Qt are used instead if present).

### Step 4 : Install gbp source and tools

- Create the directory "$HOME/data/dev/Qt/gbp"
- Copy gbp source, doc and tools (https://github.com/redmoon1945/gbp) in $HOME/data/dev/Qt/gbp. After the copy operation, the "gbp" directory should contain 3 subdirectories : "src", "doc" and "build-tools"

### Step 5 : Prepare the AppImage build directory (once)

Create the directory where the scripts are run and the AppImage is built : `$HOME/gbp-app-image-build` (the default of the scripts). In this guide, we will call this directory GBP_APP_IMAGE_BUILD_DIR.

#### A) The 4 downloaded tools (required)

The AppImage is prepared with the FOSS application “linuxdeploy”, then packed by “appimagetool” with a custom static AppImage runtime (instead of the default one) so the resulting AppImage doesn't depend on the host having `libfuse2` installed. **Download these 4 files into GBP_APP_IMAGE_BUILD_DIR** : they **must** be there, the scripts look for them in this directory only.

| File                                           | Download from                                                                                                    | Execute permission              |
| ---------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- | ------------------------------- |
| `linuxdeploy-x86_64.AppImage`                  | https://github.com/linuxdeploy/linuxdeploy/releases                                                              | yes                             |
| `linuxdeploy-plugin-qt-x86_64.AppImage`        | https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases                                                    | yes                             |
| `appimagetool-x86_64.AppImage`                 | https://github.com/AppImage/appimagetool/releases (release "continuous")                                         | yes                             |
| `runtime-x86_64` (the static AppImage runtime) | https://github.com/AppImage/type2-runtime/releases (pick the `runtime-x86_64` asset for the appropriate release) | no (only read as an input file) |

Set the permissions to "execute" on the three `.AppImage` files (e.g., `chmod a+x *.AppImage`). For more info, see the Appendix at the end of this document.

#### B) The 3 scripts (required)

Copy into GBP_APP_IMAGE_BUILD_DIR the 3 scripts found in "$HOME/data/dev/Qt/gbp/build-tools/Linux" :

| Script                                                        | What it does                                                                                     |
| ------------------------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| `gbp_linux_compile_and_deploy.sh x.y.z [manual \| nomanual]` | The command to run (Step 6) : runs the 2 other scripts, for both versions (default) or only one |
| `gbp_linux_compile.sh [nomanual]`                             | Compiles gbp in "Release", one version                                                           |
| `gbp_linux_deploy.sh x.y.z [nomanual]`                        | Builds the AppImage of one version                                                               |

"gbp_linux_compile_and_deploy.sh" builds both versions, unless `manual` (only the Embedded manuals version) or `nomanual` (only the Online manuals version) is given. The 2 other scripts build one version : the Embedded manuals version without argument, the Online manuals version with `nomanual` (see "Two versions of GBP for Linux" above).

They must stay together. They are not overwritten when the "gbp" directory is replaced by new sources : if the scripts in "build-tools/Linux" change, copy them again.

Nothing else needs to be copied there : the scripts take the other files themselves.

#### C) Check the settings of the scripts

The scripts use these directories. If yours are different, change them at the top of the scripts :

| Variable    | In script            | Default                                                       | Meaning                                                                                                          |
| ----------- | -------------------- | ------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| `QT_DIR`    | gbp_linux_compile.sh | `$HOME/Qt/<qt-version>/gcc_64`                                | Qt installed in Step 3 (`QT_VERSION` = 6.11.3 by default)                                                        |
| `QT_PATH`   | gbp_linux_deploy.sh  | `$HOME/Qt/<qt-version>/gcc_64`                                | The same                                                                                                         |
| `SRC_DIR`   | gbp_linux_compile.sh | `$HOME/data/dev/Qt/gbp/src`                                   | gbp sources of Step 4                                                                                            |
| `REPO_DIR`  | gbp_linux_deploy.sh  | `$HOME/data/dev/Qt/gbp`                                       | The same, one level up                                                                                           |
| `BUILD_DIR` | both                 | `$HOME/data/dev/Qt/build/gbp/Desktop_Qt_<qt_version>-Release` | Build directory, where `<qt_version>` is `<qt-version>` with "_" instead of "." (e.g. Desktop_Qt_6_11_3-Release). With `nomanual`, "-nomanual" is added at the end (e.g. Desktop_Qt_6_11_3-Release-nomanual) |
| `WORK_DIR`  | gbp_linux_deploy.sh  | `$HOME/gbp-app-image-build`                                   | GBP_APP_IMAGE_BUILD_DIR                                                                                          |

### Step 6 : Compile and create the AppImages

Open a terminal, go to GBP_APP_IMAGE_BUILD_DIR and run, passing the gbp version (e.g. 1.9.0) :
`./gbp_linux_compile_and_deploy.sh 1.9.0`

It builds **both versions** : first the Embedded manuals version (compilation, then AppImage), then the Online manuals version, each in its own build directory. The script stops with an error message as soon as something fails, and prints "Release 1.9.0 done" then "Release 1.9.0 nomanual done" otherwise. If the Online manuals version fails, the AppImage of the Embedded manuals version is already built and stays there. For each version, it :

1. checks the version (mandatory, in the form x.y.z) before anything else, so that a mistake does not waste a compilation
2. **compiles gbp** in "Release" with CMake and Ninja (script "gbp_linux_compile.sh") : always a full rebuild, from an empty build directory, with all the processor cores. This produces in `BUILD_DIR` the executable "graphical-budget-planner" and the "gbp_en.qm" and "gbp_fr.qm" translation files, and updates the translation files "gbp_en.ts" and "gbp_fr.ts" in "src"
3. **only if the compilation succeeded, builds the AppImage** (script "gbp_linux_deploy.sh") :
   - checks that every file needed is there : the executable and the translation files (from `BUILD_DIR`), the app Desktop shortcut "build-tools/Linux/graphical-budget-planner.desktop" and the app icon "build-tools/graphical-budget-planner.png" (from `REPO_DIR`), and the 4 downloaded tools of Step 5 A (from GBP_APP_IMAGE_BUILD_DIR)
   - runs linuxdeploy, which bundles the executable with the Qt libraries and plugins (from `QT_PATH`), then trims the plugins and libraries GBP does not use. Of Qt's own translations, only English and French (the 2 languages of GBP) are kept
   - runs appimagetool, which packs everything with the `runtime-x86_64` file into the final AppImage. The content is compressed with zstd at level 19 in blocks of 1 MB, which makes the AppImage about 3 MB smaller than with the default settings (level not set, blocks of 128 KB), without slowing down the start of GBP. linuxdeploy can also produce the AppImage (it calls appimagetool itself), but it does not allow to change these settings. For more info, see https://github.com/linuxdeploy/linuxdeploy, https://github.com/linuxdeploy/linuxdeploy-plugin-qt, https://github.com/AppImage/appimagetool and https://github.com/AppImage/type2-runtime (source of the static runtime binary)
   - deletes the working directory "AppDir" (the content of the AppImage before packing), no longer needed. It is kept if the AppImage could not be created, so that it can be inspected

For the Online manuals version, gbp is compiled with `GBP_EMBEDDED_MANUALS=OFF`, in the build directory `BUILD_DIR` followed by "-nomanual".

**The resulting executables "gbp-1.9.0-x86_64.AppImage" (Embedded manuals version) and "gbp-1.9.0-nomanual-x86_64.AppImage" (Online manuals version), named after the version passed to the script, are now in the directory GBP_APP_IMAGE_BUILD_DIR** . Set permission to “execute”. These are your gbp executable files for Linux.

#### Building only one version

Building both versions takes about twice as long as one. To build only one, add `manual` or `nomanual` at the end of the command :

| Command                                            | AppImage produced                         |
| -------------------------------------------------- | ----------------------------------------- |
| `./gbp_linux_compile_and_deploy.sh 1.9.0 manual`   | "gbp-1.9.0-x86_64.AppImage" only          |
| `./gbp_linux_compile_and_deploy.sh 1.9.0 nomanual` | "gbp-1.9.0-nomanual-x86_64.AppImage" only |

#### Running the 2 steps separately

The 2 scripts run by "gbp_linux_compile_and_deploy.sh" can also be run one after the other, e.g. to rebuild only the AppImage :

| Version                  | 1. Compile                         | 2. Build the AppImage                      |
| ------------------------ | ---------------------------------- | ------------------------------------------ |
| Embedded manuals version | `./gbp_linux_compile.sh`           | `./gbp_linux_deploy.sh 1.9.0`              |
| Online manuals version   | `./gbp_linux_compile.sh nomanual`  | `./gbp_linux_deploy.sh 1.9.0 nomanual`     |

"gbp_linux_deploy.sh" refuses to use the build of the other version : it checks `GBP_EMBEDDED_MANUALS` in the file `CMakeCache.txt` of the build directory, and stops with an error asking to run the right compilation first.

#### Building in Qt Creator

**Qt Creator is for development and testing only : never use it to build the AppImages of a release.** Always build them with `gbp_linux_compile_and_deploy.sh` (see above) : it compiles gbp from scratch in "Release", with the right options, then builds the AppImage from that compilation. An executable compiled in Qt Creator may come from a stale build directory or other settings (e.g. "Debug", other CMake options), and would end up in the AppImage as is.

The scripts are not needed to compile gbp in Qt Creator. As `GBP_EMBEDDED_MANUALS` is `ON` by default, **the usual configurations ("Debug", "Release") build the Embedded manuals version**, nothing to change.

To test the **Online manuals version** in Qt Creator, create once a second configuration :

1. Open the gbp project, click **Projects** in the left bar, then **Build** under the kit "Desktop Qt <qt-version>"
2. At the top, in **Edit build configuration**, select **Release**, then click **Add** → **Clone Selected**, and name the new configuration `Release-nomanual`
3. In **Build directory**, put a directory of its own (e.g. `$HOME/data/dev/Qt/build/gbp/Desktop_Qt_6_11_3-Release-nomanual-qtc`), not the one used by the scripts
4. In the **CMake** section, tab **Current Configuration**, find **GBP_EMBEDDED_MANUALS** and uncheck it (if it is not listed, run **Build → Run CMake** once first)
5. Click **Run CMake** (or **Apply Configuration Changes**, depending on the Qt Creator version)

Then, choose the version to build with the configuration selector at the bottom left (the computer icon above the "Run" button) : **Release** for the Embedded manuals version, **Release-nomanual** for the Online manuals version. The CMake output (pane "General Messages") shows which one is built : `GBP_EMBEDDED_MANUALS : ON` or `GBP_EMBEDDED_MANUALS : OFF`.

## Building GBP for Windows

If you want to rebuild yourself the application **for Windows®** and not use the provided zip file in the "Releases" folder, here is the procedure to rebuild from scratch the zip file from the sources. Once everything is installed and configured (Steps 1 to 4), **a single command builds the zip file** (Step 5) :
`gbp_windows_compile_and_deploy.bat x.y.z`

On Windows, only the **Embedded manuals version** is built (the User Manual, the Quick Tutorial and the Welcome document are inside the executable) : the Online manuals version (see "Two versions of GBP for Linux") is not available with the Windows scripts.

### Step 1 : Install Windows 11

GBP is built and tested on **Windows® 11 Home Edition**. Perform all the updates with Windows Update.

### Step 2 : Install Qt

Download and run the **Qt online installer**. Try this link : https://www.qt.io/download-open-source (look at "Download Qt for open source use")
Install Qt in the default directory (C:\Qt). In this guide, `<qt-version>` is the Qt version installed (e.g. 6.11.3). The components **required** to build gbp are :

* Qt `<qt-version>` : **MinGW 64-bit** and **Additional Libraries** (Qt Charts, ...). Android and Web Assembly stuff are not needed
* In "Build Tools" : the latest **MinGW 64-bit** toolchain, **CMake** and **Ninja**

The scripts use only these tools, at these locations (default installation directory). `<qt-version>` must also be set in the variable `QT_VERSION` at the top of the scripts (see Step 4) :

| Tool                           | Location                                                                                                                     | Used by                 |
| ------------------------------ | ---------------------------------------------------------------------------------------------------------------------------- | ----------------------- |
| Qt for MinGW 64-bit            | `C:\Qt\<qt-version>\mingw_64`                                                                                                | gbp_windows_compile.bat |
| MinGW 64-bit toolchain (g++)   | `C:\Qt\Tools\mingw*_64` (the folder name contains the MinGW version, e.g. mingw1310_64 : the script takes the one installed) | gbp_windows_compile.bat |
| CMake                          | `C:\Qt\Tools\CMake_64\bin`                                                                                                   | gbp_windows_compile.bat |
| Ninja                          | `C:\Qt\Tools\Ninja`                                                                                                          | gbp_windows_compile.bat |
| windeployqt                    | `C:\Qt\<qt-version>\mingw_64\bin`                                                                                            | gbp_windows_deploy.bat  |
| PowerShell (to create the zip) | included in Windows                                                                                                          | gbp_windows_deploy.bat  |

### Step 3 : Install gbp source and tools

- Create the directory `C:\Users\<user>\data\dev\Qt\gbp`
- Copy gbp source, doc and tools (https://github.com/redmoon1945/gbp) in this directory. After the copy operation, the "gbp" directory should contain 3 subdirectories : "src", "doc" and "build-tools"

### Step 4 : Configure the build scripts (once)

The build is done by 3 scripts, found in "build-tools\Windows". Copy them just above the "gbp" directory (e.g. `C:\Users\<user>\data\dev\Qt`) : this way, they are not overwritten when the "gbp" directory is replaced by new sources, and the command of Step 5 is always run from the same place. They must stay together (their paths are absolute, so the directory they are in does not matter otherwise). If the scripts in "build-tools\Windows" change, copy them again.

Then open 2 of them in a text editor and check the variables at the top. They must match your computer (replace "renoir" by your user name) :

- In "gbp_windows_compile.bat" :
  - `QT_VERSION` : `<qt-version>`, the Qt version installed in Step 2 (e.g. 6.11.3)
  - `SRC_DIR` : the "src" directory of Step 3 (e.g. `C:\Users\<user>\data\dev\Qt\gbp\src`)
- In "gbp_windows_deploy.bat" :
  - `QT_VERSION` : the same as above
  - `BUILD_DIR` : the build directory, that is `<SRC_DIR>\build\Desktop_Qt_<qt_version>_MinGW_64_bit_Release`, where `<qt_version>` is `<qt-version>` with "_" instead of "." (e.g. Desktop_Qt_6_11_3_MinGW_64_bit_Release). It is the one created by "gbp_windows_compile.bat"
  - `DEPLOY_DIR` : a temporary working directory where GBP is deployed before being zipped (e.g. `C:\Users\<user>\data\dev\deploy-gbp`). **Warning : it is deleted at each run** : do not keep anything in it. It must not be the directory of the scripts.

### Step 5 : Compile and deploy with zip file

Open a Command Prompt, go to the directory of the scripts (Step 4) and run, passing the gbp version (e.g. 1.9.0) :
`gbp_windows_compile_and_deploy.bat 1.9.0`

The script stops with an error message as soon as something fails, and ends with "Release 1.9.0 done" otherwise. It :

1. checks the version (mandatory, in the form x.y.z) before anything else, so that a mistake does not waste a long compilation
2. **compiles gbp** in "Release" (script "gbp_windows_compile.bat") : always a full rebuild, from an empty build directory. It uses the MinGW toolchain found under "C:\Qt\Tools" (whatever its version), CMake and Ninja, and all the processor cores. This produces "graphical-budget-planner.exe" and the "gbp_en.qm" and "gbp_fr.qm" translation files in the build directory, and updates the translation files "gbp_en.ts" and "gbp_fr.ts" in "src"
3. **only if the compilation succeeded, deploys and zips gbp** (script "gbp_windows_deploy.bat") :
   - deletes and recreates `DEPLOY_DIR`, so that no file of a previous deployment ends up in the zip
   - runs windeployqt, which copies the Qt libraries, plugins and MinGW runtime into `DEPLOY_DIR`\gbp-1.9.0 (see https://doc.qt.io/qt-6/windows-deployment.html)
   - copies "graphical-budget-planner.exe" there, renamed **"gbp.exe"** (the name used on Windows), and the translation files "gbp_*.qm" (windeployqt copies neither)
   - zips the folder gbp-1.9.0 with PowerShell, in the directory of the scripts
   - deletes `DEPLOY_DIR`, no longer needed

**The resulting file "gbp-1.9.0-windows-x86_64.zip" (named after the version passed to the script) is now in the directory of the scripts** (e.g. `C:\Users\<user>\data\dev\Qt`), and replaces a previous zip of the same version. It contains the folder "gbp-1.9.0" : unzip it anywhere and run "gbp.exe" inside. This is your gbp application for Windows.

## Appendix (Linux) : the AppImage runtime and FUSE, in detail

This section explains what the `runtime-x86_64` file actually is, why we use it instead of linuxdeploy's default, and what that means for anyone trying to run the resulting `gbp` AppImage.

### Background : what "the runtime" even is

Every AppImage file is really two things glued together:

1. A small **runtime** stub — a compiled ELF binary a few hundred KB in size, stuck at the very front of the file.
2. A **squashfs filesystem image** appended after it, containing the actual application (in our case: `gbp`, its Qt libraries, plugins, translations, icon, desktop file).

When you double-click or execute an `.AppImage` file, the kernel runs the runtime stub. The stub's job is to make the squashfs payload accessible as a normal directory tree and then launch the app inside it (`AppRun`). Historically it did this by **mounting** the squashfs image via FUSE, which is why AppImages have always had a FUSE dependency of some kind.

### The two runtime families

**1. Classic runtime (AppImageKit / old default)**

- Dynamically links against `libfuse.so.2` (FUSE **2**, the old API) at startup, via `dlopen()`.
- If `libfuse.so.2` isn't present on the host, the AppImage fails immediately with an error like `dlopen(): error loading libfuse.so.2`.
- Since Ubuntu 22.04, `libfuse.so.2` is **not installed by default** anymore (only FUSE 3's `libfuse.so.3` is) — the package was renamed `libfuse2t64` from 24.04 onward. This is the classic "AppImages don't work out of the box on modern Ubuntu" complaint you'll see all over forums and GitHub issues.
- Fix for this family: `sudo apt install libfuse2` (22.04) or `sudo apt install libfuse2t64` (24.04/26.04).

**2. Static / "type2" runtime (what `runtime-x86_64` is)**

- Instead of dynamically loading a system FUSE library, this runtime has `libfuse` and `squashfuse` **statically compiled directly into the binary itself**. It never calls `dlopen()` on `libfuse.so.2` or `libfuse.so.3` at all.
- Consequence: the host does **not** need `libfuse2`, `libfuse2t64`, or `libfuse3` installed for the AppImage to run. This is exactly why we switched to it — it removes the single most common "AppImage won't start" problem for anyone using our AppImage.
- It still needs to actually *mount* the squashfs image at the kernel level, though, which means it still needs the **`fusermount` helper program** on `$PATH` (used to request the FUSE mount as a non-root user) and a working `/dev/fuse` device (present in every stock Linux kernel).
- We confirmed this by running `strings` on a built `gbp` AppImage: it contains internal `fuse_*` symbols and `fsname=squashfuse` / `subtype=squashfuse` markers (proof of the static linking), but **no** reference to `libfuse.so.2` or `libfuse.so.3` anywhere in the binary.

### The `fusermount` vs `fusermount3` naming trap

Modern distros ship FUSE 3's package (`fuse3`), whose helper binary is technically named `fusermount3`, not `fusermount`. Some non-Debian distros (e.g. Arch) only ship `fusermount3` with no compatibility name, which breaks any tool — including our AppImage — that specifically looks for a binary literally called `fusermount`.

**Good news for us:** Ubuntu's `fuse3` package installs **both** names by default:

```
/bin/fusermount
/bin/fusermount3
```

This is true on Ubuntu/Kubuntu 22.04, 24.04, and 26.04 out of the box — confirmed directly against Ubuntu's package file listings. So on any target machine running stock Ubuntu/Kubuntu, `fusermount` is already present and our AppImage should run with zero extra setup.

If someone ever runs the AppImage on a distro where only `fusermount3` exists, it will fail with:

```
Error: No suitable fusermount binary found on the $PATH
```

The fix is a one-line symlink:

```
sudo ln -s /usr/bin/fusermount3 /usr/bin/fusermount
```

### How the build script wires this in

`gbp_linux_deploy.sh` builds the final `.AppImage` file by calling appimagetool directly, with:

```
--runtime-file "$(pwd)/runtime-x86_64"
```

This option tells appimagetool to splice in *our* `runtime-x86_64` file as the runtime stub instead of downloading/using its own default runtime. This is the entire mechanism by which our AppImages end up using the static runtime instead of the FUSE2-dependent one. (Before version 1.3.0 of the script, the AppImage was built by linuxdeploy, and the runtime was given with the environment variable `LDAI_RUNTIME_FILE`, read by `linuxdeploy-plugin-appimage`, the internal plugin that calls appimagetool. That plugin does not allow to set the compression level and block size, hence the direct call.)

### Where to get `runtime-x86_64`

Download the `runtime-x86_64` asset from the official AppImage project's runtime releases:
https://github.com/AppImage/type2-runtime/releases

Pick a recent release and grab the `runtime-x86_64` asset (there are also `runtime-i686`, `runtime-aarch64`, etc. for other architectures — we only need `runtime-x86_64` for our builds). Place it in `GBP_APP_IMAGE_BUILD_DIR` alongside the two linuxdeploy tools (see Step 5 A of "Building GBP for Linux") before running `gbp_linux_deploy.sh`.

> Historical note: an older, closely related source for the same kind of static runtime is probonopd's `go-appimage` project. If you ever find a `runtime-x86_64` file of unknown origin lying around (e.g. inherited from a previous build environment), you can check which project produced it and roughly confirm it's the static type before trusting it:
>
> ```
> strings runtime-x86_64 | grep -i 'libfuse\.so'
> ```
>
> No output = static runtime (good, this is what we want). Any `libfuse.so.2` / `libfuse.so.3` hit = it's a classic dynamically-linked runtime, and using it would reintroduce the `libfuse2`/`libfuse2t64` host dependency we're trying to avoid.

### Quick diagnostic checklist (for troubleshooting a built AppImage)

1. **Confirm it's the static runtime type**: `strings gbp-*.AppImage | grep -i 'libfuse\.so'` → expect no output.

2. **Confirm `fusermount` exists on the target machine**: `which fusermount`.

3. **If it's missing**, symlink it: `sudo ln -s /usr/bin/fusermount3 /usr/bin/fusermount`.

4. **Universal fallback**, works regardless of FUSE setup on any AppImage: extract and run directly, bypassing FUSE mounting entirely:

   ```
   ./gbp-1.9.0-x86_64.AppImage --appimage-extract
   ./squashfs-root/AppRun
   ```
