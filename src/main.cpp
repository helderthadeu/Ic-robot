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
#define robotSelected 1 // 1 = Controlled - 2 = Pre defined

BLECharacteristic *characteristicTX; // através desse objeto iremos enviar dados para o client

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

bool deviceConnected = false; // controle de dispositivo conectado

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

    // Create the BLE Device
    BLEDevice::init("RobotCar1"); // nome do dispositivo bluetooth

    // Create the BLE Server
    BLEServer *server = BLEDevice::createServer(); // cria um BLE server
    // ServerCallbacks *servCall = new ServerCallbacks();
    server->setCallbacks(new ServerCallbacks()); // seta o callback do server

    // Create the BLE Service
    BLEService *service = server->createService(SERVICE_UUID);

    // Create a BLE Characteristic para envio de dados
    characteristicTX = service->createCharacteristic(CHARACTERISTIC_UUID_TX,
                                                     BLECharacteristic::PROPERTY_NOTIFY);
    characteristicTX->addDescriptor(new BLE2902());

    // Create a BLE Characteristic para recebimento de dados
    BLECharacteristic *characteristic = service->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

    // CharacteristicCallbacks *corCall = new CharacteristicCallbacks();
    characteristic->setCallbacks(new CharacteristicCallbacks());

    // Start the service
    service->start();

    // Start advertising (descoberta do ESP32)
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
        // sendMessage("Dispositivo conectado");
        if (startMoviment && numMovimentCompiled < commandsCompiled.size() && !commandsCompiled[numMovimentCompiled][0].equals("nada"))
        {

            Serial.printf("Bloco atual: %d\n", numMovimentCompiled+1);
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

            // Serial.printf("Posição do movimento próprio: linha %d coluna %d\n", levelMoviment[0] + 1, levelMoviment[1] + 1);
            // Serial.printf("Posição do movimento compilado: linha %d coluna %d\n", controlledMoviment[0] + 1, controlledMoviment[1] + 1);

            String tempLevel = currentLevelDetails.getLevelInstruction()[numMovimentLevel];
            numMovimentCompiled = (multiplierController != 1) ? numMovimentCompiled : numMovimentCompiled + 1;
            if (multiplierController > 1)
            {
                
                // Serial.println();

                // Serial.println("Está repetindo");
                // Serial.printf("Movimento do self: %s\n", commandsCompiled[numMovimentCompiled][0]);
                changePosition(controlledMoviment, commandsCompiled[numMovimentCompiled][0]);
                multiplierController--;
                // Serial.println("\n");
            }
            else
            {

                // Serial.printf("Movimento do self: %s\n", commandsCompiled[numMovimentCompiled][0]);
                changePosition(controlledMoviment, commandsCompiled[numMovimentCompiled][0]);

                // Serial.printf("Movimento do other: %s\n", tempLevel);
                changePosition(levelMoviment, tempLevel);
                // Serial.println("\n");

                numMovimentLevel = (numMovimentLevel != currentLevelDetails.getLevelInstruction().size() - 1) ? numMovimentLevel + 1 : 0;

                multiplierController = multiplier(commandsCompiled[numMovimentCompiled][1]);
                // for (int i = 0; i < sizeSquare; i++)
                // {
                //     for (int j = 0; j < sizeSquare; j++)
                //     {
                //         if (i == controlledMoviment[0] && j == controlledMoviment[1])
                //         {
                //             Serial.print("x ");
                //         }
                //         else if (i == levelMoviment[0] && j == levelMoviment[1])
                //         {
                //             Serial.print("o ");
                //         }
                //         else
                //         {
                //             Serial.print("- ");
                //         }
                //     }
                //     Serial.println();
                // }
                // Serial.println();
            }

            if (controlledMoviment[0] == levelMoviment[0] && controlledMoviment[1] == levelMoviment[1])
            {
                startMoviment = false;
                Serial.println("Robos encontrados");
                sendMessage("Robos encontrados");
            }

            if (numMovimentCompiled == commandsCompiled.size() || !startMoviment)
            {
                Serial.println("Fim de jogo!");
            }
        }
        else
        {
            // Serial.print("Fim de jogo!");
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

        /** Teste para verificar a comunicação com o app via serial monitor
        if (Serial.available())
        {
            // Lê todos os caracteres até pressionar "Enter"
            String input = Serial.readStringUntil('\n'); // Lê até o final da linha (Enter)

            // Mostra no monitor serial o que foi enviado
            Serial.print("O que estou enviando: ");
            Serial.println(input); // Mostra a string completa

            // Envia a string para o BLE
            characteristicTX->setValue(input.c_str()); // Envia a string para a característica
            characteristicTX->notify();                // Notifica que o valor foi alterado
        }
         */
    }
}

LevelDetails::LevelDetails()
{
}

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

    /** Print informações sobre os dados carregados
     *
     * Serial.println("Values loaded sucesfully!\n Number level: ");
    Serial.print(this->numLevel);
    Serial.print("Pattern level: ");
     *
     */

    // for (String data : dataReceived)
    // {
    //     Serial.println(data);
    // }
}

vector<String> LevelDetails::getLevelInstruction()
{
    return this->levelInstruction;
}

vector<String> split(String s, char del)
{
    vector<String> retorno;
    string strCpp = string(s.c_str());
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

void changePosition(int moviment[], String direction)
{
    if (direction.equals("frente") && moviment[0] < sizeSquare)
    {
        // Serial.println("Fomos para frente");
        moviment[0]++;
    }
    else if (direction.equals("atras") && moviment[0] > 0 )
    {
        // Serial.println("Fomos para atras");
        moviment[0]--;
    }
    else if (direction.equals("direita") && moviment[1] > 0 )
    {
        // Serial.println("Fomos para direita");
        moviment[1]--;
    }
    else if (direction.equals("esquerda") && moviment[1] < sizeSquare)
    {
        // Serial.println("Fomos para esquerda");
        moviment[1]++;
    }
}

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

void sendMessage(String message)
{
    characteristicTX->setValue(message.c_str());
    characteristicTX->notify();
}

void changeVertical(int prev[2], int next[2])
{
}

void changeHorizontal(int prev[2], int next[2])
{
}
