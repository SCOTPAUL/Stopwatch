# Stopwatch for Pebble

An extremely basic stopwatch for the Pebble smartwatch (since the Pebble doesn't come with a built-in stopwatch for some reason), with support for timing in the background.

## Building

After setting up [the SDK](https://developer.getpebble.com/sdk/install/linux/), clone the project into a directory and then install through a connected phone using:

````
cd Stopwatch/
pebble build
pebble install --phone <ip_address_of_phone>
````

or on the emulator using:

```
pebble install --emulator aplite
```
