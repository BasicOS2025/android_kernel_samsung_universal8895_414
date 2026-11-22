# Exynos 8895 Android 17 / Linux 4.14 development

Experimental hardware bring-up, not a complete Android 17 kernel or flashable release.
The ROM built with the 4.4 kernel remains separate.

Upstream: https://github.com/LumiPlayground/android_kernel_samsung_universal8895_414
Pinned donor: a7432fa4d30426fdd45bf0c0c03d3a930056fa90 (8895-rebase).
Original source history and licensing are retained by this GitHub fork.
Actual source release: Linux 4.14.113; no version string spoof is added here.
The donor reports boot/pstore support, not a fully working Android device.

Initial changes:
- Fix the duplicated S8+ DTB entry so S8, S8+ and Note8 DTBs are selected.
- Add an Android bring-up defconfig with explicit Binder devices, XZ initrd,
  swap/ZRAM, BPF, cgroups and seccomp settings.
- Add a manually triggered Image/DTB build preserving failure logs.

The donor configuration uses simple framebuffer, early console and maxcpus=4.
These debugging settings are retained until hardware logs justify switching
to full display and CPU/power paths. This is not a production configuration.
KernelSU Next has not been integrated into this 4.14 tree.

Required before connecting this tree to the ROM build:
1. Pass Image/all three DTB builds and check the effective .config.
2. Establish reproducible S8 recovery/initramfs boot with pstore logs.
3. Adapt/test UFS, PMIC, clocks, CPU/power, ION/dma-buf, Mali/DECON,
   camera, audio, modem and Wi-Fi with the pinned vendor binaries.
4. Validate Android init/cgroups/Binder/SELinux, the actual BPF loader,
   networking and tethering. Linux 4.14 alone does not guarantee Android 17.
5. Integrate KernelSU after a working baseline, then package ROM boot images.

No physical boot or driver validation is claimed by this branch.

## Galaxy S8 early-boot logs

The EUR Galaxy S8 (dreamlte, rev10 and later in this tree) reserves the
same 32 KiB ramoops region as the upstream dreamlte device tree. The
region starts at 0x92000000 and provides 16 KiB each for the panic
record and kernel console. Its compiled range fits inside board RAM
and does not overlap the other declared reservations. The S8+ and
Note8 device trees do not inherit this board-specific reservation.

On a warm reboot into a recovery that maps the same ramoops region,
mount pstore if needed and collect the console and crash records:

```sh
adb shell 'mountpoint -q /sys/fs/pstore || mount -t pstore pstore /sys/fs/pstore'
adb pull /sys/fs/pstore dreamlte-pstore
```

The bootloader must preserve the RAM contents across the reset.
Registering the backend and recovering records on physical hardware
remain to be tested.

Upstream board DTS: https://linux.googlesource.com/linux/kernel/git/torvalds/linux/+/7711f4417f1870e4bfbee1a7d501f789255bc7aa/arch/arm64/boot/dts/exynos/exynos8895-dreamlte.dts

## Android 17 integration status

The current LineageOS 24 ROM manifest still pins the separate Linux 4.4
kernel. The dreamlte device tree uses a device-specific defconfig and
Samsung boot packaging with a separate DTB. An Image and DTB build from
this repository therefore does not exercise the existing ROM build. The
most recent ROM workflow run inspected (44509323395) stopped during
source checkout at `platform/packages/services/display_safety`, before
ROM compilation. The available diagnostics identify the failing checkout
but give no underlying Git error.

The AOSP kernel compatibility matrix supports android17-6.18. It does
not list Linux 4.14. The android17-6.18 branch includes a dreamlte board
DTS with a basic framebuffer and the same ramoops region. Its placeholder
PMIC supplies are still present, so the board DTS is an early bring-up
starting point. The pinned Galaxy S8 DTS compiles to a DTB, with duplicate
unit-address warnings for the USI peripheral modes. Storage, power,
vendor HAL integration and the device's boot packaging require their
own validation.

Before claiming a working port, build the selected ROM against the intended
kernel and boot it on dreamlte. Verify Android reaches sys.boot_completed
with SELinux enforcing, /data mounts and decrypts, the network BPF loader
loads and pins its programs, and the display, touch, storage, power,
Wi-Fi, audio, camera and telephony work on the target device. Confirm
S8+ and Note8 independently. Full AOSP compatibility additionally
requires the applicable VINTF, CTS and VTS checks.

- [ROM source checkout failure](https://github.com/BasicOS2025/universal8895-lineage24-build/actions/runs/44509323395)
- [ROM manifest](https://github.com/BasicOS2025/universal8895-lineage24-build/blob/main/manifests/dreamlte.xml)
- [AOSP Android Common Kernel compatibility matrix](https://source.android.com/docs/core/architecture/kernel/android-common)
- [Pinned android17-6.18 dreamlte DTS](https://android.googlesource.com/kernel/common/+/76bdb627cab87dfa1d901810e22ee7fae6d3dc89/arch/arm64/boot/dts/exynos/exynos8895-dreamlte.dts)
