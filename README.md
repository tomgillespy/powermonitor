# powermonitor
A NodeMCU Based Power monitor for the UK power grid.

The Micro for this is a NodeMCU 1.0 with an ESP-12E and its my first foray into the world of PlatformIO and using something like `maakbaas/esp8266-iot-framework`
as a provider of a react interface, api for the micro and a WifiManager.

Power information is provider by gridwatch.templar.co.uk and processed by the free tier of AWS Lamdba. https://4fgfqq5us9.execute-api.eu-west-2.amazonaws.com/.
