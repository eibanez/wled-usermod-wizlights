#include "wled.h"
#include <WiFiUdp.h>
#include "colors.h"

// Maximum number of lights supported
#define MAX_WIZ_LIGHTS 15

WiFiUDP UDP;

class WizLightsUsermod : public Usermod {
  
  private:
    unsigned long lastTime = 0;
    long updateInterval;
    long sendDelay;
    
    long forceUpdateMinutes;
    bool forceUpdate;

    bool useEnhancedWhite;
    long warmWhite;
    long coldWhite;

    IPAddress lightsIP[MAX_WIZ_LIGHTS];    // Stores light IP addresses
    bool      lightsValid[MAX_WIZ_LIGHTS]; // Stores light IP address validity (string is formatted light an IP address)
    uint32_t  colorsSent[MAX_WIZ_LIGHTS];  // Stores last color sent for each light

  public:
    // Send JSON message to WiZ Light over UDP (RGB or C/W white)
    void wizSendColor(IPAddress ip, uint32_t color, bool gammaCorrect) {
      // Start UDP packet
      UDP.beginPacket(ip, 38899);

      // If color is black, turn light off
      //   NOTE: Wiz light setting for "Off fade-out" will be applied by the light itself
      if (color == 0) { 
        UDP.print("{\"method\":\"setPilot\",\"params\":{\"state\":false}}");

      // If color is white, try and use the lights WHITE LEDs instead of mixing RGB LEDs
      } else if (color == 16777215 && useEnhancedWhite) {
        // TODO: Better utilize WLED existing white mixing logic

        // Set cold white light only
        if (coldWhite > 0 && warmWhite == 0) {
          UDP.print("{\"method\":\"setPilot\",\"params\":{\"c\":"); UDP.print(coldWhite) ;UDP.print("}}");
        }
          
        // Set warm white light only
        if (warmWhite > 0 && coldWhite == 0) {
          UDP.print("{\"method\":\"setPilot\",\"params\":{\"w\":"); UDP.print(warmWhite) ;UDP.print("}}");
        }
          
        // Set combination of warm and cold white light
        if (coldWhite > 0 && warmWhite > 0) {
          UDP.print("{\"method\":\"setPilot\",\"params\":{\"c\":"); UDP.print(coldWhite) ;UDP.print(",\"w\":"); UDP.print(warmWhite); UDP.print("}}");
        }

      // Send color as RGB  
      } else {
        // Use gamma color correction, as needed
        uint32_t color2 = color;
        if (gammaCorrect) color2 = gamma32(color2);

        // Send RBG information
        UDP.print("{\"method\":\"setPilot\",\"params\":{\"r\":");
        UDP.print(R(color2));
        UDP.print(",\"g\":");
        UDP.print(G(color2));
        UDP.print(",\"b\":");
        UDP.print(B(color2));
        UDP.print("}}");
      }
    
      // Finish UDP packet
      UDP.endPacket();
    }

    // Override definition of setup() so it compiles
    void setup() {
      
    }

    // The loop() colors updates the light colors
    void loop() {
      // Make sure we are connected first
      if (!WLED_CONNECTED) return;

      // Use color gamma correction if enabled, not in realtime mode with gamma disabled or currently overriding RT mode
      //   NOTE: This condition is the same used inside the WLED code
      bool useGammaCorrection = gammaCorrectCol && !(realtimeMode && arlsDisableGammaCorrection && !realtimeOverride);
      
      unsigned long ellapsedTime = millis() - lastTime;
      if (ellapsedTime > updateInterval) {
        // Track whether any of the lights were updated in this pass
        bool update = false;
        
        for (uint8_t i = 0; i < MAX_WIZ_LIGHTS; i++) {
          // Skip lights without a valid IP address
          if (!lightsValid[i]) continue;

          // Get color for this light
          uint32_t newColor = strip.getPixelColor(i);

          // Update Wiz light color, if necessary
          if (forceUpdate || (newColor != colorsSent[i]) || (ellapsedTime > forceUpdateMinutes*60000)) {
            wizSendColor(lightsIP[i], newColor, useGammaCorrection);
            colorsSent[i] = newColor;
            update = true;
            delay(sendDelay);
          }
        }
        
        if (update) lastTime = millis();
      }
    }

    // Save configuration parameters
    void addToConfig(JsonObject& root) {
      JsonObject top = root.createNestedObject("wizLightsUsermod");
      top["Interval (ms)"]                = updateInterval;
      top["Send Delay (ms)"]              = sendDelay;
      top["Use Enhanced White *"]         = useEnhancedWhite;
      top["* Warm White Value (0-255)"]   = warmWhite;
      top["* Cold White Value (0-255)"]   = coldWhite;
      top["Always Force Update"]          = forceUpdate;
      top["Force Update Every x Minutes"] = forceUpdateMinutes;
      
      for (uint8_t i = 0; i < MAX_WIZ_LIGHTS; i++) {
        top[getJsonLabel(i)] = lightsIP[i].toString();
      }
    }

    // Load configuration parameters
    bool readFromConfig(JsonObject& root) {
      JsonObject top = root["wizLightsUsermod"];
      bool configComplete = !top.isNull();

      configComplete &= getJsonValue(top["Interval (ms)"],                updateInterval,     1000);  // How frequently to update the Wiz lights
      configComplete &= getJsonValue(top["Send Delay (ms)"],              sendDelay,          0);     // Optional delay after sending each UDP message
      configComplete &= getJsonValue(top["Use Enhanced White *"],         useEnhancedWhite,   false); // When color is white, use Wiz white LEDs instead of mixing RGB
      configComplete &= getJsonValue(top["* Warm White Value (0-255)"],   warmWhite,          0);     // Warm white LED value for enhanced white
      configComplete &= getJsonValue(top["* Cold White Value (0-255)"],   coldWhite,          50);    // Cold white LED value for enhanced white
      configComplete &= getJsonValue(top["Always Force Update"],          forceUpdate,        false); // Update Wiz lights every loop, even if color value has not changed
      configComplete &= getJsonValue(top["Force Update Every x Minutes"], forceUpdateMinutes, 5);     // Update Wiz lights if color value has not changed, every x minutes
      
      // Read list of IPs
      String tempIp;
      for (uint8_t i = 0; i < MAX_WIZ_LIGHTS; i++) {
        configComplete &= getJsonValue(top[getJsonLabel(i)], tempIp, "0.0.0.0");
        lightsValid[i] = lightsIP[i].fromString(tempIp);
        
        // If the IP is not valid, force the value to be empty
        if (!lightsValid[i]) lightsIP[i].fromString("0.0.0.0");
      }

      return configComplete;
    }

    // Create label for the usermod HTML page
    String getJsonLabel(uint8_t i) {
      return "WiZ Light IP #" + String(i+1);
    }
    
    uint16_t getId() {
      return USERMOD_ID_WIZLIGHTS;
    }
};

static WizLightsUsermod wizlights;
REGISTER_USERMOD(wizlights);
