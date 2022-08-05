mkdir -p "$MODPATH/config/properties"
echo -n "310030" >"$MODPATH/config/properties/gsm.sim.operator.numeric"
echo -n "us" >"$MODPATH/config/properties/gsm.sim.operator.iso-country"

mkdir -p "$MODPATH/config/packages"
touch "$MODPATH/config/packages/com.google.android.gsf"
touch "$MODPATH/config/packages/com.google.android.gms"
touch "$MODPATH/config/packages/com.google.android.apps.maps"
