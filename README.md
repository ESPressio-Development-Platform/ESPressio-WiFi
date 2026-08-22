# ESPressio-WiFi
WiFi Components of the ESPressio Development Platform

## Version / release state

The repository's legacy package manifests declare **1.0.0**, but the current repository does not contain a `src` implementation and no verified published stable release/tag is presently evidenced by the repository history available to this audit.

Accordingly, **1.0.0 should be treated as legacy package/source metadata, not as evidence of a usable or verified stable WiFi release**. The manifest version is being preserved for historical accuracy rather than rewritten to invent a new release history.

## Compatibility

This repository does not currently contain a `src` implementation, so it does not yet provide functional WiFi support for any microcontroller.

The intended targets are the ESP32 and ESP8266 families using their respective Arduino frameworks. The manifests are restricted to those architecture/platform families to describe that intended scope, but this must not be interpreted as a working or tested implementation until source code is added.

## Dependency status

ESPressio-WiFi is not currently an active dependency edge in the released ESPressio graph. No current ESPressio library should depend on this repository merely because legacy package metadata exists here.
