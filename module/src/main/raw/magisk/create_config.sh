mkdir -p "$MODPATH/config/properties"
touch "$MODPATH/config/properties/ro.product.mod_device"
echo -n "CN" >"$MODPATH/config/properties/ro.miui.region"
echo -n "cn" >"$MODPATH/config/properties/ro.miui.cust_variant"
echo -n "9460" >"$MODPATH/config/properties/ro.miui.mcc"
echo -n "zh-CN" >"$MODPATH/config/properties/ro.product.locale"
echo -n "CN" >"$MODPATH/config/properties/ro.product.locale.region"
echo -n "cn" >"$MODPATH/config/properties/ro.product.locale.language"

mkdir -p "$MODPATH/config/packages"
#contacts && mms (黄页、短信分类、ai通话)
touch "$MODPATH/config/packages/com.android.contacts"
touch "$MODPATH/config/packages/com.android.mms"
touch "$MODPATH/config/packages/com.xiaomi.aiasst.service"
touch "$MODPATH/config/packages/com.xiaomi.aiasst.vision"
touch "$MODPATH/config/packages/com.miui.yellowpage"
#日历
touch "$MODPATH/config/packages/com.android.calendar"
#通知中心
touch "$MODPATH/config/packages/com.miui.notification"
#翻译服务
touch "$MODPATH/config/packages/com.miui.translationservice"
touch "$MODPATH/config/packages/com.miui.translation.kingsoft"
touch "$MODPATH/config/packages/com.miui.translation.youdao"
#传送门
touch "$MODPATH/config/packages/com.miui.contentextension"
#负一屏
touch "$MODPATH/config/packages/com.miui.personalassistant"
#必要服务
touch "$MODPATH/config/packages/com.miui.contentcatcher"
touch "$MODPATH/config/packages/com.miui.cloudservice.sysbase"
