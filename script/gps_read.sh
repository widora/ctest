# !bin/sh
#
# getloc - read GPS location and echo status, latitude and longitude
#          separated by a space. Status 0 is success.
#
# Original Source:  http://lakenine.com/category/devices/gps/
# Coordinate confirm website: http://www.gpsspg.com/maps.htm
#

UART_PORT=/dev/ttyUSB0

closePort()
{
  exec 6<&-
}
 
openPort()
{
  #exec 6</dev/ttyUSB0
  exec 6<$UART_PORT
}
 
# Pass the GPS value and direction (N/S/E/W) to get the
# decimal latitude/longitude.
gpsDecimal() {
    gpsVal=$1
    gpsDir=$2
    # Integer part of the lat/long
    gpsInt=`echo "scale=0;$gpsVal/100" | bc`
    # Minutes part of the lat/long
    gpsMin=`echo "scale=3;$gpsVal-100*$gpsInt" | bc`
    # Convert minutes to a full decimal value
    gpsDec=`echo "scale=5;$gpsInt+$gpsMin/60" | bc`
    # South and West are negative
    if [[ $gpsDir = 'W' || $gpsDir = 'S' ]]
    then
      gpsDec="-$gpsDec"
    fi
    echo $gpsDec
}
 
# Return statuses
STATUS_OK=0
STATUS_NOFIX_SATDATA=1
STATUS_TIMEOUT=2
STATUS_NOTFOUND=3
STATUS_NOFIX_LOCDATA=4
 
openPort

while [ 1 ] #---BIG LOOP ---------------------------
do

# Status and counter values
foundReliability='false'
foundLocation='false'
linesRead=0

while [[ $linesRead -le 100 ]] && [[ "$foundReliability" = "false" || "$foundLocation" = "false" ]]  
do
  # Read the next line from the GPS port, with a timeout of 10 seconds.
  read -t 10 RESPONSE <&6
  #echo $RESPONSE
  if [[ $? -eq 1 ]]
  then
    # Read timed out so bail with error
    closePort
    echo "status_timeout:$STATUS_TIMEOUT 0 0"
#   exit 1
  fi
 
  # Fallthrough: line was read. Count it because we have a threshhold
  # for the number of sentences to process before giving up.
  #linesRead=`expr $linesRead + 1`

  # Get the sentence type.
  sentenceType=`echo $RESPONSE | cut -d ',' -f 1`
  #echo "------type:  $sentenceType"
 
  #---------  get UTC time  ---------------
  if [[ "$sentenceType" = "\$GNGGA" ]] #--- GPGGA!!!
  then
     utctime=`echo $RESPONSE | cut -d ',' -f 2`
     echo UTC TIME: ${utctime:0:2}:${utctime:2:2}:${utctime:4:2}
     satnum=`echo $RESPONSE | cut -d ',' -f 8` 
     echo Satllites Used: $satnum 
  fi

  #------------  get GPGSA ------------	
  if [[ "$sentenceType" = "\$GPGSA"  &&  "$foundReliability" == "false" ]]
  then
    # Found the "fix information" sentence; see if the reliability
    # is at least 2.
    fixValue=`echo $RESPONSE | cut -d ',' -f 3`
    if [[ $fixValue -ne 2 && $fixValue -ne 3 ]]
    then
      # Insufficient fix quality so bail with error
      closePort
      echo "status_nofix_satdata: $STATUS_NOFIX_SATDATA 0 0"
#     exit 1
    fi
    # Fallthrough: reliability is sufficient
    foundReliability='true'
  fi # GPGSA sentence
 
  #-------- GPS model give GNRMC instead of GPRMC ---------------!!!!!!!!!!!!
  if [[ "$sentenceType" = "\$GNRMC" ]] && [[ "$foundLocation" = "false" ]]
  then
    #echo " ---------- get GPRMC ------------"
    # Found the "recommended minimum data" (GPRMC) sentence;
    # determine if it's "active", which means "valid".
    #
    statusValue=`echo $RESPONSE | cut -d ',' -f 3`
    if [[ $statusValue = 'V' ]]
    then
      # Void status; can't use the reading so bail
      closePort
      echo "status_nofix_locdata:$STATUS_NOFIX_LOCDATA 0 0"
#     exit 1
    fi
 
    # Fallthrough: active status, so we can use the reading.
    foundLocation='true'
    latitudeValue=`echo $RESPONSE | cut -d ',' -f 4`
    latitudeNS=`echo $RESPONSE | cut -d ',' -f 5`
    latitudeDec=$(gpsDecimal $latitudeValue $latitudeNS)
    longitudeValue=`echo $RESPONSE | cut -d ',' -f 6`
    longitudeEW=`echo $RESPONSE | cut -d ',' -f 7`
    longitudeDec=$(gpsDecimal $longitudeValue $longitudeEW)

    #echo  latitudeNS:"$latitudeNS" --- latitudeDec:"$latitudeDec" --- longitudeEW:"$longitudeEW" --- longitudeDec:"$longitudeDec"

  fi  #$GPRMC sentence
done # read-line loop
 
# If we get to here and location and reliability were OK, we
# have a fix.
if [[ $foundReliability = 'true' && $foundLocation = 'true' ]]
then
  #echo "status:$STATUS_OKc"
  #--Ok-- mtime=$(date +%T)
  mtime=`date +%T`
  echo "$mtime Google Earth GPS: $latitudeDec$latitudeNS  $longitudeDec$longitudeEW"
# exit 0
fi

done  #---BIG LOOP------------------------------------

closePort

# Fallthrough to here means too many lines were read without
# finding location information. Return failure.
echo "status_notfound: $STATUS_NOTFOUND 0 0"
exit 1

