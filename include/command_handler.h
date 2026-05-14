// command_handler.h
#pragma once
#include <Arduino.h>

// Processes a command string (same syntax used by Telegram)
// and returns the response string to send back to the caller.
String processCommand(const String& text);
