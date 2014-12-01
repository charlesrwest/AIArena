#ifndef AICOMMUNICATIONINTERFACEHPP
#define AICOMMUNICATIONINTERFACEHPP


#include<future>
#include<thread>
#include<exception>
#include<string>
#include<unistd.h> //For delay
#include "zmq.hpp"

#include "SOMException.hpp"
#include "portLocations.hpp"
#include "perceptOrActionMessage.pb.h"

/*
This class makes it easier to write a game for AI Arena by abstracting away all of the communication details so that the programmer can just call a few simple functions.
*/
class AICommunicationInterface
{
public:
/*
This function establishes the connections used to run the AI/game interaction.
@exceptions: This function can throw exceptions (especially if starting the connection to the game times out)
*/
AICommunicationInterface();

/*
This function retrieves the most recent perceptions.
@return: The perceptions associated with the current round of the game 
@exceptions: This function can throw exceptions
*/
std::string getCurrentPerceptions();

/*
Get the reward associated with the last game round.
@return: The reward associated with the last game round
*/
uint64_t getCurrentReward();

/*
Get the size of the perception in bits.
@return: The size of the perception in bits
@exceptions: This function can throw exceptions
*/
uint64_t getSizeOfPerceptionInBits();

/*
Get the size of an action specification in bits.
@return: The size of an action specification in bits.
@exceptions: This function can throw exceptions
*/
uint64_t getSizeOfActionSpecificationInBits();

/*
This function sends what the AI decides to do.  The class takes care of all of the details associated with sending AI actions.  When the function returns, it is safe to retrieve percept and game related information.
@param inputAIActions: The data to send to the agent for it to act on (must have at least as many bits as sizeOfExpectedActionsInBits)
@param inputResetGame: Set this true to signal to the game that the AI would like to end the game prematurely
@param inputShutdownGame: Set this true to signal that the game engine should shut down

@exceptions: This function can throw some exceptions
*/
void sendActionsAndUpdatePerceptions(const std::string &inputAIActions, bool inputResetGame = false, bool inputShutdownGameEngine = false);

private:
std::unique_ptr<zmq::context_t> context;
std::unique_ptr<zmq::socket_t> actionPublishingSocket;
std::unique_ptr<zmq::socket_t> perceptReceptionSocket;

uint64_t perceptSequenceCounter;  //The expected value of the next percept sequence number


std::string currentPercept;
uint64_t currentReward;
uint64_t sizeOfPerceptionInBits;
uint64_t sizeOfExpectedActionInBits;
uint64_t sizeOfExpectedActionInBytes;
gameState currentGameState; //Start at the first percept of the new game, game over if the game is terminated, continue at any other time

/*
Update the catch of the current percept.
*/
void updateCurrentPerceptCache();
};






#endif
