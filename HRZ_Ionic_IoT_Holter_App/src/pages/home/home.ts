import { Component } from '@angular/core';
import { NavController } from 'ionic-angular';
import { BLE } from '@ionic-native/ble';
import { LoadingController } from 'ionic-angular';
import { DevicePage } from '../device/device';

@Component({
  selector: 'page-home',
  templateUrl: 'home.html'
})
export class HomePage {
  isScanning: boolean;
  devices: any[];

  constructor(public navCtrl: NavController, private ble: BLE, public loadingCtrl: LoadingController) {
    this.devices = [];
    this.isScanning = false;
  }

  startScanning() {
    let loader = this.loadingCtrl.create({
      content: "Scanning devices...",
    });
    loader.present();
    this.devices = [];
    this.isScanning = true;
    this.ble.startScan([]).subscribe(device => {
      this.devices.push(device);
    });

    setTimeout(() => {
      this.ble.stopScan().then(() => {
        console.log('Scanning has stopped');
        console.log(JSON.stringify(this.devices))
        this.isScanning = false;
        loader.dismiss();
      });
    }, 500);
  }

  connectToDevice(device) {
    console.log('Connecting to device...');
    console.log(JSON.stringify(device))
    let loader = this.loadingCtrl.create({
      content: "Connecting...",
    });
    loader.present();
    this.ble.connect(device.id).subscribe(result => {
        console.log('Connected')
        console.log(JSON.stringify(result));
        loader.dismiss();
        this.navCtrl.push(DevicePage, {
          device: device,
          characteristics: result.characteristics
        });
      },
      err => {
        console.log('Error connecting');
        loader.dismiss();
      });
    }

  }
