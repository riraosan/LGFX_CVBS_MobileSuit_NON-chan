# AnimatedGIF Sample for M5GFX Panel_CVBS

## Summary

[![機動戦士のんちゃん](http://img.youtube.com/vi/LADcNpvA-8U/0.jpg)](https://www.youtube.com/watch?v=LADcNpvA-8U)

This program is a sample of AnimatedGIF Library.
You can play "[Mobile Suit NON-chan](https://nosferatunon.wixsite.com/nonchan/kn-non)" Animated GIF and BGM on your Digital TV (composite).

## Devices

I have tested this sample program with the following devices.

- [ATOM Lite ESP32 IoT Development Kit](https://www.switch-science.com/catalog/6262/)
- [ATOM TF-Card Reader Development Kit up to 16GB](https://www.switch-science.com/catalog/6475/)
- [ATOM Speaker Kit (NS4168)](https://www.switch-science.com/catalog/7092/)
- microSD 16M(TF-CARD)
- External Digital Analog Converter(DAC Bit per sample is 32bit or 16bit)
  - NS4168    (16Bit per sample)
  - ES9038MQ2M(32Bit per sample)

## How to build

This sample has been tested to build only in the PlatformIO IDE environment.
I have not checked if it can be built in the Arduino IDE environment. If you know how to build it, I would appreciate it if you commit it to this repository.

If you want to build in Arduino IDE environment, please change the setting of platformio.ini for Arduino IDE environment. I believe that the settings will probably be almost the same.

### Library

I checked with arduino-esp32 library version 2.0.3. 

```yaml:platformio.ini
[arduino-esp32]
platform          = platformio/espressif32@^4.4.0
```

I have put the GitHub link to the library under the `lib_deps =` directive. You can download the library from GitHub yourself and register it with the Arduino IDE.

```yaml:platformio.ini
lib_deps =
        https://github.com/bitbank2/AnimatedGIF.git#1.4.7
        https://github.com/m5stack/M5Unified.git#0.0.7
        https://github.com/earlephilhower/ESP8266Audio.git
        https://github.com/LennartHennigs/Button2.git
```

### I2S Audio Output

- You set the GPIO number that you are connecting to the I2S of the external DAC.

```cpp
  // Audio
  out = new AudioOutputI2S(I2S_NUM_1);  // CVBSがI2S0を使っている。AUDIOはI2S1を設定
  out->SetPinout(21, 25, 22);           // for TF-CARD Module
  out->SetGain(0.8);                    // 1.0だと音が大きすぎる。0.3ぐらいが適当。後は外部アンプで増幅するのが適切。
```

- The ATOM SPK module's built-in DAC has 16 sample bits. You do not need to replace the modification file or add build flags.

If you use a DAC (ES9038MQ2M) with 32 sample bits, you need to do the following setup/operation.

- [x] You should replace the files in the patch folder with the files in the ESP8266Audio folder. (i2s_write_expand is added)
- [x] You must put the definition of BITS_PER_SAMPLE_32BIT_DAC in the build flags in platformio.ini.

```yaml:platformio.ini
build_flags =
        -D BITS_PER_SAMPLE_32BIT_DAC
````

### gif and mp3 files

After you have stored the non5.gif, non5.mp3 files you have placed under the data folder in the microSD, insert them into the microSD slot of each module.

## How to use

- You must somehow connect the G25 (or G26) port to the composite input of your digital TV. (Instructions are omitted.)
- Turn on the ATOM Lite. The digital TV screen will display `Long Click: episode 5` on a black background.
- If you click the G39 button, ATOM Lite will play Mobile Suit NON-CHAN episode 5 "OSクラッシャー".

## License

MIT License

Feel free to modify or reprint. We would appreciate it if you could reprint the URL of this repository.

## Acknowledgements

If I could look out over the distance, it was on the shoulders of giants.
We would like to thank the authors of each library. Thank you very much.

- Special thanks to [bitbank2](https://github.com/bitbank2), author of [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF).
- Special thanks to [Roger-random](https://github.com/Roger-random), author of [ESP_8_BIT_composite](https://github.com/Roger-random/ESP_8_BIT_composite.git).
- Special thanks to [lovyan03](https://github.com/lovyan03), author of [LovyanGFX(M5GFX)](https://github.com/lovyan03/LovyanGFX.git).

## Conclusion

It would be a great pleasure and a blessing if I could contribute in some way to someone somewhere.

Enjoy!👍
