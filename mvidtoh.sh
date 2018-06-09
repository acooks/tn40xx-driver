#!/bin/bash

# Version 0.2, 2017/11/05
# Support for AQR105 FW conversion added

# Version 0.1, 2017/10/15
# Initial release

if [ $# -ne 3 ]; then
	echo "Usage: mvidtoh.sh <HDR file> <PHY NAME> <H file>"
	exit 1
fi

HDR_FILE=$1
PHY_NAME=$2
H_FILE=$3

if [ ! -e $HDR_FILE ]; then
	echo "mvidtoh: HDR file $HDR_FILE not found"
	exit 1
fi

if [ $PHY_NAME != "MV88X3120" ] && [ $PHY_NAME != "MV88X3310" ] && [ $PHY_NAME != "MV88E2010" ] && [ $PHY_NAME != "AQR105" ]; then
	echo "mvidtoh: PHY $PHY_NAME not supported. Expecting one of AQR105, MV88X3120, MV88X3310 or MV88E2010"
	exit 1
fi

len=`xxd -i $HDR_FILE | grep _len | awk '{print $5}'`

/bin/rm -f $H_FILE
echo "" > $H_FILE
echo "#ifndef _${PHY_NAME}_PHY_H" >> $H_FILE
echo "#define _${PHY_NAME}_PHY_H" >> $H_FILE
echo "" >> $H_FILE

if [ $PHY_NAME == "AQR105" ]; then
	echo "unsigned int ${PHY_NAME}_phy_firmware_len = ${len}" >> $H_FILE
	echo "static u8 ${PHY_NAME}_phy_firmware[] __initdata = {" >> $H_FILE
	echo "/* $HDR_FILE */" >> $H_FILE
	xxd -i -c 8 $HDR_FILE | grep -v "unsigned" >> $H_FILE
	echo "#endif" >> $H_FILE
else
	echo "static u16 ${PHY_NAME}_phy_initdata[] __initdata = {" >> $H_FILE
	echo "/* $HDR_FILE */" >> $H_FILE
	xxd -i -c 2 $HDR_FILE | sed 's/, 0x//' | grep -v "unsigned" >> $H_FILE
	echo "unsigned int ${PHY_NAME}_phy_initdata_len = ${len}" >> $H_FILE
	echo "#endif" >> $H_FILE
fi

exit 0
