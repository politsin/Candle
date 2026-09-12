# Qt 6 modernization track

## Target and non-negotiable rule

The production branch currently builds with Qt 5.15.2 because Candle's legacy
user-script ecosystem relies on QtScript. The target for this track is Qt
6.11.2, the current Qt 6.11 patch release at the time this document was updated.
The migration must retain the operational CNC features: visual GUI,
`candle-cli`, GRBL, Marlin, G-code parser, height map and the localhost
automation API. It does **not** preserve the legacy QtScript plugin runtime.

Do not replace the production Qt 5 build in place. QtScript was removed from
Qt 6, so switching `find_package(Qt5 ...)` to Qt6 would silently remove a large
part of the scripting/plugin feature set or fail to build.

## Approved product boundary

The Qt 6 build retires the unrestricted QtScript runtime. Its four bundled
plugins are replaced with explicit native features or intentionally scoped
away from the controller contract:

| Legacy plugin | Decision | Replacement |
| --- | --- | --- |
| `camera` | Native Qt 6 dock | Live USB-camera preview, device selector, mirror and center reticle. Vision/calibration decisions remain external via the localhost API. |
| `emergencybutton` | Native Qt 6 user-panel action | Red Emergency Stop button. Marlin sends `M112`; GRBL uses the existing reset path. A physical E-stop remains mandatory. |
| `usercommands` | Native Qt 6 user-panel action | Persisted command buttons with labels and icons; no GUI `eval()` or arbitrary Qt object access. |
| `coordinatesystem` | Native Qt 6 GRBL panel | `G54…G59` selector plus `G10 L20` work-zero actions; automatically unavailable for Marlin. |

This removes the need to port a generated QtScript binding layer for every Qt
class. It also makes the future automation boundary explicit: Candle is the
machine adapter, while vision, orchestration and decision making are external
services.

## Measured migration surface

* The legacy build contained 46 direct `QScript*` references in handwritten Candle sources.
* The legacy script binding generator contains about 45,000 `QScript*` source
  lines.  It exposes Qt/Candle objects to user scripts and is the main blocker.
* 59 uses of Qt 5 APIs already deprecated or removed in Qt 6 were found,
  including `QRegExp`, `QGLWidget`, `QTime::start/elapsed`, `qSort`, and
  `QSysInfo::windowsVersion`.

## Staged implementation

1. **Build duality.** Introduce `find_package(QT NAMES Qt6 Qt5 ...)` and CMake
   wrappers while keeping the existing Qt 5 preset green. Add a separate
   `windows-x64-qt6-release` preset; do not repoint the operational build.
2. **Remove straightforward deprecated APIs.** Convert `QRegExp` to
   `QRegularExpression`, `QTime` timing to `QElapsedTimer`, `qSort` to
   `std::sort`, Windows version checks to `QOperatingSystemVersion`, and
   `QGLWidget` to `QOpenGLWidget`. These changes can be regression-tested on
   Qt 5 first.
3. **Remove the legacy script runtime.** Exclude the QtScript editor, generated
   bindings, `eval()` user-command path and the four plugin folders from the
   Qt 6 package. Replace the emergency action and command profiles with small
   native/API features.
4. **Remove generated bindings.** The QtScript generator output is not carried
   to Qt 6. The supported integration surface is the versioned local HTTP API,
   not in-process scripting of arbitrary Qt objects.
5. **Port CMake/deployment.** Move resources, translations and plugins to Qt 6
   CMake APIs and update `windeployqt` packaging for both `candle.exe` and
   `candle-cli.exe`.
6. **Bring up a Qt 6 build.** Install Qt 6.11.2 MSVC 2022 x64 (or a verified
   vcpkg equivalent), compile, then run API/GUI/CLI smoke tests without a
   machine and with the controlled Marlin test stand.
7. **Feature parity gate.** Only after GUI, CLI and API tests pass may the Qt
   6 build become the preview release. Qt 5 remains available as the rollback
   artifact until a real-machine regression cycle completes.

## Definition of done

* Both `candle.exe` and `candle-cli.exe` build from a clean Qt 6 environment.
* A visible GUI and a no-window CLI can coexist without taking each other's
  controller connection.
* Existing Marlin Wi-Fi and GRBL serial profiles pass `/test/connection`.
* Loading, jogging, homing, file sender, spindle, height-map and automation
  safety-arm paths have automated tests plus a documented manual machine test.
* The shipped archive contains no Qt 5 DLLs and has a reproducible build
  command.

## References

[Qt's official porting guide](https://doc.qt.io/qt-6/portingguide.html) notes
that Qt 6 removes some Qt 5 modules and recommends resolving Qt 5.15
deprecations before moving. [Qt 6.11 release information](https://www.qt.io/blog/qt-6-11-released)
is available from Qt's official release pages.
