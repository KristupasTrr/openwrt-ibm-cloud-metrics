include $(TOPDIR)/rules.mk

PKG_NAME:=ibm-conn
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

# PKG_BUILD_DEPENDS:=libiotp-as-device

include $(INCLUDE_DIR)/package.mk

define Package/ibm-conn
	CATEGORY:=Base system
	TITLE:=ibm-conn
	DEPENDS:=+libubus +libubox +libblobmsg-json +iot-c-sdk # nurodyti biblioteka, o src Makefile -> liba naudoti
endef

define Package/ibm-conn/description
	MQTT connection to IBM cloud
endef

define Package/ibm-conn/install
	$(INSTALL_DIR) $(1)/etc/config $(1)/etc/init.d
	$(INSTALL_BIN) ./files/ibm-conn.init $(1)/etc/init.d/ibm-conn
	$(INSTALL_CONF) ./files/ibm-conn.config $(1)/etc/config/ibm-conn
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ibm-conn $(1)/usr/bin
endef

$(eval $(call BuildPackage,ibm-conn))