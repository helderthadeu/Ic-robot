#include <vector>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <iostream>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <time.h>
#include <sstream>
#include <string>
#include <sys/time.h>
#include "header.h"

using namespace std;

#define SERVICE_UUID "6ff0ae3b-24aa-46ec-8521-436605ce3fbe"
#define CHARACTERISTIC_UUID_RX "6a1981ae-6033-456a-88a9-a0d6b3eab72f"
#define CHARACTERISTIC_UUID_TX "c14b6701-76a3-454a-9dcf-80902b406cba"
#define sizeSquare 5
#define robotSelected 1

BLECharacteristic *characteristicTX;

vector<String> split(String s, char del);
void changePosition(int moviment[], String direction);
void sendMessage(String message);
int multiplier(String value);
void changeHorizontal(int prev[2], int next[2]);
void changeVertical(int prev[2], int next[2]);

class LevelDetails
{
private:
    int numLevel;
    vector<String> levelInstruction;

public:
    LevelDetails();
    void setValues(vector<String> dataReceived);
    vector<String> getLevelInstruction();
};

bool deviceConnected = false;

int controlledMoviment[2] = {0, sizeSquare / 2};
int levelMoviment[2] = {sizeSquare - 1, sizeSquare / 2};
int selfMoviment[2];

bool startMoviment = false;
LevelDetails currentLevelDetails;
vector<vector<String>> commandsCompiled;
int numMovimentCompiled = 0;
int numMovimentLevel = 0;
int multiplierController = 1;

unsigned int previousMillis;

class CharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *characteristic)
    {

        string rxValue = characteristic->getValue();

        if (rxValue.length() > 0)
        {
            Serial.print("Valor recebido: ");
            for (int i = 0; i < rxValue.length(); i++)
            {
                Serial.print(rxValue[i]);
            }
            Serial.println();
            if (rxValue.find("currentPattern") != -1)
            {
                String temp = String((rxValue.c_str()));
                currentLevelDetails.setValues(split(temp, '@'));
            }
            if (rxValue.find("commands") != -1)
            {
                if (!commandsCompiled.empty())
                {
                    commandsCompiled.clear();
                }

                vector<String> temp = split(String((rxValue.c_str())), '@');
                for (int i = 0; i < temp.size(); i++)
                {
                    if (i > 0)
                    {
                        commandsCompiled.push_back({split(temp[i], ':')[0], split(temp[i], ':')[1]});
                    }
                }
                Serial.print("Comandos: ");
                for (vector<String> i : commandsCompiled)
                {
                    Serial.printf("%s, ", i[0]);
                }
            }
            if (rxValue.find("start") != -1)
            {
                startMoviment = true;
            }
        }
    }
};

class ServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        Serial.println("Device connected!");
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        Serial.println("Device disconected");
        deviceConnected = false;
        startMoviment = false;

        BLEDevice::startAdvertising();
        Serial.println("Waiting device conect...");
    }
};

void setup()
{

    Serial.begin(115200);

    // Start device
    BLEDevice::init("RobotCar1");

    // Start Server
    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    // Start Service
    BLEService *service = server->createService(SERVICE_UUID);
    characteristicTX = service->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
    characteristicTX->addDescriptor(new BLE2902());

    // Start Characteristic
    BLECharacteristic *characteristic = service->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
    characteristic->setCallbacks(new CharacteristicCallbacks());


    service->start();
    server->getAdvertising()->start();

    Serial.println("Waiting device conect...");
    selfMoviment[0] = (robotSelected == 1) ? selfMoviment[0] : levelMoviment[0];
    selfMoviment[1] = (robotSelected == 1) ? selfMoviment[1] : levelMoviment[1];
}

