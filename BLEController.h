#ifndef BLE_CONTROLLER_H
#define BLE_CONTROLLER_H

#include <Arduino.h>

class BLEController
{
public:
    using CommandCallback = bool (*)(const String &command);

    void begin();
    void update();

    void notify(const String &message);

    void print(const String &message);
    void println(const String &message = "");

    void setCommandCallback(CommandCallback callback);
    void receiveCommand(const String &command);

    bool isConnected() const;
    bool isOtaRunning() const;

    void clearAck();
    bool isAckReceived(uint32_t sequence) const;

    void stop();

private:
    CommandCallback _commandCallback = nullptr;
    uint32_t _ackSequence = 0;
    bool _ackReceived = false;

#endif