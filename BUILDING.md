## Building GBP for Linux

If you want to rebuild yourself the application binary **for Linux** and not use the provided AppImage binary in the "Releases" folder, here is the procedure to rebuild from scratch the AppImage from the sources :

### Step 1 : Install Ubuntu 22.04

On a new machine or VM, install the latest version of **UBUNTU 22.04** with 80 GB disk space on local File System. Then perform all the updates required by running the software updater.
For a VM, it is also required to configure your host OS to have a share folder with the guest system (to transfer gbp code and produced AppImage). See :
https://sysguides.com/share-files-between-kvm-host-and-linux-guest-using-virtiofs

### Step 2 : Install additional packages required

To prepare for later install of Qt Framework and building of gbp in Qt Creator, some additional packages are required. Run the following :
`sudo apt install build-essential libgl1-mesa-dev`
`sudo apt install libxcb-cursor0 libxcb-cursor-dev`
`sudo apt install cmake`  In QtCreator, make it the default instead of the one bundled with Qt.
`sudo apt install ninja-build`
In QtCreator, check "Projects panel → Build Settings → CMake → Generator". If it says "Unix Makefiles," switch to "Ninja". Delete build directory, restart QtCreator and rebuild.

> **Note on FUSE:** older versions of this guide had you install `libfuse2` here to be able to *run* AppImages. That's no longer needed — the AppImages we produce (see Step 6) use a bundled static runtime that doesn't depend on `libfuse2`/`libfuse3` at all. It only needs the `fusermount` helper binary, which ships by default as part of Ubuntu's `fuse3` package on 22.04, 24.04, and 26.04 — nothing to install.

### Step 3 : Install Qt

Download and run the **Qt online installer**. Try this link : https://www.qt.io/download-open-source (look at "Download Qt for open source use")
Install Qt in default directory ($HOME). The components to install are :

* Qt 6.10.3 and all the sub items, except Android and Web Assembly stuff
* Qt Creator 20.x
* CMake
* Ninja
  Test QtCreator by creating dummy widget app and verify that everything works.

### Step 4 : Install gbp source and tools

