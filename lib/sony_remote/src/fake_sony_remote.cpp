// Fake Sony Remote - simulates a working connection without Bluetooth
#include <sony_remote/fake_sony_remote.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fake_sony_remote, LOG_LEVEL_INF);

SonyRemote::SonyRemote() {
  LOG_INF("FakeSonyRemote: constructor");
  ready = true;
}

SonyRemote::SonyRemote(const char *target_address) {
  LOG_INF("FakeSonyRemote: constructor with target address (ignored): %s",
          target_address ? target_address : "null");
}

void SonyRemote::startScan() { LOG_INF("FakeSonyRemote: startScan (no-op)"); }

void SonyRemote::stopScan() { LOG_INF("FakeSonyRemote: stopScan (no-op)"); }

void SonyRemote::end() {
  LOG_INF("FakeSonyRemote: end (no-op)");
  ready_ = true; // Keep ready state as true even after "end"
}

bool SonyRemote::ready() const { return ready_; }

void SonyRemote::log_state() { LOG_INF("FakeSonyRemote: ready=%d", ready_); }

void SonyRemote::focusDown() { LOG_INF("FakeSonyRemote: focusDown"); }

void SonyRemote::focusUp() { LOG_INF("FakeSonyRemote: focusUp"); }

void SonyRemote::shutterDown() { LOG_INF("FakeSonyRemote: shutterDown"); }

void SonyRemote::shutterUp() { LOG_INF("FakeSonyRemote: shutterUp"); }

void SonyRemote::cOneDown() { LOG_INF("FakeSonyRemote: cOneDown"); }

void SonyRemote::cOneUp() { LOG_INF("FakeSonyRemote: cOneUp"); }

void SonyRemote::recToggle() { LOG_INF("FakeSonyRemote: recToggle"); }

void SonyRemote::zoomTPress() { LOG_INF("FakeSonyRemote: zoomTPress"); }

void SonyRemote::zoomTRelease() { LOG_INF("FakeSonyRemote: zoomTRelease"); }

void SonyRemote::zoomWPress() { LOG_INF("FakeSonyRemote: zoomWPress"); }

void SonyRemote::zoomWRelease() { LOG_INF("FakeSonyRemote: zoomWRelease"); }

void SonyRemote::shoot() { LOG_INF("FakeSonyRemote: shoot"); }
