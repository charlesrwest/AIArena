#include <cstdio>

#include "gameEngineCommunicationInterface.hpp"
#include<exception>

int main(int argc, char **argv)
{

//Start game communication engine
gameEngineCommunicationInterface gameCom(16, 16);

std::string action;
unsigned char additionIntegers[2];
for(int i=1; i<256; i++)
{
additionIntegers[0] = i % 256;
additionIntegers[1] = (i+5) % 256;

std::string firstPercept;
firstPercept.push_back(additionIntegers[0]);
firstPercept.push_back(additionIntegers[1]);

//Initial percept is two numbers to add, with 0 reward
action = gameCom.sendPerceptionsAndGetActions( firstPercept, 0);

if(action.size() < 2)
{
fprintf(stderr, "Error, the action was of size %ld (should be 2)\n", action.size());
return -1;
}

//See if the action that the AI generated corresponses to the addition of the two integers
uint16_t expectedResult = ((uint16_t) additionIntegers[0]) + ((uint16_t) additionIntegers[1]);
uint16_t actionAsInteger = *((uint16_t *) action.c_str());


try
{
if(actionAsInteger == expectedResult)
{
action = gameCom.sendPerceptionsAndGetActions( firstPercept, 100, true);
}
else
{
action = gameCom.sendPerceptionsAndGetActions( firstPercept, 0, true);
}
}
catch(const std::exception &inputException)
{
fprintf(stderr, "Error: %s\n", inputException.what());
}

if(gameCom.AIWantsToEndSession())
{
break;
}

} 

return 0;
}
