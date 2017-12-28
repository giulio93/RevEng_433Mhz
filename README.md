## Backeng a 433mhz H/T sensor with an Arduino

In this project i implement a well know hack!
Using a common 433 mhz RF Reciver, i decode a signal coming from a wireless sensor and directed to a weather station.
I just bougth on Amazon a weather station with 3 wireless sensor like this one : 

https://www.bresser.de/en/Weather-Time/Weather-Stations/BRESSER-Temeo-Hygro-Quadro-thermo-and-hygrometer-with-4-independent-measuring-data.html

Then i use a MXRM5V RF reciver , working between 315mhz to 433mhz.

First i use Audacity to analyze and understand the wireless signal probed out by the sensors.
Than i write some code for Arduino that let me capture and store the signal.
After some bit-to-temperature/humidity decoding i get the right conversion.
Finally i get my humidity and temperature sniffed and stored by the Arduino!




## More Info...


https://rayshobby.net/reverse-engineer-wireless-temperature-humidity-rain-sensors-part-1/

I followed this guide step by step, just let me point out something...

1) To connect my RF reciver to the sound card i use an old headphone, since is tripole,i connected the "single" 
wire to the DATA PIN of the RF Reciver and the "couple" wire to the ground.
2) The code that it's in the site, as the code you find in this repository, are HARD CODED; 
or better, based on the signal that is retrived by the wireless sensor. 
So if you don'thave the SAME wireless sensor, you have to write your code.

## Backeng a 433mhz H/T sensor with an Arduino

In this project i implement a well know hack!
Using a common 433 mhz RF Reciver, i decode a signal coming from a wireless sensor and directed to a weather station.
I just bougth on Amazon a weather station with 3 wirelss sensor like this one : 

https://www.bresser.de/en/Weather-Time/Weather-Stations/BRESSER-Temeo-Hygro-Quadro-thermo-and-hygrometer-with-4-independent-measuring-data.html

Then i use a MXRM5V RF reciver , working between 315mhz to 433mhz.

First i use Audacity to analyze and understand the wireless signal probed out by the sensors.
Than i write some code for Arduino that let me capture and store the signal.
After some bit-to-temperature decoding i get the right conversion.
Finally i get my humidity and temperature sniffed and stored by the Arduino!




## More Info...

https://rayshobby.net/reverse-engineer-wireless-temperature-humidity-rain-sensors-part-1/

I follow this guide step by step, just let me point out something...

1) To connect my RF reciver to the sound card i use an old headphone, since is tripole, connect the "single" 
wire to the DATA PIN of the RF Reciver and the couple to the ground.
2) The code that it's in the site, as the code you find in this repository are HARD CODED, 
or better, based on the signal that is retrived by the wireless sensor. So if you don't
have the SAME wireless sensor, you have to write your code.

https://fetzerch.github.io/2014/11/15/reveng433/

