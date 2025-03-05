include /opt/fpp/src/makefiles/common/setup.mk

all: libfpp-plugin-tplink.so
debug: all

OBJECTS_fpp_tplink_so += src/tplinkPlugin.o src/TPLinkItem.o src/TPLinkLight.o src/TPLinkSwitch.o src/BaseItem.o src/BaseSwitch.o src/BaseLight.o src/GoveeLight.o src/TasmotaLight.o src/TasmotaSwitch.o
LIBS_fpp_tplink_so += -L/opt/fpp/src -lfpp
CXXFLAGS_src/tplinkPlugin.o += -I/opt/fpp/src

%.o: %.cpp Makefile
	$(CCACHE) $(CC) $(CFLAGS) $(CXXFLAGS) $(CXXFLAGS_$@) -c $< -o $@

libfpp-plugin-tplink.so: $(OBJECTS_fpp_tplink_so) /opt/fpp/src/libfpp.so
	$(CCACHE) $(CC) -shared $(CFLAGS_$@) $(OBJECTS_fpp_tplink_so) $(LIBS_fpp_tplink_so) $(LDFLAGS) -o $@

clean:
	rm -f libfpp-plugin-tplink.so $(OBJECTS_fpp_tplink_so)
