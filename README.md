# RFID_Tag_Scanner
This is a self contained "appliance" that can be used for testing ISO15693 RFID tags and optionally
subscribing to an MQTT broker to monitor any published messages (which might not be RFID related).

RFID input is via a PN5180-NFC scanner board. Output is via a color TFT screen (Output is also mirrored on a web page).
For Initial setup this device sets up its own Access Point hotspot. 
Pointing a web browser at it allows for it to be configured further. (see readme for details)


## Parts List (one of each):
ESP32 DevKit 1 case  .... plus 
2.8 inch TFT angled display box   ... plus 
PN5180-nfc   RFID tag reader board

## Usage
In use RFID tags can be placed on top and the LCD screen will indicate the tag details.

## 3-D Printed Project Box
This is a mash up of several 3D designs. When assembled it makes a desktop box that houses
an ESP32 in a compartment accessed from the bottom, a 2.8 inch color LCD screen in a front bezel,
and a PN5180 RFID scanner module hidden behind a top mounted cover plate.
All other wiring is contained in the box and only the USB connector is available as an external connection (for power). 

## 3D Case parts
Print one of each:
- Item 1 the modified box structure c/w ESP compartment        01 RFID_Scanner_RGB_Box_R0.6.stl
- Item 2 the unmodified LCD lid                                02 TFT_RGB_LCD_28_FRONT.STL
- Item 4 the lid for the ESP32 holder compartment in item 1    04 ESP32_Case_Lid

## And choose one of these to Print 
- Item 3a the modified LCD Lid adapted to mount the PN5180-NFC 03 PN5180-NFC_Cover.stl
- Item 3b An alternate for top mounting the PN5180-NFC         03b PN5180-NFC_Cover_TopMount.stl

# Print Notes:
The main box takes several hours to print. It has designed in supports that are easy to break away.
It is to be printed with the ESP cavity down against the print bed. (the top and front are open).
The other parts are less than an hour each to print and must be printed flat to the print bed.

The easy method for the PN5180-NFC is to attach it with hot glue to the top of the lid using the TopMount version of the lid.
The wires will protrude through the slot and down into the box.

Alternatively the original lid (03 PN5180-NFC_Cover.stl) for the PN5180-NFC has no openings and the PCB will mount
on the inside  of this lid ((again hot glue recommended but screws are also possible.)                 

That lid then attaches to the top of the box using 4 screws (3x10 mm)
The  LCD is 'sandwhiched' between the frony bezel and the box using 4 screws (3x10 mm)


## Wiring:
I used simple dupont F-F connectors.
The wiring could be done individually or in groups according to what you have on hand.
I had a 1o pin F-F cable and used that as a basis. At one end the 10 pin housing was kept 'stock' (LCD end) 
and at the other end the individual wires were removed and reinserted to change thw wire order.
Beyond that individual or multipin dupont housings were used according to what was on hand.
Wire Splices:
There are 4 signals that need splicing, one is 3.3 volt power and the other 3 are to do with SPI communications.

The 3.3volt output from the ESP32 needs to be split into 3 different wires, one goes to the 3 volt supply on the LCD, another 
goes to the backlight LED supply on the LCD, and the third goes to the 3.3 volt supply pin on the PN5180-NFC.
 
Because the SPI interface is used for both the LCD display and the PN5180-nfc the three wires
the three SPI wires (Clock, MISO, and MOSI) need to be split with one going to the LCD, and one to the RFID scanner board.

## Caution:
When sitting on the desk excessive downward presssure can activiate the reset button on the ESP32
You could elect to break away the cover over the reset button - making it a recessed button not subject to accidental activation.
