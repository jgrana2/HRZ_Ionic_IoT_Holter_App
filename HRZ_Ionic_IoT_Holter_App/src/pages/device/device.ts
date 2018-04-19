import { Component, ChangeDetectorRef} from '@angular/core';
import { NavController } from 'ionic-angular';
import { NavParams } from 'ionic-angular';
import { BLE } from '@ionic-native/ble';
import SmoothieChart from 'smoothie';
import { Socket } from 'ng-socket-io';

let channel1TimeSeries = new SmoothieChart.TimeSeries();
let channel2TimeSeries = new SmoothieChart.TimeSeries();
let channel3TimeSeries = new SmoothieChart.TimeSeries();
let channel4TimeSeries = new SmoothieChart.TimeSeries();
let channel5TimeSeries = new SmoothieChart.TimeSeries();
let channel6TimeSeries = new SmoothieChart.TimeSeries();
let channel7TimeSeries = new SmoothieChart.TimeSeries();
let channel8TimeSeries = new SmoothieChart.TimeSeries();
var time1, time2, time3, time4, time5, time6, time7, time8;
var millisPerPixelVal = 8;
var samplePeriod = 4; //Sample period in ms
var sampleSize = 4; //Size of each sample in bytes
var nSamplesPerUpload = 120;

@Component({
  selector: 'page-device',
  templateUrl: 'device.html'
})
export class DevicePage{
  device: {id: string};
  characteristics:any[];
  status: string;
  channel1: any[];
  channel2: any[];
  channel3: any[];
  channel4: any[];
  channel5: any[];
  channel6: any[];
  channel7: any[];
  channel8: any[];

  constructor(public navCtrl: NavController, public navParams: NavParams, private ble: BLE,
    private chRef: ChangeDetectorRef, private socket: Socket){
      this.device = this.navParams.get('device');
      this.characteristics = this.navParams.get('characteristics');
      this.channel1 = [];
      this.channel2 = [];
      this.channel3 = [];
      this.channel4 = [];
      this.channel5 = [];
      this.channel6 = [];
      this.channel7 = [];
      this.channel8 = [];
    }

    ionViewDidLoad(){
      this.createTimeline();

      //Start all channels at the same time
      time1 = new Date().getTime();
      time2 = time1;
      time3 = time1;
      time4 = time1;
      time5 = time1;
      time6 = time1;
      time7 = time1;
      time8 = time1;

      //Start reading data
      this.readChannel1();
      this.readChannel2();
      this.readChannel3();
      this.readChannel4();
      this.readChannel5();
      this.readChannel6();
      this.readChannel7();
      this.readChannel8();

    }

    createTimeline(){
      let chart1 = new SmoothieChart.SmoothieChart({millisPerPixel:millisPerPixelVal, grid:{fillStyle:'#ffffff',strokeStyle:'#f4f4f4'}});
      chart1.addTimeSeries(channel1TimeSeries, {lineWidth:0.9,strokeStyle:'#000000'});
      chart1.streamTo(<HTMLCanvasElement> document.getElementById("chart1"), 500);
      let chart2 = new SmoothieChart.SmoothieChart({millisPerPixel:millisPerPixelVal, grid:{fillStyle:'#ffffff',strokeStyle:'#f4f4f4'}});
      chart2.addTimeSeries(channel2TimeSeries, {lineWidth:0.9,strokeStyle:'#000000'});
      chart2.streamTo(<HTMLCanvasElement> document.getElementById("chart2"), 500);
      let chart3 = new SmoothieChart.SmoothieChart({millisPerPixel:millisPerPixelVal, grid:{fillStyle:'#ffffff',strokeStyle:'#f4f4f4'}});
      chart3.addTimeSeries(channel3TimeSeries, {lineWidth:0.9,strokeStyle:'#000000'});
      chart3.streamTo(<HTMLCanvasElement> document.getElementById("chart3"), 500);
      let chart4 = new SmoothieChart.SmoothieChart({millisPerPixel:millisPerPixelVal, grid:{fillStyle:'#ffffff',strokeStyle:'#f4f4f4'}});
      chart4.addTimeSeries(channel4TimeSeries, {lineWidth:0.9,strokeStyle:'#000000'});
      chart4.streamTo(<HTMLCanvasElement> document.getElementById("chart4"), 500);
      let chart5 = new SmoothieChart.SmoothieChart({millisPerPixel:millisPerPixelVal, grid:{fillStyle:'#ffffff',strokeStyle:'#f4f4f4'}});
      chart5.addTimeSeries(channel5TimeSeries, {lineWidth:0.9,strokeStyle:'#000000'});
      chart5.streamTo(<HTMLCanvasElement> document.getElementById("chart5"), 500);
      let chart6 = new SmoothieChart.SmoothieChart({millisPerPixel:millisPerPixelVal, grid:{fillStyle:'#ffffff',strokeStyle:'#f4f4f4'}});
      chart6.addTimeSeries(channel6TimeSeries, {lineWidth:0.9,strokeStyle:'#000000'});
      chart6.streamTo(<HTMLCanvasElement> document.getElementById("chart6"), 500);
      let chart7 = new SmoothieChart.SmoothieChart({millisPerPixel:millisPerPixelVal, grid:{fillStyle:'#ffffff',strokeStyle:'#f4f4f4'}});
      chart7.addTimeSeries(channel7TimeSeries, {lineWidth:0.9,strokeStyle:'#000000'});
      chart7.streamTo(<HTMLCanvasElement> document.getElementById("chart7"), 500);
      let chart8 = new SmoothieChart.SmoothieChart({millisPerPixel:millisPerPixelVal, grid:{fillStyle:'#ffffff',strokeStyle:'#f4f4f4'}});
      chart8.addTimeSeries(channel8TimeSeries, {lineWidth:0.9,strokeStyle:'#000000'});
      chart8.streamTo(<HTMLCanvasElement> document.getElementById("chart8"), 500);
    }

