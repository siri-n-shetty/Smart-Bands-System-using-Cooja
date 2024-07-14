# Smart-Bands-System-using-Cooja

This project focuses on analysing and simulating a smart band system using the Cooja simulator within Contiki OS. The system consists of parent and child bands that communicate wirelessly to monitor the child's status and alert the parent in case of emergencies such as falls or prolonged absence.

Using Cooja, the simulation emulates real-world scenarios with sky motes representing the smart bands. The system is designed to trigger alarms for events like missing signals or detecting a fall, ensuring timely alerts for the parent.

Through this simulation, the project aims to demonstrate the reliability and effectiveness of smart band systems in enhancing child safety and providing peace of mind for parents.

## Important Observations

* The code simulates two bands (parent and child) communicating via broadcast and unicast messages.
* This system operates in two phases, the Pairing Phase and the Operation Phase.
* The parent band sends a pairing message every 2 seconds until pairing is complete, then closes the broadcast, sends a **Stop Pairing** message, and enters **Operation Mode**.
* The child band behaves similarly during the Pairing Phase and sends its information to the parent every 10 seconds in **Operation Mode**.
* If the parent band does not receive any unicast message from the child’s address for 60 seconds, it prints a “MISSING” alarm.
* There was no effect on the performance of the bands with different mobility models. (RandomWaypoint, Random Walk, Manhattan Grid and RPGM)
* The PCAP file was analysed to obtain Zigbee and IEEE 802.15.4 traffic.

## Usage

* Make sure you have Cooja simulator ready within the Contiki Operating System.
* Clone this repository to the Contiki OS machine.
```sh
git clone https://github.com/siri-n-shetty/Smart-Bands-System-using-Cooja.git
```
* Ensure the correct directory structure:
```sh
/home/user/
├── contiki/
│   ├── Makefile.include
│   └── ...
└── smart-bands/
    ├── Makefile
    ├── smart-bands.h
    └── ...

```
* Open Cooja simulator, create a simulation, and add the desired number of sky-motes. (Or create new parents and children with different pairs of hardcoded keys)
* Start the simulation and analyse it according to your goal.

## Contributors

* Siri N Shetty
* Craig Nigel Fernandes
* Shravya Reddy B

This project was developed as a part of the summer course **Developing Next-Gen IoT Solutions with Contiki OS and Cooja Simulator**. Feel free to contribute by submitting issues for any bugs or improvements. 
