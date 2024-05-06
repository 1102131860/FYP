import * as echarts from '../../ec-canvas/echarts';
const app = getApp()
var mychart = null;

Page({
  data: {
    inputText: 'mootek',
    name: '',
    connectedDeviceId: '',
    services: [],
    characteristics: [],
    notifyPair: [], // this records the object which components {charateristicUUID, charateristicProperty, serviceUUID} 
    buffer: [],
    connected: true,
    ec: {             // This is used to load the graphic data
      //onInit: initChart
      onInit: function (canvas, width, height) { 
        chart = echarts.init(canvas, null, {
          width: width,
          height: height
        });
        canvas.setChart(chart);
        return chart;
      },
      lazyLoad: true
    },
    timer:'' // This is use to update the canvas chart
  },
  bindInput: function (e) {
    this.setData({
      inputText: e.detail.value
    })
    console.log(e.detail.value)
  },
  Send: function () {
    var that = this
    if (that.data.connected) {
      var buffer = new ArrayBuffer(that.data.inputText.length)
      var dataView = new Uint8Array(buffer)
      for (var i = 0; i < that.data.inputText.length; i++) {
        dataView[i] = that.data.inputText.charCodeAt(i)
      }

      wx.writeBLECharacteristicValue({
        deviceId: that.data.connectedDeviceId,
        serviceId: that.data.services[0].uuid,
        characteristicId: that.data.characteristics[1].uuid,
        value: buffer,
        success: function (res) {
          console.log('发送指令成功:'+ res.errMsg)
          wx.showModal({
            title: '数据发送成功',
            content: ''
          })        
        },
        fail: function (res) {
          // fail
          //console.log(that.data.services)
          console.log('message发送失败:' +  res.errMsg)
          wx.showToast({
            title: '数据发送失败，请稍后重试',
            icon: 'none'
          })
        }       
      })
    }
    else {
      wx.showModal({
        title: '提示',
        content: '蓝牙已断开',
        showCancel: false,
        success: function (res) {
          that.setData({
            searching: false
          })
        }
      })
    }
  },
  onLoad: function (options) {
    let that = this
    console.log(options)
    that.setData({
      name: options.name,
      connectedDeviceId: options.connectedDeviceId
    })
    // change the MTU dataset
    wx.setBLEMTU({
      deviceId: options.connectedDeviceId,
      mtu: 512,
    })
    // to further get the services
    that.getBLEDeviceServices() 
    // to update the line graph
    that.init_chart(mychart);       // initalize first
    if (!mychart) {
      console.log("Mychart still hasn't be initialized")
      that.init_chart(mychart);
    } else {
      that.setData({                    // fresh every 5 seconds
        timer: setInterval(() => {
          that.getOption(mychart)
        }, 5000)
      })
    }
  },
  onReady: function () {
    
  },
  onUnload: function (){
    clearInterval(this.data.timer) // cancel time listenor
  },
  onShow: function () {

  },
  onHide: function () {
    var that = this
    that.setData({
      inputText: 'mootek',
      name: '',
      connectedDeviceId: '',
      services: [],
      characteristics: [],
      notifyPair: [], 
      buffer: [],
      connected: false
    })
  },
  getBLEDeviceServices: function() {
    let that = this
    console.log(that.data.connectedDeviceId)
    wx.getBLEDeviceServices({
      deviceId: that.data.connectedDeviceId,
      success: function (res) {
        console.log(res.services)
        that.setData({
          services: res.services
        })
        for (let i = 0; i < that.data.services.length; i++){
          if (that.data.services[i].uuid.startsWith("00001800") || that.data.services[i].uuid.startsWith("00001801")){
            continue
          }
          that.getBLEDeviceCharacteristics(that.data.services[i].uuid); // to further get the characteristcis feature
        }
      },
      fail: function(res){
        console.log('getBLEDeviceServices is error', res)
      }
    })
  },
  getBLEDeviceCharacteristics: function(givenServiceUUID){
    let that = this
    console.log(givenServiceUUID)
    wx.getBLEDeviceCharacteristics({
      deviceId: that.data.connectedDeviceId,
      serviceId: givenServiceUUID,
      success: function (res) {
        console.log(res.characteristics)
        let newCharacteristics = [...that.data.characteristics, ...res.characteristics] // 展开符号运算
        that.setData({
          characteristics: newCharacteristics
        })
        console.log(that.data.characteristics)
        for (let i = 0; i < res.characteristics.length; i++){ // to build the searching characteristics
          let oneCharacteristicsPair = {
            charateristicUUID: res.characteristics[i].uuid, 
            charateristicProperty: res.characteristics[i].properties,
            serviceUUID: givenServiceUUID
          }
          let newNotifyPair = [...that.data.notifyPair, oneCharacteristicsPair] // 展开符号运算
          that.setData({
            notifyPair: newNotifyPair
          })
          console.log(that.data.notifyPair)
        }
      },
      fail: function(res){
        console.log('getBLEDeviceCharacteristics is error', res)
      }
    })
    // // start read and notify
    // that.StartNotify()
  },
  StartNotify: function(){
    let that = this
    console.log(that.data.notifyPair)
    for (let i = 0; i < that.data.notifyPair.length; i++){
      let item = that.data.notifyPair[i]
      if (item.charateristicProperty.notify || item.charateristicProperty.indicate){ // 表示该部分可持续通知
        wx.notifyBLECharacteristicValueChange({
          deviceId: that.data.connectedDeviceId,
          serviceId: item.serviceUUID,
          characteristicId: item.charateristicUUID,
          state: true,
          type:"notification", // default is "indication (with ack)" 
          success: function (res) {
            console.log("Start notifying successfully " + item.charateristicUUID)
          },
          fail: function () {
            console.log('Fail to notify ' + item.charateristicUUID)
          }
        })
      }
    }
    // start to listen to the change of the characteristic values
    setTimeout(() => { // delay a moment
      that.onBLECharacteristicValueChange(); //这个是自己定义的函数
    }, 5000) // dalay 3 s
  },
  onBLECharacteristicValueChange: function() {
    let that = this;
    wx.onBLECharacteristicValueChange(function(res) {
      console.log(res)
      const idx = app.inArray(that.data.buffer, 'uuid', res.characteristicId)
      // let newbuffer = [...that.data.buffer] // 拓展运算来实现浅复制
      const data = {}
      if (idx === -1) {
        // newbuffer[that.data.buffer.length] = {
        data[`buffer[${that.data.buffer.length}]`] = {
          uuid: res.characteristicId,
          value: app.buf2Int16(res.value) // it is a list
        }
      } else {
        // newbuffer[idx] = {
        data[`buffer[${idx}]`] = {
          uuid: res.characteristicId,
          value: app.buf2Int16(res.value) // it is a list
        }
      }
      // console.log(newbuffer)
      console.log(data)
      // that.setData({
      //   buffer: newbuffer
      // })
      that.setData(data)
      console.log(that.data.buffer)
    })
  },
  offNotify: function(){
    wx.offBLECharacteristicValueChange({
      success: function(res) {
        console.log("quit wx.offBLECharacteristicValueChange successfully");
        console.log(JSON.stringify(res));
      },
      fail(res) {
        console.log("wx.offBLECharacteristicValueChange fails  " + res);
      }
    });
  },
  closeBLEConnection: function(){
    wx.closeBLEConnection({
      deviceId: this.data.connectedDeviceId,
      success (res) {
        console.log(res)
      }
    })
    var that = this
    that.setData({
      inputText: 'mootek',
      name: '',
      connectedDeviceId: '',
      services: [],
      characteristics: [],
      notifyPair: [], 
      buffer: [],
      connected: true
    })
    wx.navigateBack({
      delta: 1 // return to last page
    })
  },
  getOption : function(mychart){
    let that = this
    let x_data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
    let y_data = []
    for (let i = 0; i < x_data.length; i++){
      y_data.push(Math.floor(Math.random() * (200 - 10 + 1)) + 10)
    }
    var chart = mychart; // asign chart = mychart
    console.log(mychart)
    mychart.clear(); // clear up
    mychart.setOption(that.line(x_data, y_data)); // we only setOption at here
    // that.line(x_data, y_data)
  },
  init_chart: function (mychart) {
    this.Component = this.selectComponent('#mychart');
    this.Component.init((canvas, width, height) => {
      const chart = echarts.init(canvas, null, {
          width: width,
          height: height
      });
      canvas.setChart(chart);
      mychart = chart;
      console.log("My chart is", mychart);
      return mychart; // here we return mychart instead of chart
    });
  },
  line : function (xdata, ydata)  { //Line Graph
    let option = {
      title: {
        text: 'Line Chart',
        left: 'center'
      },
      xAxis: {
        type: 'category',
        data: ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12']  //[...new Array(this.data.buffer[0].value.length).keys()] // form a length
      },
      yAxis: {
        type: 'value'
      },
      series: [{
        data: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12], //ydata, //this.data.buffer[0].value,
        type: 'line'
      }]
    };
    console.log(option)
    return option; // Now we only return option here, and mychart.setOption(this.line()) outside
    //mychart.setOption(option); // when we call the line function, we use setTimeOut instead of directly setTimeOut here
  }
})

