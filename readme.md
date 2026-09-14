# Controlling Wiz lights

Enables controlling [WiZ](https://www.wizconnected.com/en/consumer/) lights that are part of the same network as the [WLED](https://kno.wled.ge/) controller.

The usermod takes the colors from the first few pixels and sends them to the lights.


## Compilation

Please refer to the [WLED documentation](https://kno.wled.ge/advanced/custom-features/#2-reference-it-locally-during-development) to include this mod in the compilation of WLED.

### Changing the number of lights

The usermod will default to maximum of 15 Wiz lights. That value can be overridden during the compilation of WLED by setting, for example:

```
-D WIZ_MAX_LIGHTS=20
```


## Configuration

The configuration can be found under the Usermods entry in the Setting menu.

It provides access to the following parameters:

- Interval (ms)
    - How frequently to update the WiZ lights, in milliseconds.
    - Setting it too low may cause the ESP to become unresponsive.
- Send Delay (ms)
    - An optional millisecond delay after updating each WiZ light. 
    - Can help smooth out effects when using a large number of WiZ lights
- Use Enhanced White
    - Uses the WiZ lights onboard white LEDs instead of sending maximum RGB values.
    - Tunable with warm and cool LEDs as supported by WiZ bulbs
    - Note: Only sent when max RGB value is set, the automatic brightness limiter must be disabled
    - ToDo: Have better logic for white value mixing to take advantage of the light's capabilities
- Always Force Update
    - Can be enabled to always send update message to light even if the new value matches the old value.
- Force update every x minutes
    - Adjusts the default force update timeout of 5 minutes.
    - Setting to 0 is the same as enabling Always Force Update

Next, enter the IP addresses for the lights to be controlled, in order. Use as many entries as you need and leave the rest set to the default values. To increase the limit of lights that can be access by the usermod, please see the Compilation section above.


## Authors

This WLED usermod includes contributions by:

- [@eibanez](https://github.com/eibanez)
- [@ChuckMash](https://github.com/ChuckMash), when the usermod was part of the WLED repository ([commit](https://github.com/wled/WLED/commit/099d2fd03de804b341f20681b13bba002a544b73))


## Related project

If you use these lights and python, make sure to check out the [pywizlight](https://github.com/sbidy/pywizlight) project. You can learn how to
format the messages to control the lights from that project.
