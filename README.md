### DeepSky

Temp-RH data logger using DFRobot SHT20 sensor

Reads sensor at ```readInterval``` (e.g. every 3 seconds).

Calculates min, max, mean and stdev.

Publishes results to Mosquitto broker at ```uploadInterval``` (e.g. every 1 minute) .

Loop begins at :00 seconds of next minute, to get nice timeseries.
