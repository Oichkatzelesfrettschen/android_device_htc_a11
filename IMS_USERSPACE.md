# a11chl IMS userspace compatibility

The M8 LineageOS 22.2 product selects `PhhIms`, an open-source SIP/IMS
`ImsService` with userspace RTP audio. The service does not require the
Qualcomm vendor IMS app or the Qualcomm IMS socket. The disconnected socket
in the supplied RIL log establishes only that the RIL had no connected IMS
socket client; the log does not establish SIM AKA, IMS registration, or voice
support.

The inspected PhhIms source revision is
`a6ab624109e2ba01d6d9e59755114fd85c2b5f8e`; the M8 product integration revision
is `b12e5e5beb7b54ae8c75b55cd5c0ca59e298d5a8`. The donor's
`app/build.gradle` declares `minSdk 32`. Android 11 provides API 30.
The Android 11 package tree contains no PhhIms module, and the inspected donor
package contains no APK. Adding `PRODUCT_PACKAGES += PhhIms` alone therefore
cannot produce an Android 11 IMS service.

## Source compatibility boundary

A source port must preserve subscription changes and service teardown while
adapting the following donor mechanisms to Android 11:

- `PhhImsService.kt`: subscription-specific feature, configuration, and
  registration callbacks must use Android 11's slot-based `createMmTelFeature`,
  `getConfig`, and `getRegistration` entry points.
- `PhhMmTelFeature.kt`: `TelephonyCallback` and its registration methods must
  use Android 11's telephony listener lifecycle.
- `PhhImsService.kt`: receiver export flags and exact-alarm permission queries
  require Android 11 equivalents; periodic registration must retain its
  bounded renewal timing and teardown cancellation.
- `PhhImsConfig.kt`: `RcsClientConfiguration` and newer configuration callbacks
  require reconciliation with Android 11's `ImsConfigImplBase` contract.
- `Android.bp` and `app/jni/Android.bp`: Kotlin/coroutine modules, ARM JNI
  libraries, and the donor's C++20 WebRTC build require Android 11 toolchain
  validation. A newer platform stub JAR cannot supply missing runtime APIs.

## Product integration boundary

After an API-30-compatible service is available, the a11chl product needs the
PhhIms package, its privileged permission allowlist, and the
`android.hardware.telephony.ims.xml` feature declaration. Android 11 resolves
MmTel through `packages/services/Telephony/res/values/config.xml` resource
`config_ims_mmtel_package`; the package name is `me.phh.ims`.
The framework resource `config_device_volte_available` controls device
availability. CarrierConfig exposes `carrier_volte_available_bool` and
`config_ims_mmtel_package_override_string` for subscription policy.

Carrier policy must retain provisioning requirements. Global availability or
provisioning bypass properties cannot establish carrier admission. The M8
common property `persist.radio.jbims=1` belongs to its legacy radio integration;
the standalone SIP service does not justify copying that property to the HTC
Sprint RIL. Product availability flags remain unchanged while the compatible
service is missing.

## Orchestrator validation

Build the compatible package and product with `m bacon`. Verify package
installation, privileged permission grants, MmTel binding, and service
recovery after reboot and subscription changes before claiming registration.
Then verify an IMS-capable APN bearer, P-CSCF discovery, and USIM EAP-AKA through
`TelephonyManager.getIccAuthentication` and `RIL_REQUEST_SIM_AUTHENTICATION`
(request 125). The supplied log does not establish those RIL capabilities.

The selected `cyanogenmod_a11ul_defconfig` enables XFRM, XFRM_USER, IPv4/IPv6
ESP, AES, and HMAC. Runtime IPsec transform creation through Android 11 netd
still requires validation on the booted kernel; enabled defconfig options
alone establish neither netd compatibility nor successful SIP security
negotiation.

Validate SIP registration, incoming and outgoing ordinary calls, two-way
earpiece/speaker audio, SMS behavior, call teardown, and memory pressure on the
low-RAM product. The donor documents emergency IMS calling as outside its
implemented voice/SMS scope; ordinary call success cannot establish emergency
calling support. Redact subscriber identifiers and authentication material
from retained evidence. Keep radio configuration, modem firmware, EFS, and NV
outside the userspace port.
