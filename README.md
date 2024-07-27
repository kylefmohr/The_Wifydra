This is my fork of @lozaning's excellent Wifydra project. I modified it to use my hodgepodge of ESP32 boards, and also added compatibility with PlatformIO. 

Another minor modification I made was that instead of using a MicroSD card to store the results, I'm instead using SPIFFS (onboard flash storage on the Dom board). I was having issues with my MicroSD adapters, and my main board happens to be an ESP32-S3-DevKitC-1-N32R8V, which has a comfortable 32MB of storage. It automatically dumps the contents of the .csv file when it is booted up and connected to a serial monitor.
