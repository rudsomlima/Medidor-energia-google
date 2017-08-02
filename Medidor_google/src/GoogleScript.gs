//-----------------------------------------------
//Originally published by Mogsdad@Stackoverflow
//Modified for jarkomdityaz.appspot.com
//-----------------------------------------------
/*

  Modified by PDAControl for read Onewire + ESP8266

  http://pdacontrolenglish.blogspot.com.co/2016/06/esp8266-direct-connection-to-google.html
  http://pdacontrol.blogspot.com.co/2016/06/conexion-esp8266-directa-google.html
  https://youtu.be/5f7wCeD4gB4  -- TUTORIAL
  https://youtu.be/YgMl30IDrxw

*/


/* Using spreadsheet API */

var SS = SpreadsheetApp.openById('1j5UX_r9JBG_qLsKYpLnlgqdZgXSkF1VC8L_mt7iAhgI');
//var sheet = SS.getSheetByName('Sheet1');
//var str = "";

var timezone = "GMT-3";
//var timestamp_format = "MM-dd-yyyy"; // Timestamp Format.
var timestamp_format = "yyyy-MM-dd HH:mm:ss";
//var formattedDate = Utilities.formatDate(time, "GMT", "MM-dd-yyyy HH:mm:ss");

function onOpen(){
  var ui = SpreadsheetApp.getUi();
  ui.createMenu('ESP8266')
  .addItem('Clear', 'Clear')
  .addToUi();
}

function Clear(){
  var id = '1Xutd4MbetPmYu1b0koJ0l2Xst7BlFYHRA86YbIsqDUk'; // Spreadsheet ID
  var SS = SpreadsheetApp.openById(id);
  var sheet = SpreadsheetApp.openById(id).getActiveSheet();
  //var settingsSheet = spreadsheet.getSheetByName('Settings');
  sheet.deleteRows(1, sheet.getLastRow());
  SS.toast('Chart cleared', 'ESP8266', 5);
}


function doPost(e) {
  var val = e.parameter.value;
  if (e.parameter.value !== undefined){
    var range = sheet.getRange('A2');
    range.setValue(val);
  }
}



function doGet(e) {
  Logger.log( JSON.stringify(e) );  // view parameters

  var result = 'Ok'; // assume success

  if (e.parameter == undefined) {
    result = 'No Parameters';
  }
  else {
    var id = '1Xutd4MbetPmYu1b0koJ0l2Xst7BlFYHRA86YbIsqDUk'; // Spreadsheet ID
    var sheet = SpreadsheetApp.openById(id).getActiveSheet();
    var newRow = sheet.getLastRow() + 1;
    var rowData = [];
    //var waktu = new Date();
 //   rowData[0] = new Date(); // Timestamp in column A

    var date = Utilities.formatDate(new Date(), timezone, timestamp_format);


    for (var param in e.parameter) {
      Logger.log('In for loop, param='+param);
      var value = stripQuotes(e.parameter[param]);
      //Logger.log(param + ':' + e.parameter[param]);
      switch (param) {
        case 'now': //Parameter
           var range = sheet.getRange('G2'); ///// ESTADO ACTUAL ESP
           range.setValue(value);
           rowData[0] = ''  ;
           break;
        case 'value': //Parameter
          rowData[1] = value; //Value in column B
          rowData[0] =  date;
          //var range = sheet.getRange('D2');   ////inserta valor actual
          //range.setValue(value);
         // var range = sheet.getRange('E2');  // insterta datetime
          //range.setValue(date);              // insterta datetime

          break;
       case 'column_C':
           rowData[2] = value;
           break;
       case 'read':
           return ContentService.createTextOutput(sheet.getRange('F2').getValue());   /// envia parametro a ESP8266
           break;
       default:
          result = "unsupported parameter";
      }
    }
    Logger.log(JSON.stringify(rowData));

    // Write new row below
    var newRange = sheet.getRange(newRow, 1, 1, rowData.length);
    newRange.setValues([rowData]);
  }

  // Return result of operation
  return ContentService.createTextOutput(result);
}

/**
* Remove leading and trailing single or double quotes
*/
function stripQuotes( value ) {
  return value.replace(/^["']|['"]$/g, "");
}
