#ifndef __SERIALCMD_h__
#define __SERIALCMD_h__

class SerialCommand {
  public:
    void readAndProcess() {
      if (Serial.available()) {
        parseCommand(Serial.readString());
      }
    }

    void parseCommand(String command) {
    }
};


#endif
