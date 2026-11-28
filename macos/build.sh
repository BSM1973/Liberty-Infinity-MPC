#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
APP="dist/Liberty Infinity MPC Test.app"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
xcrun clang++ -arch arm64 -std=c++17 -O2 -fobjc-arc -I sdk/framework macos/Main.mm -framework Cocoa -framework AVFoundation -framework CoreMIDI -o "$APP/Contents/MacOS/Infinity"
cp plugins/Infinity/layout.json plugins/Infinity/assets/{main,filter,delay,reverb,amp-modifier,libraries,factory-1,gold_knob_64,gold_knob_104,power_40_on,power_40_off,power_76_on,power_76_off,selector_156}.png "$APP/Contents/Resources/"
cp -R plugins/Infinity/Content "$APP/Contents/Resources/"
cat > "$APP/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>Infinity</string>
<key>CFBundleIdentifier</key><string>com.libertymusicsoftware.infinitympctest</string>
<key>CFBundleName</key><string>Liberty Infinity MPC Test</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>0.9.12</string>
<key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
codesign --force --sign - "$APP"
codesign --verify --strict "$APP"
"$APP/Contents/MacOS/Infinity" --qc-backup
"$APP/Contents/MacOS/Infinity" --qc-library
ditto -c -k --sequesterRsrc --keepParent "$APP" dist/Liberty-Infinity-Black-Gold-Mac-Test-arm64.zip
