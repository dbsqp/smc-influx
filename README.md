# SMC-Influxdb

Fetch SMC data from macOS Intel hardware and output in InfluxDB line protocol. Designed to be automatically called by a Telegraf exec input plugin.

Code was mofified for my hardware and influxdb structure.

## Usage 

```./smc-influxdb -h
usage: smc-influx [cgfaA]
  -c  CPU temperature
  -g  GPU temperature
  -w  WiFi temperature
  -s  SSD temperature
  -f  fan speeds
  -a  CPU, GPU and fans - same as -cgf
  -A  all temperature and fan metrics
  -h  this info
```

### Compiling

```
make
```
Note will give one warning ralating to kIOMasterPortDefault being deprecated.

### Running

```
./smc-influxdb
```

### Output example

```./smc-influxdb -A
fan,host=laptop,fan=Left          rpm=01830.83,percent=000.00 1648383407464299000
fan,host=laptop,fan=Right         rpm=01696.13,percent=000.00 1648383407464299000
temperature,host=laptop,sensor=CPU             value=00044.69 1648383407464299000
temperature,host=laptop,sensor=CPU-Virtual-1   value=00048.50 1648383407464299000
temperature,host=laptop,sensor=CPU-Virtual-2   value=00049.99 1648383407464299000
temperature,host=laptop,sensor=CPU-Core-1      value=00046.00 1648383407464299000
temperature,host=laptop,sensor=CPU-Core-2      value=00046.00 1648383407464299000
temperature,host=laptop,sensor=CPU-Core-3      value=00045.00 1648383407464299000
temperature,host=laptop,sensor=CPU-Core-4      value=00046.00 1648383407464299000
temperature,host=laptop,sensor=CPU-Core-5      value=00046.00 1648383407464299000
temperature,host=laptop,sensor=CPU-Core-6      value=00046.00 1648383407464299000
temperature,host=laptop,sensor=CPU-Core-7      value=00046.00 1648383407464299000
temperature,host=laptop,sensor=CPU-Core-8      value=00044.00 1648383407464299000
temperature,host=laptop,sensor=GPU             value=00043.25 1648383407464299000
temperature,host=laptop,sensor=GPU-VRAM        value=00042.25 1648383407464299000
temperature,host=laptop,sensor=SSD             value=00038.36 1648383407464299000
temperature,host=laptop,sensor=SSD-Filtered    value=00038.23 1648383407464299000
temperature,host=laptop,sensor=SSD-Drive-0-A   value=00035.47 1648383407464299000
temperature,host=laptop,sensor=SSD-Drive-0-B   value=00036.76 1648383407464299000
temperature,host=laptop,sensor=SSD-Drive-1-A   value=00000.00 1648383407464299000
temperature,host=laptop,sensor=SSD-Drive-1-B   value=00038.36 1648383407464299000
temperature,host=laptop,sensor=Memory          value=00035.96 1648383407464299000
temperature,host=laptop,sensor=Memory-Bank     value=00043.06 1648383407464299000
temperature,host=laptop,sensor=Mainboard       value=00045.31 1648383407464299000
temperature,host=laptop,sensor=WiFi            value=00042.94 1648383407464299000
temperature,host=laptop,sensor=Battery-1       value=00032.50 1648383407464299000
temperature,host=laptop,sensor=Battery-2       value=00032.80 1648383407464299000
temperature,host=laptop,sensor=Ambient         value=00024.20 1648383407464299000
temperature,host=laptop,sensor=Palm-Rest-1     value=00032.00 1648383407464299000
temperature,host=laptop,sensor=Palm-Rest-2     value=00030.00 1648383407464299000
temperature,host=laptop,sensor=Top-Skin        value=00036.52 1648383407464299000
temperature,host=laptop,sensor=Heatpipe-Left   value=00041.38 1648383407464299000
temperature,host=laptop,sensor=Heatpipe-Right  value=00044.06 1648383407464299000
temperature,host=laptop,sensor=PCH-Die         value=00049.00 1648383407464299000
temperature,host=laptop,sensor=PECI-GPU        value=00046.00 1648383407464299000
temperature,host=laptop,sensor=PECI-CPU        value=00047.14 1648383407464299000
temperature,host=laptop,sensor=PECI-MAX        value=00047.14 1648383407464299000
temperature,host=laptop,sensor=PECI-SA         value=00046.00 1648383407464299000
```

## Telegraf Input Plugin

Input plugin definition from telegraf.conf
```
[[inputs.exec]]
  interval = "60s"
  commands = ["/Users/dbsqp/.bin/smc-influxdb -A"]
  timeout = "5s"
  data_format = "influx"
```


### Source
* https://github.com/lavoiesl/osx-cpu-temp
* https://github.com/hholtmann/smcFanControl/tree/master/smc-command
* https://github.com/dkorunic/iSMC
