# SMC-Influxdb

Fetch SMC data from macOS Intel hardware and output in InfluxDB line protocol. Designed to be automatically called by a Telegraf exec input plugin.

Code was mofified for my hardware and influxdb structure.

## Usage 

smc-influxdb



### Compiling

```
make
```

### Running

```
./smc-influxdb
```

### Output example

```
temperature,host=laptop,sensor=GPU-Proximity   value=00041.69 1648375715611348000
temperature,host=laptop,sensor=CPU-Proximity   value=00043.12 1648375715611348000
temperature,host=laptop,sensor=SSD-Cooked-Max  value=00037.47 1648375715611348000
temperature,host=laptop,sensor=WiFi-Proximity  value=00041.81 1648375715611348000
fan,host=laptop,fan=Fan1 rpm=01838.55,percent=000.1 1648375715611348000
fan,host=laptop,fan=Fan2 rpm=01702.76,percent=000.1 1648375715611348000
```


### Source
* https://github.com/lavoiesl/osx-cpu-temp
* https://github.com/hholtmann/smcFanControl/tree/master/smc-command
* https://github.com/dkorunic/iSMC
