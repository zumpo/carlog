#include "Logger.h"
#include "Trip.h"
#include "SdHelper.h"
#include "sdfat.h"

Trip trip;
char dateTimeBuf[21]= "0000-00-00T00:00:00Z";
extern SdFat SD;

Logger::Logger (Gps* gps) {
  _gps = gps;  
  recording = false;
}

void Logger::begin() {
  recording = true;
  trip.create();

  // write header gps
  if(trip.gpsLog) {
    trip.gpsLog.println("dateTime,speed,altitude,latitudeDegrees,longitudeDegrees,fix,satellites,rmc,gga");
  }
  
  // write header sys
  if(trip.sysLog) {
    trip.sysLog.println("dateTime,vBat,vUsb");
  }
  
  //init timers
  _log_gps_timer = millis();
  _log_sys_timer = millis();
}

void Logger::log(Gps gps) {
  // if millis() or timer wraps around, we'll just reset it
  if (_log_gps_timer > millis())  _log_gps_timer = millis();
  // approximately every LOG_INTERVAL seconds or so, print out the current stats
  if (millis() - _log_gps_timer < LOG_INTERVAL) {
    return; 
  }
  _log_gps_timer = millis();

  // an open file, both NMEAs received and therefore all required data, lets log
  if(trip.gpsLog && strstr(gps.RMC, "$GPRMC") && strstr(gps.GGA, "$GPGGA")) {
    if(!gpsLogged) {
      Serial.println("Gps logging");
    }
    gpsLogged = true;
    // dateTime,speed,altitude,latitudeDegrees,longitudeDegrees,fix,satellites
    trip.gpsLog.print(_dateTime());trip.gpsLog.write(',');
    trip.gpsLog.print(gps.speed);trip.gpsLog.write(',');
    trip.gpsLog.print(gps.altitude);trip.gpsLog.write(',');
    trip.gpsLog.print(gps.latitudeDegrees, 6);trip.gpsLog.write(',');
    trip.gpsLog.print(gps.longitudeDegrees, 6);trip.gpsLog.write(',');
    trip.gpsLog.print(gps.fix);trip.gpsLog.write(',');
    trip.gpsLog.print(gps.satellites);
    trip.gpsLog.print(gps.RMC);trip.gpsLog.write(',');
    trip.gpsLog.println(gps.GGA);
    trip.gpsLog.flush();
  }
}

void Logger::log(System sys) {
  // if millis() or timer wraps around, we'll just reset it
  if (_log_sys_timer > millis())  _log_sys_timer = millis();
  // approximately every UPDATE_INTERVAL seconds or so, print out the current stats
  if (millis() - _log_sys_timer < LOG_INTERVAL) {
    return; 
  }  
  _log_sys_timer = millis();

  // trip has a log to write to
  if(trip.sysLog) {
    if(!sysLogged) {
      Serial.println("Sys logging");
    }
    sysLogged = true;
   
    trip.sysLog.print(_dateTime());trip.sysLog.write(',');
    trip.sysLog.print(sys.vbat);trip.sysLog.write(',');
    trip.sysLog.println(sys.vusb);
    trip.sysLog.flush();
  }
}

void Logger:: end() {
  Serial.println("Logging ended. Closing log files.");
  trip.gpsLog.close();
  trip.sysLog.close();
  SDHelper.dumpFile(trip.gpsLogPath);
  //SDHelper.dumpFile(trip.sysLogPath);
  if (!gpsLogged) {
    Serial.println("Delete gps");
    SDHelper.deleteFile(trip.gpsLogPath);
  }
  Serial.println("Enter beacon mode and sleep...");

  // reset
  recording = false;
  gpsLogged = false;
  sysLogged = false;
}

char* Logger::_dateTime() {
  //RFC3339DateFormatter setDateFormat:@"yyyy'-'MM'-'dd'T'HH':'mm':'ss'Z"
  if(_gps->fix) {
    sprintf(dateTimeBuf, 
      "20%d-%02d-%02dT%02d:%02d:%02dZ", 
      _gps->year, _gps->month, _gps->day, _gps->hour, _gps->minute, _gps->seconds);
  }
  return dateTimeBuf;   
}


