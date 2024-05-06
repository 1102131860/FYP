//app.js
App({
  buf2hex: function (buffer) {
    return Array.prototype.map.call(new Uint8Array(buffer), x => ('00' + x.toString(16)).slice(-2)).join('')
  },
  buf2string: function (buffer) {
    var arr = Array.prototype.map.call(new Uint8Array(buffer), x => x)
    var str = ''
    for (var i = 0; i < arr.length; i++) {
      str += String.fromCharCode(arr[i])
    }
    return str
  },
  inArray: function (arr, key, val) {
    for (let i = 0; i < arr.length; i++) {
      if (arr[i][key] === val) {
        return i;
      }
    }
    return -1;
  },
  buf2Int8: function(buffer){ // t_size is 8
    let givenString = this.buf2hex(buffer)
    let intList = []
    for(let i = 0; i < givenString.length; i+=2){
      let integer =  parseInt(givenString.substring(i, i+2), 16)
      intList.push(integer)
    }
    return intList
  },
  buf2Int16: function(buffer){ // t_size is 16
    let intList = this.buf2Int8(buffer)
    let newList = []
    for(let i = 0; i < intList.length; i+=2){
      let integer = intList[i] + intList[i+1]*256
      newList.push(integer)
    }
    return newList
  },
  onLaunch: function () {
    this.globalData.SystemInfo = wx.getSystemInfoSync()
    //console.log(this.globalData.SystemInfo)
  },
  globalData: {
    SystemInfo: {}
  }
})