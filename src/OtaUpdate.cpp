// OtaUpdate.cpp
#include "OtaUpdate.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <shared_vars.h>
// #include <BufferedLogger.h>   // for logger

// extern BufferedLogger logger;

bool performHttpOta(const String &url)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        logger.log("OTA: WiFi not connected; aborting OTA\n");
        return false;
    }

    logger.log("OTA: starting OTA from URL: " + url + "\n");

    HTTPClient http;
    if (!http.begin(url))
    {
        logger.log("OTA: http.begin() failed\n");
        return false;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        logger.log("OTA: HTTP GET failed with code ");
        logger.log(httpCode);
        logger.log("\n");
        http.end();
        return false;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0)
    {
        logger.log("OTA: invalid Content-Length; using UPDATE_SIZE_UNKNOWN\n");
    }
    else
    {
        logger.log("OTA: Content-Length = ");
        logger.log(contentLength);
        logger.log("\n");
    }

    WiFiClient *stream = http.getStreamPtr();

    // Begin OTA update
    if (!Update.begin(contentLength > 0 ? (size_t)contentLength : UPDATE_SIZE_UNKNOWN))
    {
        logger.log("OTA: Update.begin() failed; not enough space?\n");
        http.end();
        return false;
    }

    logger.log("OTA: writing firmware to flash...\n");

    uint8_t buffer[1024];
    size_t totalWritten = 0;
    while (http.connected() && (contentLength > 0 || contentLength == -1))
    {
        size_t available = stream->available();
        if (available)
        {
            size_t toRead = available;
            if (toRead > sizeof(buffer)) toRead = sizeof(buffer);

            int bytesRead = stream->readBytes(buffer, toRead);
            if (bytesRead <= 0)
            {
                logger.log("OTA: read error from stream\n");
                break;
            }

            size_t written = Update.write(buffer, bytesRead);
            totalWritten += written;

            if (written != (size_t)bytesRead)
            {
                logger.log("OTA: short write to flash\n");
                break;
            }

            if (contentLength > 0)
            {
                contentLength -= bytesRead;
            }
        }
        delay(1);
    }

    http.end();

    if (!Update.end())
    {
        logger.log("OTA: Update.end() failed. Error: ");
        logger.log(Update.getError());
        logger.log("\n");
        return false;
    }

    if (!Update.isFinished())
    {
        logger.log("OTA: Update not finished; something went wrong\n");
        return false;
    }

    logger.log("OTA: Firmware update successful, written bytes = ");
    logger.log((int)totalWritten);
    logger.log("\nOTA: restarting...\n");

    // Give logger a moment if it flushes asynchronously
    delay(500);
    ESP.restart();

    // We won't reach here, but return true to indicate success
    return true;
}
