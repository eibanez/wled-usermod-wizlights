#include "wled.h"
#include <WiFiUdp.h>
#include "colors.h"

// Maximum number of lights supported (defaults to 15)
#ifndef WIZ_MAX_LIGHTS
  #define WIZ_MAX_LIGHTS 15
#endif

WiFiUDP UDP;

class WizLightsUsermod : public Usermod {
  
  private:
    unsigned long lastTime = 0;

    long updateInterval;
    long sendDelay;
    long forceUpdateMinutes;
    bool forceUpdate;

    IPAddress lightsIP[WIZ_MAX_LIGHTS];    // Stores light IP addresses
    bool      lightsValid[WIZ_MAX_LIGHTS]; // Stores light IP address validity (string is formatted light an IP address)
    uint32_t  colorSent[WIZ_MAX_LIGHTS];   // Stores last color sent for each light
    uint8_t   cctSent[WIZ_MAX_LIGHTS];     // Stores last CCT sent for each light

  public:
    // Get color and CCT data for the pixel representing a Wiz light
    void getPixelData(uint8_t pix, uint32_t &color, uint8_t &cct) {
      // Get color for the pixel
      color = strip.getPixelColor(pix);

      // Get CCT and color correct, unless the color is black (off)
      cct = 0;
      if (color != BLACK) {
        // Use color gamma correction if enabled, not in realtime mode with gamma disabled or currently overriding RT mode
        //   NOTE: This condition is the same used inside the WLED code
        bool useGammaCorrection = gammaCorrectCol && !(realtimeMode && arlsDisableGammaCorrection && !realtimeOverride);
        if (useGammaCorrection) color = gamma32(color);
        
        // Find the segment the pixel belongs to
        for (unsigned i = 0; i < strip.getSegmentsNum(); i++) {
          Segment& seg = strip.getSegment(i);
          
          if (pix >= seg.start && pix <= seg.stop) {
            cct = seg.cct;
            break;  // No need to look in other segments
          }
        }
      }
    }

    // Send JSON message to WiZ Light over UDP (RGB or C/W white)
    void wizSendColor(IPAddress ip, uint32_t color, uint8_t cct) {
      // Start UDP packet
      UDP.beginPacket(ip, 38899);

      // If color is black, turn light off
      //   NOTE: Wiz light setting for "Off fade-out" will be applied by the light itself
      if (color == BLACK) { 
        UDP.print("{\"method\":\"setPilot\",\"params\":{\"state\":false}}");

      // Send color as RGB  
      } else {
        // Pull white value (to adjust for brightness)
        unsigned w = W(color);
        
        // Linear blend of warm and cold white (to avoid overheating the light bulb)
        //   CCT: 0 - full warm white, 255 - full cold white
        uint8_t ww = ((255 - cct) * w) / 255;
        uint8_t cw = (cct * w) / 255;

        // Send color information (red, green, blue, warm white, cold white)
        UDP.print("{\"method\":\"setPilot\",\"params\":{\"r\":");
        UDP.print(R(color));
        UDP.print(",\"g\":");
        UDP.print(G(color));
        UDP.print(",\"b\":");
        UDP.print(B(color));
        UDP.print(",\"w\":");
        UDP.print(ww);
        UDP.print(",\"c\":");
        UDP.print(cw);
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
      
      unsigned long ellapsedTime = millis() - lastTime;
      if (ellapsedTime > updateInterval) {
        // Track whether any of the lights were updated in this pass
        bool update = false;
        
        for (uint8_t i = 0; i < WIZ_MAX_LIGHTS; i++) {
          // Skip lights without a valid IP address
          if (!lightsValid[i]) continue;

          // Get color and CCT for this light
          uint32_t newColor;
          uint8_t newCct;
          getPixelData(i, newColor, newCct);

          // Update Wiz light color, if necessary
          if (forceUpdate || (newColor != colorSent[i]) || (newCct != cctSent[i]) || (ellapsedTime > forceUpdateMinutes*60000)) {
            wizSendColor(lightsIP[i], newColor, newCct);
            colorSent[i] = newColor;
            cctSent[i] = newCct;
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
      top["Always Force Update"]          = forceUpdate;
      top["Force Update Every x Minutes"] = forceUpdateMinutes;
      
      for (uint8_t i = 0; i < WIZ_MAX_LIGHTS; i++) {
        top[getJsonLabel(i)] = lightsIP[i].toString();
      }
    }

    // Load configuration parameters
    bool readFromConfig(JsonObject& root) {
      JsonObject top = root["wizLightsUsermod"];
      bool configComplete = !top.isNull();

      configComplete &= getJsonValue(top["Interval (ms)"],                updateInterval,     1000);  // How frequently to update the Wiz lights
      configComplete &= getJsonValue(top["Send Delay (ms)"],              sendDelay,          0);     // Optional delay after sending each UDP message
      configComplete &= getJsonValue(top["Always Force Update"],          forceUpdate,        false); // Update Wiz lights every loop, even if color value has not changed
      configComplete &= getJsonValue(top["Force Update Every x Minutes"], forceUpdateMinutes, 5);     // Update Wiz lights if color value has not changed, every x minutes
      
      // Read list of IPs
      String tempIp;
      for (uint8_t i = 0; i < WIZ_MAX_LIGHTS; i++) {
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