- Create the directory "$HOME/data/dev/Qt/gbp"
- Copy gbp source, doc and tools (https://github.com/redmoon1945/gbp) in $HOME/data/dev/Qt/gbp. After the copy operation, the "gbp" directory should contain 3 subdirectories : "src", "doc" and "build-tools"

### Step 5 : Rebuild gbp in Qt Creator

In QtCreator, load the project (src/CmakeLists.txt). You will probably have to click "Configure" right after the load. Do a “rebuild” with the “Release” kit selected and make sure it is 100% successful. In Qt Creator, run and check that the application seems to work fine.

### Step 6 : Create the AppImage

The last step is to create an independent AppImage bundle from the executable produced by QtCreator. For that, we use the FOSS application “linuxdeploy”, along with a custom static AppImage runtime (instead of linuxdeploy's default one) so the resulting AppImage doesn't depend on the host having `libfuse2` installed. For more info, see :

- https://github.com/linuxdeploy/linuxdeploy
- https://github.com/linuxdeploy/linuxdeploy-plugin-qt
- https://github.com/AppImage/type2-runtime (source of the static runtime binary)

#### A)

Download these files:

- "linuxdeploy-plugin-qt-x86_64.AppImage" (from https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases)
- "linuxdeploy-x86_64.AppImage" (from https://github.com/linuxdeploy/linuxdeploy/releases)
- "runtime-x86_64" — the static AppImage runtime, from https://github.com/AppImage/type2-runtime/releases (pick the `runtime-x86_64` asset for the appropriate release)

Set the permissions to "execute" on the two `.AppImage` files (e.g., `chmod a+x lin*`). `runtime-x86_64` does not need the execute bit itself — `create_appimage.sh` only reads it as an input file.

#### B)

Create a directory where the gbp AppImage will be built (e.g. ~/gbp-app-image-build). In this guide, we will call this directory GBP_APP_IMAGE_BUILD_DIR. Put there the following :

- the 3 files downloaded previously (`linuxdeploy-x86_64.AppImage`, `linuxdeploy-plugin-qt-x86_64.AppImage`, `runtime-x86_64`)
- From the QtCreator Build directory $HOME/data/dev/Qt/build/gbp/Desktop_Qt_6_10_3-Release
  - the compiled binary file "gbp" : this is the executable produced by QtCreator after a rebuild
  - the "gbp_en.qm" and "gbp_fr.qm" translation files
- From the "$HOME/data/dev/Qt/gbp/build-tools" directory :
  - the file "create_appimage.sh"
  - the app icon "gbp.png"
  - the app Desktop shortcut "gbp.desktop"

#### C)

Open a terminal and change current directory to GBP_APP_IMAGE_BUILD_DIR. Run the following shell script :
`./create_appimage.sh`

The script points `linuxdeploy`/`linuxdeploy-plugin-appimage` at the `runtime-x86_64` file in the current directory via the `LDAI_RUNTIME_FILE` environment variable, bundles the Qt libraries/plugins, and produces the final AppImage.

**The resulting executable "graphical-budget-planner-x86_64.AppImage" is now in the directory GBP_APP_IMAGE_BUILD_DIR** . Rename as you wish. Set permission to “execute”. This is your gbp executable file for Linux.

### Appendix : the AppImage runtime and FUSE, in detail

This section explains what the `runtime-x86_64` file actually is, why we use it instead of linuxdeploy's default, and what that means for anyone trying to run the resulting `gbp` AppImage.

#### Background : what "the runtime" even is

Every AppImage file is really two things glued together:

1. A small **runtime** stub — a compiled ELF binary a few hundred KB in size, stuck at the very front of the file.
2. A **squashfs filesystem image** appended after it, containing the actual application (in our case: `gbp`, its Qt libraries, plugins, translations, icon, desktop file).

When you double-click or execute an `.AppImage` file, the kernel runs the runtime stub. The stub's job is to make the squashfs payload accessible as a normal directory tree and then launch the app inside it (`AppRun`). Historically it did this by **mounting** the squashfs image via FUSE, which is why AppImages have always had a FUSE dependency of some kind.

#### The two runtime families

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

#### The `fusermount` vs `fusermount3` naming trap

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

#### How the build script wires this in

`create_appimage.sh` sets:

```
export LDAI_RUNTIME_FILE=$(pwd)/runtime-x86_64
```

`LDAI_RUNTIME_FILE` is an environment variable read by `linuxdeploy-plugin-appimage` (the internal plugin `linuxdeploy-x86_64.AppImage` calls to actually assemble the final `.AppImage` file). When set, it tells the plugin to splice in *our* `runtime-x86_64` file as the runtime stub instead of downloading/using its own default classic runtime. This is the entire mechanism by which our AppImages end up using the static runtime instead of the FUSE2-dependent one.

#### Where to get `runtime-x86_64`

Download the `runtime-x86_64` asset from the official AppImage project's runtime releases:
https://github.com/AppImage/type2-runtime/releases

Pick a recent release and grab the `runtime-x86_64` asset (there are also `runtime-i686`, `runtime-aarch64`, etc. for other architectures — we only need `runtime-x86_64` for our builds). Place it in `GBP_APP_IMAGE_BUILD_DIR` alongside the other build inputs (see Step 6B above) before running `create_appimage.sh`.

> Historical note: an older, closely related source for the same kind of static runtime is probonopd's `go-appimage` project. If you ever find a `runtime-x86_64` file of unknown origin lying around (e.g. inherited from a previous build environment), you can check which project produced it and roughly confirm it's the static type before trusting it:
> 
> ```
> strings runtime-x86_64 | grep -i 'libfuse\.so'
> ```
> 
> No output = static runtime (good, this is what we want). Any `libfuse.so.2` / `libfuse.so.3` hit = it's a classic dynamically-linked runtime, and using it would reintroduce the `libfuse2`/`libfuse2t64` host dependency we're trying to avoid.

#### Quick diagnostic checklist (for troubleshooting a built AppImage)

1. **Confirm it's the static runtime type**: `strings gbp-*.AppImage | grep -i 'libfuse\.so'` → expect no output.

2. **Confirm `fusermount` exists on the target machine**: `which fusermount`.

3. **If it's missing**, symlink it: `sudo ln -s /usr/bin/fusermount3 /usr/bin/fusermount`.

4. **Universal fallback**, works regardless of FUSE setup on any AppImage: extract and run directly, bypassing FUSE mounting entirely:
   
   ```
   ./gbp-x86_64.AppImage --appimage-extract
   ./squashfs-root/AppRun
   ```
