# HTC HAL1 camera integration

The legacy camera provider runs in the 32-bit cameraserver process because
`TARGET_USES_NON_TREBLE_CAMERA` selects passthrough transport. `camera.msm8226`
loads `camera.vendor.msm8226` with module API 1.0, zeroes the extended camera
information, forwards HAL1 operations, and owns copies of parameter strings.
The common tree starts `qcamerasvr` as camera with camera, system, inet,
input, and graphics groups. Its existing ueventd rules grant system:camera
access to video, media, V4L2 subdevice, JPEG, and rawchip nodes.

The vendor camera group retains the CM13 HTC HAL, sensor modules, EEPROM
modules, S5K5E2/S5K6A1 tuning, chromatix, CPP firmware, JPEG, and required
HTC effects/algorithm dependencies. `camera-inputs.tsv` in the vendor tree
records original and installed SHA-256 values. `tools/camera-symbols.map`
lists every renamed dynamic import and its source implementation. Patchelf
0.19.1 changes dynamic symbol names in six libraries; their `.text` dumps
match their captured inputs. The scoped linker mappings add
`liba11-camera-abi` and the existing HTC logging adapter to their load groups.

| Legacy imports | Adapter behavior |
| --- | --- |
| SensorManager default constructor, singleton lock/slot, getDefaultSensor, createEventQueue | Keep the 36-byte legacy allocation as a pointer adapter; let the framework resolve the manager from the calling UID and return real libsensor queues. |
| SensorEventQueue read, getFd, enableSensor, disableSensor, setEventRate | Forward to libsensor and acknowledge delivered wake-up events. RefBase stays the primary base at offset zero. |
| GraphicBuffer four-argument constructor, lock, unlock, getNativeBuffer | Construct a small RefBase proxy inside the 120-byte legacy allocation; allocate and own a separate Android 11 buffer. Return the backing native buffer with checked ARM32 stride and handle offsets. |
| HTC CameraParameters capture/burst key constants, getRawSize, getBrightnessLumaTargetSet | Preserve HTC key strings and parse the raw-size and brightness/luma pairs through framework CameraParameters. |
| EVP_SealInit, EVP_SealFinal, EVP_OpenInit, EVP_OpenFinal | Perform real RSA envelope encryption and cipher processing with BoringSSL; allocate RSA-sized decrypt storage and cleanse key material. |
| __htclog_init_mask | Reuse liba11-legacy-radio's bounded HTC logging-mask implementation. |

Unique camera import names prevent the legacy adapters from replacing the
framework's own SensorManager and GraphicBuffer methods. The proxy design
also avoids placement-constructing larger framework objects in HTC's smaller
allocations. The wrapper and adapter compile with -Wall -Wextra -Werror.

The camera payload contains TEXTREL libraries. The process SDK override must
apply to cameraserver and the resolved mm-qcamera-daemon executable path at
API 22. The daemon uses the system/vendor path on the embedded-vendor image.
Android 11 SELinux forbids granting execmod to these library files. The
existing permissive bring-up product can test the retained payload; an
enforcing product requires TEXTREL-free replacements. The camera policy
adds the daemon transition, camera device access, and per-camera socket
labels without weakening the platform's execmod prohibition.

## Orchestrator validation

Run `m bacon` with all three camera worktree commits present. Check the packed
image for the wrapper, vendor HAL, ABI adapter, daemon, complete vendor camera
manifest, and vendor/etc/media_profiles_V1_0.xml. Check the final linker SDK
and shim definitions, especially the resolved system/vendor daemon path.

On device, retain sanitized logs proving qcamerasvr stays running,
cameraserver reports two HAL1 cameras through legacy/0, and both cameras
open and deliver preview frames. Exercise autofocus and flash where the
rear camera supports them, JPEG capture on both sensors, orientation,
front/rear switching, recording with working audio/video synchronization,
and at least 20 open/capture/close cycles. Record daemon/cameraserver crash
counts, linker errors, SELinux denials, buffer corruption, memory pressure,
and peak/PSS behavior during those operations. Test the HTC effects path
that constructs GraphicBuffer proxies and the CAF sensor listener path.

Local checks cover ELF dependency names against the retained Android 11
libraries, source alias coverage, source/installed hashes, unchanged `.text`
for patched libraries, ARM32 adapter syntax and size/offset assertions,
wrapper/JPEG syntax, shell lint, XML parsing, and Git whitespace. Full target
linking, the dynamic linker namespace, SELinux behavior, and device lifecycle
remain orchestrator gates. The supplied vendor checkout lacks the requested
cm13-mm-camera ref; the captured CM13 vendor manifest/payload supplied the
camera inputs instead. Every selected camera input was available.
