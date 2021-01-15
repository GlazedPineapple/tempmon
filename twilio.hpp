#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>

#include "base64.h"
#include "url_coding.hpp"

class Twilio {
  public:
    Twilio(
        const String account_sid_in,
        const String auth_token_in)
        : account_sid(account_sid_in), auth_token(auth_token_in) {}
    // Empty destructor
    ~Twilio() = default;

    bool send_message(
        const String &to_number,
        const String &from_number,
        const String &message_body,
        String &response,
        const String &picture_url = "");

  private:
    // Account SID and Auth Token come from the Twilio console.
    // See: https://twilio.com/console for more.

    // Used for the username of the auth header
    String account_sid;
    // Used for the password of the auth header
    String auth_token;

    // Utilities
    static String _get_auth_header(
        const String &user,
        const String &password);
};
