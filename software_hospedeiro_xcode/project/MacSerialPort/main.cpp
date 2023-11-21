/*
EEL510265- Programação de Sistemas Embarcados

Aluno: Jonas Lendzion Schultz II
 
Professor: Eduardo Bezerra

USB Serial Port Reading Program for MacOS using Xcode development
 
*/

//Includes
#include "SerialPort/SerialPort.hpp"
#include "TypeAbbreviations/TypeAbbreviations.hpp"
#include <iostream>
using namespace std;



int main(int argc, const char * argv[]) {
    //* Open port, and connect to a device
    const char devicePathStr[] = "/dev/tty.usbmodem11301";
    const int baudRate = 9600;
    char databuffer[1024];
    vector<string> eventVec;
    int choice = 0;
    bool loop = true;
    
    int sfd = openAndConfigureSerialPort(devicePathStr, baudRate);
    if (sfd < 0) {
        if (sfd == -1) {
            printf("Unable to connect to serial port.\n");
        }
        else { //sfd == -2
            printf("Error setting serial port attributes.\n");
        }
        return 0;
    }
    while (loop){
        cout << "********************************************************" << endl;
        cout << "Welcome 'admin'" << endl;
        cout << "Press 1 if you want to read a movement (5 seconds)" << endl;
        cout << "Press 2 if you want to check the log" << endl;
        cout << "Press 3 if you want to clear the queue" << endl;
        cout << "Press 4 if you want to see the active time of Pico" << endl;
        cout << "Press 5 to close serial port and exit the program" << endl;
        cout << "********************************************************" << endl;
        cout << "Option: ";
        cin >> choice;
        
        switch (choice){
            case 1:
                sleep(5);
                cout << endl;
                cout << endl;
                readSerialData(databuffer, 76); //76 is the exact size of the message
                cout << "The event is: " << databuffer << endl;
                eventVec.push_back(databuffer);
                flushSerialData(); //clear the serial data in the buffer
                sleep(0.5);
                cout << endl;
                cout << endl;
                break;
            case 2:
                cout << endl;
                cout << endl;
                for(int i = 0; i < eventVec.size(); i++) {
                    cout << "The event " << i << " in the queue is:\n" ;
                    cout << eventVec[i] << endl;
                }
                cout << endl;
                cout << endl;
                break;
            case 3:
                eventVec.clear();
                cout << "The queue is now empty" << endl;
                break;
            case 4:
                break;
            case 5:
                cout << endl;
                cout << endl;
                cout << "Serial port closed\n";
                cout << endl;
                cout << endl;
                closeSerialPort(); // Close serial port when done
                loop = false;
                break;
        }
        
    }
/*
    for (int i = 1; i <= 12; i++){
//    while (true){
        if (i==1){ //to give time to receive a data in the serial, otherwise the first data of strVec will be 0
            sleep(5);
        }
        readSerialData(databuffer, 76); //76 is the exact size of the message
        cout << "The event read is: " << databuffer << endl;
        strVec.push_back(databuffer);
        flushSerialData();
        // * Read using readSerialData(char* bytes, size_t length)
        sleep(5);
        
        // * Remember to flush potentially buffered data when necessary
    
    }

    for(int i = 0; i < strVec.size(); i++) {
        cout << "O envento " << i << " eh: " ;
        cout << strVec[i] << endl;
    }*/

    return 0;
}
