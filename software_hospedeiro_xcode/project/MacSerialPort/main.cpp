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
#include <sstream>      // std::istringstream
#include <string>
using namespace std;

// Function to reverse words*/
string returnActiveTime(string s)
{
 
    // temporary vector to store all words
    vector<string> tmp;
    string str = "";
    for (int i = 0; i < s.length(); i++) {
 
        // Check if we encounter space
        // push word(str) to vector
        // and make str NULL
        if (s[i] == ' ') {
            tmp.push_back(str);
            str = "";
        }
 
        // Else add character to
        // str to form current word
        else
            str += s[i];
    }
    //cout << "-----------" << endl;
    //cout << s[3] << endl;
    //cout << "-----------" << endl;
    
    // Last word remaining,add it to vector
    tmp.push_back(str);
 
    // Now print from last to first in vector
    //int i;
    //for (i = tmp.size() - 1; i > 0; i--)
        //cout << tmp[i] << " ";
    // Last word remaining,print it
    //cout << tmp[0] << endl;
    
    return tmp[tmp.size() - 2]; //because last character is a space
}

//using namespace std::chrono;

int main(int argc, const char * argv[]) {
    //* Open port, and connect to a device
    const char devicePathStr[] = "/dev/tty.usbmodem1301";
    const int baudRate = 9600;
    char databuffer[1024];
    char databufferTime[1];
    vector<string> eventVec;
    vector<string> absoluteTimeVec;
    int choice = 0;
    bool loop = true;
    string activeTime;

    
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
        cout << "*******************************************************************************" << endl;
        cout << "Welcome 'admin'" << endl;
        cout << "Press 1 if you want to read a movement (5 seconds)" << endl;
        cout << "Press 2 if you want to check the log (queue)" << endl;
        cout << "Press 3 if you want to clear the log (queue)" << endl;
        cout << "Press 4 if you want to see the active time of Pico and close the serial port" << endl;
        //cout << "Press 5 to close serial port and exit the program" << endl;
        cout << "*******************************************************************************" << endl;
        cout << "Option: ";
        cin >> choice;
        
        switch (choice){
            case 1: {
                sleep(5);
                cout << endl;
                cout << endl;
                //readSerialData(databuffer, 140); //196 is the exact size of one message
                readSerialData(databuffer, 96);
                cout << "The event is: " << databuffer << endl;
                eventVec.push_back(databuffer);
                flushSerialData(); //clear the serial data after passing it to the host buffer
                sleep(0.5);
                cout << endl;
                cout << endl;
                break;}
            case 2: {
                cout << endl;
                cout << endl;
                for(int i = 0; i < eventVec.size(); i++) {
                    cout << "The event " << i << " in the queue of the host is:\n" ;
                    cout << eventVec[i] << endl;
                }
                cout << endl;
                cout << endl;
                break;}
            case 3: {
                cout << endl;
                cout << endl;
                eventVec.clear();
                cout << "The queue is now empty" << endl;
                cout << endl;
                cout << endl;
                break;}
            case 4: {
                sleep(4.5);
                cout << endl;
                cout << endl;
                readSerialData(databufferTime, 141);
                absoluteTimeVec.push_back(databufferTime);
                activeTime = returnActiveTime(absoluteTimeVec[0]); //reversing the string to print only the 'last' character -- it's the active time
                cout << "The active time of Pico is: " << activeTime << " (us) " << endl;
                cout << endl;
                cout << "Serial port closed\n";
                cout << endl;
                closeSerialPort(); // Close serial port when done
                loop = false;
                break;}
            case 5: {
                cout << endl;
                cout << endl;
                cout << "Serial port closed\n";
                cout << endl;
                cout << endl;
                closeSerialPort(); // Close serial port when done
                loop = false;
                break;}
        }
        
    }

    return 0;
}