void loop()
{
    unsigned int currentMillis = millis();
    if (deviceConnected && currentMillis - previousMillis >= 3000)
    {
        previousMillis = currentMillis;
        if (startMoviment && numMovimentCompiled < commandsCompiled.size() && !commandsCompiled[numMovimentCompiled][0].equals("none"))
        {

            
            // Print the table with the two robots
            for (int i = 0; i < sizeSquare; i++)
            {
                for (int j = 0; j < sizeSquare; j++)
                {
                    if (i == controlledMoviment[0] && j == controlledMoviment[1])
                    {
                        Serial.print("x ");
                    }
                    else if (i == levelMoviment[0] && j == levelMoviment[1])
                    {
                        Serial.print("o ");
                    }
                    else
                    {
                        Serial.print("- ");
                    }
                }
                Serial.println();
            }
            Serial.println();

            String tempLevel = currentLevelDetails.getLevelInstruction()[numMovimentLevel];
            numMovimentCompiled = (multiplierController != 1) ? numMovimentCompiled : numMovimentCompiled + 1;
            if (multiplierController > 1)
            {
                changePosition(controlledMoviment, commandsCompiled[numMovimentCompiled][0]);
                multiplierController--;
            }
            else
            {

                // Change Robot Position
                changePosition(controlledMoviment, commandsCompiled[numMovimentCompiled][0]);
                changePosition(levelMoviment, tempLevel);
                
                numMovimentLevel = (numMovimentLevel != currentLevelDetails.getLevelInstruction().size() - 1) ? numMovimentLevel + 1 : 0;
                multiplierController = multiplier(commandsCompiled[numMovimentCompiled][1]);

            }

            if (controlledMoviment[0] == levelMoviment[0] && controlledMoviment[1] == levelMoviment[1])
            {
                startMoviment = false;
                Serial.println("Robot found!");
                sendMessage("Robot found!");
            }

            if (numMovimentCompiled == commandsCompiled.size() || !startMoviment)
            {
                sendMessage("The game is over!");
                Serial.println("The game is over!");
            }
        }
        else
        {
            numMovimentLevel = 0;
            numMovimentCompiled = 0;
            multiplierController = 1;
            controlledMoviment[0] = 0;
            controlledMoviment[1] = 2;
            levelMoviment[0] = 4;
            levelMoviment[1] = 2;

            startMoviment = false;
        }

        if (robotSelected == 1)
        {
            if (selfMoviment[0] != controlledMoviment[0])
            {
                changeVertical(selfMoviment, controlledMoviment);
                selfMoviment[0] = controlledMoviment[0];
            }
            if (selfMoviment[1] != controlledMoviment[1])
            {
                changeHorizontal(selfMoviment, controlledMoviment);
                selfMoviment[1] = controlledMoviment[1];
            }
        }
        else
        {
            if (levelMoviment[0] != controlledMoviment[0])
            {
                changeVertical(levelMoviment, controlledMoviment);
                levelMoviment[0] = controlledMoviment[0];
            }
            if (levelMoviment[1] != controlledMoviment[1])
            {
                changeHorizontal(levelMoviment, controlledMoviment);
                levelMoviment[1] = controlledMoviment[1];
            }
        }

        /** Test to verify the communication from app through serial monitor
        if (Serial.available())
        {
            String input = Serial.readStringUntil('\n'); 


            Serial.print("What I'm sending: ");
            Serial.println(input); 

            characteristicTX->setValue(input.c_str()); 
            characteristicTX->notify();                
        }
         */
    }
}

LevelDetails::LevelDetails()
{
}

/**
 * @brief Define the instructions pattern
 * 
 * @param dataReceived Vector with the follow format: command | level number | instructions 
 */
void LevelDetails::setValues(vector<String> dataReceived)
{
    for (int i = 0; i < dataReceived.size(); i++)
    {
        switch (i)
        {
        case 0:
            break;
        case 1:
            this->numLevel = dataReceived.at(i).toInt();
            break;
        default:

            levelInstruction.push_back(dataReceived.at(i));
            break;
        }
    }

    /** Print information about data loaded
     *
     * Serial.println("Values loaded sucesfully!\n Number level: ");
    Serial.print(this->numLevel);
    Serial.print("Pattern level: ");
     *
     */
}

vector<String> LevelDetails::getLevelInstruction()
{
    return this->levelInstruction;
}

/**
 * @brief Split a string returning a vector
 * 
 * @param string_Splited string to be splited
 * @param del character to split the string
 * @return vector<String> 
 */
vector<String> split(String string_Splited, char del)
{
    vector<String> retorno;
    string strCpp = string(string_Splited.c_str());
    stringstream ss(strCpp);
    string word;
    for (int i = 0; !ss.eof(); i++)
    {
        getline(ss, word, del);

        String strArduino = String(word.c_str());

        retorno.push_back(strArduino);
    }
    return retorno;
}

/**
 * @brief Change a moviment value in an array
 * 
 * @param moviment Moviment array
 * @param direction
 */
void changePosition(int moviment[], String direction)
{
    if (direction.equals("ahead") && moviment[0] < sizeSquare)
    {
        
        moviment[0]++;
    }
    else if (direction.equals("back") && moviment[0] > 0)
    {
        
        moviment[0]--;
    }
    else if (direction.equals("right") && moviment[1] > 0)
    {
        
        moviment[1]--;
    }
    else if (direction.equals("left") && moviment[1] < sizeSquare)
    {
        
        moviment[1]++;
    }
}

/**
 * @brief Calculate the multiplier value 
 * 
 * @param value Value to be calculate in String format
 * @return int Return the calculated value
 */
int multiplier(String value)
{

    if (value.equals("xTwo"))
    {
        return 2;
    }
    else if (value.equals("xThree"))
    {
        return 3;
    }
    else
    {
        return 1;
    }
}

/**
 * 
 * @brief Send a message to the connected device
 * 
 * @param message 
 */
void sendMessage(String message)
{
    characteristicTX->setValue(message.c_str());
    characteristicTX->notify();
}

