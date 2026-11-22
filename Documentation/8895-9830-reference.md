# Pinned Android 17 reference comparison

Reference: LineageOS/android_kernel_samsung_universal9830, lineage-24.0,
97369af7bbd66581d3e993fea083f3eed02d3c20 (Linux 4.19.325).
URL: https://github.com/LineageOS/android_kernel_samsung_universal9830/tree/97369af7bbd66581d3e993fea083f3eed02d3c20

This does not turn Exynos 8895 into a 9830 device. No 9830 board drivers,
device trees or arbitrary subsystem directories are copied.

The first comparison found that the 8895 donor disabled F2FS, ext4/F2FS
file encryption, BPF traffic classifier/actions, and some cgroup controllers.
These are implemented in the existing 4.14 source and are now selected in
the Android configuration. The actual 8895 fstab has both ext4 and F2FS /data
entries with fileencryption=aes-256-xts; both filesystem encryption paths and
their key/XTS dependencies must therefore be available in the Image.
The donor also implements the f2fs fsync_mode=nobarrier mount option.

CI checks the generated .config and fails on silently dropped options.
Enabling features is not evidence of successful mounts or Android boot.

Remaining gaps include BinderFS, PSI, newer fscrypt policies and newer BPF
program/map/helper implementations. The 9830 tree contains significant
backports beyond its release number. Its full BPF directory cannot simply
replace 4.14's without adapting core networking, memory and cgroup APIs.
No kernel release override is introduced to hide those gaps.

Still required: real 8895 display/GPU/audio/camera/modem/Wi-Fi integration,
UFS mounting, matching vendor userspace, and physical boot/subsystem tests.
The original 4.4 ROM currently reboots from the Samsung logo into TWRP;
the user reported empty crash logs, so its root cause is unproven.

## BPF object metadata

The 4.14 kernel now accepts the upstream 4.15 map and program name fields
in BPF_MAP_CREATE and BPF_PROG_LOAD. BPF_OBJ_GET_INFO_BY_FD reports the
names and the corresponding program metadata. Clients using the original
4.14 argument and info structure sizes continue to work.

Upstream commits `cb4d2b3f03d8`, `ad5b177bd73f` and `473d97343f94`
define the ABI and name validation. Run the BPF selftest `test_obj_name` on
a booted test kernel to check named and legacy objects and rejection of
invalid or unterminated names.

The Android 17 and LineageOS 24.0 network loaders use these name fields
when their kernel version check is at least 4.15. Their later platform
version checks and BPF features, including BTF, still apply. This backport
leaves the reported kernel version at 4.14.113.

- [AOSP Android 17 NetBpfLoad](https://android.googlesource.com/platform/packages/modules/Connectivity/+/347fbd34b368d19f0d87e908ea101eed3601a731/bpf/loader/NetBpfLoad.cpp)
- [LineageOS 24.0 NetBpfLoad](https://github.com/LineageOS/android_packages_modules_Connectivity/blob/7c04d866538e6f689823b769fb5c1e614ca0e295/bpf/loader/NetBpfLoad.cpp)

## Cgroup socket creation

The Android network loader sets `expected_attach_type` when loading its
socket-creation filter. This 4.14 tree accepts `BPF_CGROUP_INET_SOCK_CREATE`
with `BPF_PROG_TYPE_CGROUP_SOCK` at the upstream ABI offset. A zero value
continues to support old callers. Other nonzero attach types and a nonzero
`prog_ifindex` are rejected because this backport only covers the existing
socket-creation hook and the tree has no BPF device-offload implementation.

The `test_sock_create` BPF selftest covers both load formats and rejected
attributes. Newer cgroup socket hooks, including bind hooks, are separate
kernel features. The Android loader's kernel version checks still apply.
