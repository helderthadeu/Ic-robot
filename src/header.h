#define HEADER_H
#ifdef HEADER_H

// #include <vector>
// #include <BLEDevice.h>
// #include <BLEServer.h>
// #include <iostream>
// #include <BLEUtils.h>
// #include <BLE2902.h>
// #include <Arduino.h>
// #include <time.h>
// #include <sstream>
// #include <string>
// #include <sys/time.h>

// #define sizeSquare 5


using namespace std;

// vector<String> split(String s, char del)
// {
//     vector<String> retorno;
//     string strCpp = string(s.c_str());
//     stringstream ss(strCpp);
//     string word;
//     for (int i = 0; !ss.eof(); i++)
//     {
//         getline(ss, word, del);

//         String strArduino = String(word.c_str());

//         retorno.push_back(strArduino);
//     }
//     return retorno;
// }

// void changePosition(int moviment[], String direction)
// {
//     if (direction.equals("frente") && moviment[0] >= 0 && moviment[0] < sizeSquare)
//     {
//         Serial.println("Fomos para frente");
//         moviment[0]++;
//     }
//     else if (direction.equals("atras")&& moviment[0]>=0 && moviment[0]<sizeSquare)
//     {
//         Serial.println("Fomos para atras");
//         moviment[0]++;
//     }
//     else if (direction.equals("direita")&& moviment[1]>=0 && moviment[1]<sizeSquare)
//     {
//         Serial.println("Fomos para direita");
//         moviment[1]++;
//     }
//     else if (direction.equals("esquerda")&& moviment[1]>=0 && moviment[1]<sizeSquare)
//     {
//         Serial.println("Fomos para esquerda");
//         moviment[1]++;
//     }
// }

// void sendMessage(String message)
// {
//     characteristicTX->setValue(message.c_str());
//     characteristicTX->notify();
// }
// void sendMessage(string message)
// {
//     characteristicTX->setValue(message);
//     characteristicTX->notify();
// }



#endif
