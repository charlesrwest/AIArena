#include <cstdio>
#include "AICommunicationInterface.hpp"
#include<string>


/*
This (not too smart) AI treats the first two bytes of the percepts as unsigned chars and its action is to add them as a 16 bit unsigned integer.  Should really convert to network byte order and back.
*/
int main(int argc, char ** argv)
{
AICommunicationInterface AICom;


std::string percept = AICom.getCurrentPerceptions();

if(percept.size() != 2)
{
return -1;
}

printf("First percept (reward: %lu): %x %x\n", AICom.getCurrentReward(), percept[0], percept[1]);

//Send back the action
uint16_t actionInteger = ( (uint16_t) percept[0]) + ( (uint16_t) percept[1]);
std::string action;
action.push_back(((const unsigned char *) &actionInteger)[0]);
action.push_back(((const unsigned char *) &actionInteger)[1]);

AICom.sendActionsAndUpdatePerceptions(action);

percept = AICom.getCurrentPerceptions();

if(percept.size() != 2)
{
return -1;
}

printf("Second percept (reward: %lu): %x %x\n", AICom.getCurrentReward(), percept[0], percept[1]);

//End game
AICom.sendActionsAndUpdatePerceptions(action, false, true);

return 0;
}
