ui_print "- Magisk version: $MAGISK_VER_CODE"
if [ "$MAGISK_VER_CODE" -lt 24000 ]; then
  ui_print "*********************************************************"
  ui_print "! Please install Magisk v24+"
  abort    "*********************************************************"
fi

ui_print "- Checking arch"

if [ "$ARCH" != "arm" ] && [ "$ARCH" != "arm64" ] && [ "$ARCH" != "x86" ] && [ "$ARCH" != "x64" ]; then
  abort "! Unsupported platform: $ARCH"
else
  ui_print "- Device platform: $ARCH"
fi

# Check System API Level
if [ "$API" -lt "26" ];then
  ui_print "Unsupported api version ${API}"
  abort "This module only for Android 8+"
fi

# Create config
. $MODPATH/create_config.sh


# Set permission
ui_print "- Set permissions"
set_perm_recursive $MODPATH 0    0    0755 0644