    connectToCharacteristic(deviceId, characteristic){
      console.log('Connect to Characteristic');
      console.log(deviceId);
      console.log(JSON.stringify(characteristic));
    }

    readStatus(){
      this.ble.startNotification(this.device.id, '805B', '8170').subscribe(result => {
        let data = new Uint8Array(result);
        this.status = this.buf2hex(data);
        this.chRef.detectChanges();
      },
      err => {
        console.log('Error starting notification');
      });
    }

    readChannel1(){
      this.ble.startNotification(this.device.id, '805B', '8171').subscribe(result => {
        let data = new Uint8Array(result);
        for (var i = 0; i < data.length; i+=sampleSize) {
          let sampleValue = data[i+3] + (data[i+2] << 8) + (data[i+1] << 16) + (data[i] << 24);
          time1 += samplePeriod;
          channel1TimeSeries.append(time1, sampleValue);
          this.channel1.push(sampleValue);
        }
        if (this.channel1.length >= nSamplesPerUpload*data.byteLength/4) {
            let jsonData = {'device_id':'j70Py2JyWo','channel':1,'time_stamp':new Date(),'n_samples':nSamplesPerUpload*data.byteLength/4,'n_bits':32,'filtered':false,'data': this.channel1};
            this.socket.emit("channel1", jsonData);
            this.channel1 = [];
        }
        this.chRef.detectChanges();
      },
      err => {
        console.log('Error starting notification');
      });
    }
    readChannel2(){
      this.ble.startNotification(this.device.id, '805B', '8172').subscribe(result => {
        let data = new Uint8Array(result);
        for (var i = 0; i < data.length; i+=sampleSize) {
          let sampleValue = data[i+3] + (data[i+2] << 8) + (data[i+1] << 16) + (data[i] << 24);
          time2 += samplePeriod;
          channel2TimeSeries.append(time2, sampleValue);
          this.channel2.push(sampleValue);
        }
        if (this.channel2.length >= nSamplesPerUpload*data.byteLength/4) {
            let jsonData = {'device_id':'j70Py2JyWo','channel':2,'time_stamp':new Date(),'n_samples':nSamplesPerUpload*data.byteLength/4,'n_bits':32,'filtered':false,'data': this.channel2};
            this.socket.emit("channel2", jsonData);
            this.channel2 = [];
        }
        this.chRef.detectChanges();
      },
      err => {
        console.log('Error starting notification');
      });
    }
    readChannel3(){
      this.ble.startNotification(this.device.id, '805B', '8173').subscribe(result => {
        let data = new Uint8Array(result);
        for (var i = 0; i < data.length; i+=sampleSize) {
          let sampleValue = data[i+3] + (data[i+2] << 8) + (data[i+1] << 16) + (data[i] << 24);
          time3 += samplePeriod;
          channel3TimeSeries.append(time3, sampleValue);
          this.channel3.push(sampleValue);
        }
        if (this.channel3.length >= nSamplesPerUpload*data.byteLength/4) {
            let jsonData = {'device_id':'j70Py2JyWo','channel':3,'time_stamp':new Date(),'n_samples':nSamplesPerUpload*data.byteLength/4,'n_bits':32,'filtered':false,'data': this.channel3};
            this.socket.emit("channel3", jsonData);
            this.channel3 = [];
        }
        this.chRef.detectChanges();
      },
      err => {
        console.log('Error starting notification');
      });
    }
    readChannel4(){
      this.ble.startNotification(this.device.id, '805B', '8174').subscribe(result => {
        let data = new Uint8Array(result);
        for (var i = 0; i < data.length; i+=sampleSize) {
          let sampleValue = data[i+3] + (data[i+2] << 8) + (data[i+1] << 16) + (data[i] << 24);
          time4 += samplePeriod;
          channel4TimeSeries.append(time4, sampleValue);
          this.channel4.push(sampleValue);
        }
        if (this.channel4.length >= nSamplesPerUpload*data.byteLength/4) {
            let jsonData = {'device_id':'j70Py2JyWo','channel':4,'time_stamp':new Date(),'n_samples':nSamplesPerUpload*data.byteLength/4,'n_bits':32,'filtered':false,'data': this.channel4};
            this.socket.emit("channel4", jsonData);
            this.channel4 = [];
        }
        this.chRef.detectChanges();
      },
      err => {
        console.log('Error starting notification');
      });
    }
    readChannel5(){
      this.ble.startNotification(this.device.id, '805B', '8175').subscribe(result => {
        let data = new Uint8Array(result);
        for (var i = 0; i < data.length; i+=sampleSize) {
          let sampleValue = data[i+3] + (data[i+2] << 8) + (data[i+1] << 16) + (data[i] << 24);
          time5 += samplePeriod;
          channel5TimeSeries.append(time5, sampleValue);
          this.channel5.push(sampleValue);
        }
        if (this.channel5.length >= nSamplesPerUpload*data.byteLength/4) {
            let jsonData = {'device_id':'j70Py2JyWo','channel':5,'time_stamp':new Date(),'n_samples':nSamplesPerUpload*data.byteLength/4,'n_bits':32,'filtered':false,'data': this.channel5};
            this.socket.emit("channel5", jsonData);
            this.channel5 = [];
        }
        this.chRef.detectChanges();
      },
      err => {
        console.log('Error starting notification');
      });
    }
    readChannel6(){
      this.ble.startNotification(this.device.id, '805B', '8176').subscribe(result => {
        let data = new Uint8Array(result);
        for (var i = 0; i < data.length; i+=sampleSize) {
          let sampleValue = data[i+3] + (data[i+2] << 8) + (data[i+1] << 16) + (data[i] << 24);
          time6 += samplePeriod;
          channel6TimeSeries.append(time6, sampleValue);
          this.channel6.push(sampleValue);
        }
        if (this.channel6.length >= nSamplesPerUpload*data.byteLength/4) {
            let jsonData = {'device_id':'j70Py2JyWo','channel':6,'time_stamp':new Date(),'n_samples':nSamplesPerUpload*data.byteLength/4,'n_bits':32,'filtered':false,'data': this.channel6};
            this.socket.emit("channel6", jsonData);
            this.channel6 = [];
        }
        this.chRef.detectChanges();
      },
      err => {
        console.log('Error starting notification');
      });
    }
    readChannel7(){
      this.ble.startNotification(this.device.id, '805B', '8177').subscribe(result => {
        let data = new Uint8Array(result);
        for (var i = 0; i < data.length; i+=sampleSize) {
          let sampleValue = data[i+3] + (data[i+2] << 8) + (data[i+1] << 16) + (data[i] << 24);
          time7 += samplePeriod;
          channel7TimeSeries.append(time7, sampleValue);
          this.channel7.push(sampleValue);
        }
        if (this.channel7.length >= nSamplesPerUpload*data.byteLength/4) {
            let jsonData = {'device_id':'j70Py2JyWo','channel':7,'time_stamp':new Date(),'n_samples':nSamplesPerUpload*data.byteLength/4,'n_bits':32,'filtered':false,'data': this.channel7};
            this.socket.emit("channel7", jsonData);
            this.channel7 = [];
        }
        this.chRef.detectChanges();
      },
      err => {
        console.log('Error starting notification');
      });
    }
    readChannel8(){
      this.ble.startNotification(this.device.id, '805B', '8178').subscribe(result => {
        let data = new Uint8Array(result);
        for (var i = 0; i < data.length; i+=sampleSize) {
          let sampleValue = data[i+3] + (data[i+2] << 8) + (data[i+1] << 16) + (data[i] << 24);
          time8 += samplePeriod;
          channel8TimeSeries.append(time8, sampleValue);
          this.channel8.push(sampleValue);
        }
        if (this.channel8.length >= nSamplesPerUpload*data.byteLength/4) {
            let jsonData = {'device_id':'j70Py2JyWo','channel':8,'time_stamp':new Date(),'n_samples':nSamplesPerUpload*data.byteLength/4,'n_bits':32,'filtered':false,'data': this.channel8};
            this.socket.emit("channel8", jsonData);
            this.channel8 = [];
        }
        this.chRef.detectChanges();
      },
      err => {
        console.log('Error starting notification');
      });
    }


    buf2hex(buffer){ // buffer is an ArrayBuffer
      return Array.prototype.map.call(new Uint8Array(buffer), x => ('00' + x.toString(16)).slice(-2)).join('');
    }
  }
