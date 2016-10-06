//Server main. To be used on RPi.

var express = require('express');
var fs = require('fs');
var SerialPort = require('serialport');

var port = new SerialPort('/dev/ttyACM0', {
  baudRate: 115200,
  parser: SerialPort.parsers.readline('\n')
});

port.on('data', function(data) {
  console.log('Data: ' + data);
});
