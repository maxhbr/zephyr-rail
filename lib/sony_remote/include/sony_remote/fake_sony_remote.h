// Fake Sony Remote - simulates a working connection without Bluetooth
#pragma once

#include <cstddef>
#include <cstdint>

class SonyRemote {
public:
  SonyRemote();
  SonyRemote(const char *target_address); // Constructor with specific BT
                                          // address (ignored)

  void begin();       // no-op
  void startScan();   // no-op
  void end();         // no-op
  bool ready() const; // always returns true

  void log_state(); // no-op

  // Common actions (all no-ops)
  void focusDown();
  void focusUp();
  void shutterDown();
  void shutterUp();
  void cOneDown();
  void cOneUp();
  void recToggle();
  void zoomTPress();
  void zoomTRelease();
  void zoomWPress();
  void zoomWRelease();

  // high level functions:
  void shoot();

private:
  bool ready_ = true;
};
